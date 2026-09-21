#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace {
constexpr std::uint32_t kLogicalSector = 2048u;
constexpr std::uint32_t kCdiV2 = 0x80000004u;
constexpr std::uint32_t kCdiV3 = 0x80000005u;
constexpr std::uint32_t kCdiV35 = 0x80000006u;

std::uint32_t le32(const std::uint8_t* p) {
    return p[0] | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
std::uint16_t le16(const std::uint8_t* p) { return p[0] | (std::uint16_t(p[1]) << 8); }

std::string trim(const std::uint8_t* p, std::size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
    while (!s.empty() && (s.front() == ' ' || s.front() == '\0')) s.erase(s.begin());
    return s;
}

std::vector<std::uint8_t> read_at(std::ifstream& f, std::uint64_t off, std::size_t n) {
    f.clear();
    f.seekg(0, std::ios::end);
    const auto end = f.tellg();
    if (end < 0 || off >= static_cast<std::uint64_t>(end)) return {};
    const auto avail = static_cast<std::uint64_t>(end) - off;
    n = static_cast<std::size_t>(std::min<std::uint64_t>(n, avail));
    std::vector<std::uint8_t> out(n);
    f.seekg(static_cast<std::streamoff>(off), std::ios::beg);
    if (!f) return {};
    f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
    out.resize(static_cast<std::size_t>(f.gcount()));
    return out;
}

std::uint32_t read_u32(std::ifstream& f) {
    std::array<std::uint8_t, 4> b{};
    f.read(reinterpret_cast<char*>(b.data()), 4);
    if (!f) throw std::runtime_error("Unexpected end of CDI metadata");
    return le32(b.data());
}
std::uint16_t read_u16(std::ifstream& f) {
    std::array<std::uint8_t, 2> b{};
    f.read(reinterpret_cast<char*>(b.data()), 2);
    if (!f) throw std::runtime_error("Unexpected end of CDI metadata");
    return le16(b.data());
}
std::uint8_t read_u8(std::ifstream& f) {
    char c{}; f.read(&c, 1); if (!f) throw std::runtime_error("Unexpected end of CDI metadata");
    return static_cast<std::uint8_t>(c);
}

struct Entry { std::string name; std::uint32_t extent{}, size{}; bool dir{}; };

struct Track {
    std::uint32_t session{};
    std::uint32_t number{};
    std::uint64_t file_base{};      // first physical sector including pregap
    std::uint64_t data_position{};  // FAD start maps here
    std::uint32_t pregap{};
    std::uint32_t length{};
    std::uint32_t total_length{};
    std::uint32_t start_lba{};
    std::uint32_t mode{};
    std::uint32_t sector_size{};
    std::uint32_t user_offset{};

    std::uint32_t start_fad() const { return start_lba + pregap; }
    std::uint32_t end_fad() const { return length ? start_fad() + length - 1u : start_fad(); }
    bool data() const { return mode != 0u; }
};

struct DiscLayout {
    std::vector<Track> tracks;
    bool from_cdi{};
};

bool track_mark(const std::vector<std::uint8_t>& b) {
    constexpr std::array<std::uint8_t, 10> m{0,0,1,0,0,0,0xFF,0xFF,0xFF,0xFF};
    return b.size() == m.size() && std::equal(m.begin(), m.end(), b.begin());
}

Track read_cdi_track(std::ifstream& f, std::uint64_t& file_offset, std::uint32_t version,
                     std::uint32_t session, std::uint32_t number) {
    Track t{}; t.session = session; t.number = number;
    auto tmp = read_u32(f);
    if (tmp != 0u) f.seekg(8, std::ios::cur);
    std::vector<std::uint8_t> mark(10);
    f.read(reinterpret_cast<char*>(mark.data()), 10); if (!track_mark(mark)) throw std::runtime_error("CDI track mark #1 missing");
    f.read(reinterpret_cast<char*>(mark.data()), 10); if (!track_mark(mark)) throw std::runtime_error("CDI track mark #2 missing");
    f.seekg(4, std::ios::cur);
    const auto filename_len = read_u8(f);
    f.seekg(filename_len, std::ios::cur);
    f.seekg(11 + 4 + 4, std::ios::cur);
    tmp = read_u32(f);
    if (tmp == 0x80000000u) f.seekg(8, std::ios::cur);
    f.seekg(2, std::ios::cur);
    t.pregap = read_u32(f);
    t.length = read_u32(f);
    f.seekg(6, std::ios::cur);
    t.mode = read_u32(f);
    f.seekg(12, std::ios::cur);
    t.start_lba = read_u32(f);
    t.total_length = read_u32(f);
    f.seekg(16, std::ios::cur);
    const auto sector_id = read_u32(f);
    if (sector_id == 0u) t.sector_size = 2048u;
    else if (sector_id == 1u) t.sector_size = 2336u;
    else if (sector_id == 2u) t.sector_size = 2352u;
    else throw std::runtime_error("Unsupported CDI sector-size id " + std::to_string(sector_id));
    if (t.mode > 2u) throw std::runtime_error("Unsupported CDI track mode " + std::to_string(t.mode));
    if (t.sector_size == 2048u) t.user_offset = 0u;
    else if (t.sector_size == 2336u) t.user_offset = 8u;
    else if (t.mode == 1u) t.user_offset = 16u;
    else if (t.mode == 2u) t.user_offset = 24u;
    else t.user_offset = 0u;

    t.file_base = file_offset;
    t.data_position = file_offset + std::uint64_t(t.pregap) * t.sector_size;
    file_offset += std::uint64_t(t.total_length) * t.sector_size;

    f.seekg(29, std::ios::cur);
    if (version != kCdiV2) {
        f.seekg(5, std::ios::cur);
        tmp = read_u32(f);
        if (tmp == 0xFFFFFFFFu) f.seekg(78, std::ios::cur);
    }
    return t;
}

std::optional<DiscLayout> parse_cdi(std::ifstream& f) {
    f.clear(); f.seekg(0, std::ios::end);
    const auto end = f.tellg();
    if (end < 8) return std::nullopt;
    const auto size = static_cast<std::uint64_t>(end);
    f.seekg(static_cast<std::streamoff>(size - 8u), std::ios::beg);
    const auto version = read_u32(f);
    const auto header_value = read_u32(f);
    if (version != kCdiV2 && version != kCdiV3 && version != kCdiV35) return std::nullopt;
    if (header_value == 0u) throw std::runtime_error("CDI header value is zero");
    const std::uint64_t header_pos = version == kCdiV35 ? size - header_value : header_value;
    if (header_pos >= size - 8u) throw std::runtime_error("CDI metadata header lies outside image");
    f.seekg(static_cast<std::streamoff>(header_pos), std::ios::beg);
    const auto sessions = read_u16(f);
    if (sessions == 0u || sessions > 99u) throw std::runtime_error("Implausible CDI session count");
    DiscLayout out{}; out.from_cdi = true;
    std::uint64_t track_file_offset = 0u;
    std::uint32_t global_track_number = 0u;
    for (std::uint32_t s = 1; s <= sessions; ++s) {
        const auto count = read_u16(f);
        if (count == 0u || count > 99u) throw std::runtime_error("Implausible CDI track count");
        for (std::uint32_t t = 1; t <= count; ++t) {
            if (++global_track_number > 99u) throw std::runtime_error("CDI contains more than 99 tracks");
            out.tracks.push_back(read_cdi_track(f, track_file_offset, version, s, global_track_number));
        }
        f.seekg(4 + 8, std::ios::cur);
        if (version != kCdiV2) f.seekg(1, std::ios::cur);
    }
    if (track_file_offset != header_pos)
        throw std::runtime_error("CDI track table does not match container data length");
    return out;
}

bool starts_with(const std::vector<std::uint8_t>& b, std::string_view s) {
    return b.size() >= s.size() && std::equal(s.begin(), s.end(), b.begin());
}

std::vector<std::uint8_t> read_track_bytes(std::ifstream& f, const Track& t,
                                           std::int64_t first_physical_sector,
                                           std::uint32_t byte_offset,
                                           std::uint32_t bytes) {
    if (!t.data() || t.user_offset + kLogicalSector > t.sector_size || first_physical_sector < 0) return {};
    std::vector<std::uint8_t> out(bytes);
    std::uint32_t done = 0u;
    while (done < bytes) {
        const auto physical = first_physical_sector + static_cast<std::int64_t>((byte_offset + done) / kLogicalSector);
        if (physical < 0 || static_cast<std::uint64_t>(physical) >= t.length) return {};
        const auto within = (byte_offset + done) % kLogicalSector;
        const auto take = std::min<std::uint32_t>(bytes - done, kLogicalSector - within);
        const auto off = t.data_position + std::uint64_t(physical) * t.sector_size + t.user_offset + within;
        auto part = read_at(f, off, take);
        if (part.size() != take) return {};
        std::copy(part.begin(), part.end(), out.begin() + done);
        done += take;
    }
    return out;
}

std::vector<Entry> parse_directory(const std::vector<std::uint8_t>& b) {
    std::vector<Entry> out;
    std::size_t pos = 0;
    while (pos < b.size()) {
        const auto len = b[pos];
        if (!len) { pos = ((pos / kLogicalSector) + 1u) * kLogicalSector; continue; }
        if (len < 34u || pos + len > b.size()) break;
        const auto nl = b[pos + 32u];
        if (33u + nl > len) break;
        if (!(nl == 1u && (b[pos + 33u] == 0u || b[pos + 33u] == 1u))) {
            std::string name(reinterpret_cast<const char*>(b.data() + pos + 33u), nl);
            if (const auto semi = name.find(';'); semi != std::string::npos) name.resize(semi);
            out.push_back({name, le32(b.data() + pos + 2u), le32(b.data() + pos + 10u), (b[pos + 25u] & 2u) != 0u});
        }
        pos += len;
    }
    return out;
}

const Entry* find_entry(const std::vector<Entry>& entries, std::string_view wanted) {
    for (const auto& e : entries) {
        if (e.name.size() != wanted.size()) continue;
        bool ok = true;
        for (std::size_t i = 0; i < wanted.size(); ++i)
            if (std::toupper(static_cast<unsigned char>(e.name[i])) != std::toupper(static_cast<unsigned char>(wanted[i]))) { ok = false; break; }
        if (ok) return &e;
    }
    return nullptr;
}

struct BootTrack {
    std::size_t track_index{};
    std::int32_t iso_physical_base{}; // physical sector corresponding to ISO LBA 0 for relative extents
    bool absolute_extents{};
    std::string volume;
    std::vector<Entry> root;
    std::vector<std::uint8_t> ip;
    std::string boot_name;
    std::string title;
    Entry boot_entry{};
    std::vector<std::uint8_t> boot;
    std::uint64_t boot_offset{};
};

std::vector<std::uint8_t> read_iso_extent(std::ifstream& f, const Track& t, const BootTrack& bt,
                                          std::uint32_t extent, std::uint32_t size) {
    const std::int64_t physical = bt.absolute_extents
        ? static_cast<std::int64_t>(extent) - static_cast<std::int64_t>(t.start_lba)
        : static_cast<std::int64_t>(bt.iso_physical_base) + extent;
    return read_track_bytes(f, t, physical, 0u, size);
}

std::optional<BootTrack> inspect_boot_track(std::ifstream& f, const DiscLayout& layout, std::size_t index) {
    const auto& t = layout.tracks[index];
    if (!t.data() || t.user_offset + kLogicalSector > t.sector_size) return std::nullopt;
    constexpr std::array<std::uint8_t, 7> pvd_sig{1,'C','D','0','0','1',1};
    std::int32_t pvd_sector = -1, ip_sector = -1;
    std::vector<std::uint8_t> pvd;
    for (std::uint32_t s = 0; s < std::min<std::uint32_t>(256u, t.length); ++s) {
        auto head = read_track_bytes(f, t, s, 0u, 16u);
        if (head.size() < 16u) break;
        if (ip_sector < 0 && starts_with(head, "SEGA SEGAKATANA")) ip_sector = static_cast<std::int32_t>(s);
        if (pvd_sector < 0 && std::equal(pvd_sig.begin(), pvd_sig.end(), head.begin())) {
            pvd_sector = static_cast<std::int32_t>(s);
            pvd = read_track_bytes(f, t, s, 0u, kLogicalSector);
        }
        if (ip_sector >= 0 && pvd_sector >= 0) break;
    }
    if (pvd_sector < 0 || pvd.size() < 190u || le16(pvd.data() + 128u) != kLogicalSector) return std::nullopt;

    BootTrack bt{}; bt.track_index = index; bt.iso_physical_base = pvd_sector - 16;
    bt.volume = trim(pvd.data() + 40u, 32u);
    if (pvd[156] < 34u) return std::nullopt;
    const auto root_extent = le32(pvd.data() + 158u);
    const auto root_size = le32(pvd.data() + 166u);
    if (root_size == 0u || root_size > 16u * 1024u * 1024u) return std::nullopt;

    // Multisession self-boot discs commonly store ISO9660 extents as absolute
    // disc LBAs (e.g. Shenmue: start LBA 11700, root extent 11723). Try that
    // first when it is plausible, then fall back to ordinary ISO-relative LBAs.
    auto try_root = [&](bool absolute) -> std::vector<Entry> {
        BootTrack temp = bt; temp.absolute_extents = absolute;
        auto bytes = read_iso_extent(f, t, temp, root_extent, root_size);
        return bytes.size() == root_size ? parse_directory(bytes) : std::vector<Entry>{};
    };
    auto absolute_root = root_extent >= t.start_lba ? try_root(true) : std::vector<Entry>{};
    auto relative_root = try_root(false);
    if (!absolute_root.empty()) { bt.absolute_extents = true; bt.root = std::move(absolute_root); }
    else if (!relative_root.empty()) { bt.absolute_extents = false; bt.root = std::move(relative_root); }
    else return std::nullopt;

    if (ip_sector >= 0) bt.ip = read_track_bytes(f, t, ip_sector, 0u, 16u * kLogicalSector);
    if (bt.ip.size() < 0x100u || !starts_with(bt.ip, "SEGA SEGAKATANA")) {
        const auto* ip_entry = find_entry(bt.root, "IP.BIN");
        if (!ip_entry) return std::nullopt;
        bt.ip = read_iso_extent(f, t, bt, ip_entry->extent, ip_entry->size);
    }
    if (bt.ip.size() < 0x100u || !starts_with(bt.ip, "SEGA SEGAKATANA")) return std::nullopt;
    bt.boot_name = trim(bt.ip.data() + 0x60u, 16u);
    bt.title = trim(bt.ip.data() + 0x80u, std::min<std::size_t>(128u, bt.ip.size() - 0x80u));
    const auto* boot_entry = find_entry(bt.root, bt.boot_name);
    if (!boot_entry) return std::nullopt;
    bt.boot_entry = *boot_entry;
    bt.boot = read_iso_extent(f, t, bt, boot_entry->extent, boot_entry->size);
    if (bt.boot.size() != boot_entry->size) return std::nullopt;
    const std::int64_t physical = bt.absolute_extents
        ? static_cast<std::int64_t>(boot_entry->extent) - static_cast<std::int64_t>(t.start_lba)
        : static_cast<std::int64_t>(bt.iso_physical_base) + boot_entry->extent;
    bt.boot_offset = t.data_position + std::uint64_t(physical) * t.sector_size + t.user_offset;
    return bt;
}

DiscLayout raw_2048_layout(std::ifstream& f) {
    f.clear(); f.seekg(0, std::ios::end); const auto end = f.tellg();
    if (end <= 0) throw std::runtime_error("Input image is empty");
    const auto bytes = static_cast<std::uint64_t>(end);
    Track t{}; t.session=1; t.number=1; t.mode=1; t.sector_size=2048; t.user_offset=0;
    t.file_base=0; t.data_position=0; t.length=static_cast<std::uint32_t>(std::min<std::uint64_t>(bytes/2048u, 0xFFFFFFFFu));
    t.total_length=t.length; t.start_lba=0; t.pregap=0;
    DiscLayout d{}; d.tracks.push_back(t); return d;
}

void save(const std::filesystem::path& p, const std::vector<std::uint8_t>& b) {
    std::ofstream f(p, std::ios::binary); if (!f) throw std::runtime_error("Unable to create output");
    f.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
}

bool has(const std::vector<std::uint8_t>& b, std::string_view s) {
    return std::search(b.begin(), b.end(), s.begin(), s.end()) != b.end();
}

void save_disc_map(const std::filesystem::path& out, const std::filesystem::path& image,
                   const DiscLayout& layout, const BootTrack& bt) {
    std::ofstream f(out); if (!f) throw std::runtime_error("Unable to create disc map");
    std::error_code ec; const auto abs = std::filesystem::absolute(image, ec);
    f << "DCR_DISC_MAP_V2\n";
    f << "image=" << (ec ? image.string() : abs.string()) << "\n";
    f << "logical_sector_size=" << kLogicalSector << "\n";
    f << "boot_track=" << bt.track_index << "\n";
    f << "volume=" << bt.volume << "\n";
    f << "title=" << bt.title << "\n";
    f << "track_count=" << layout.tracks.size() << "\n";
    for (std::size_t i = 0; i < layout.tracks.size(); ++i) {
        const auto& t = layout.tracks[i];
        const std::string p = "track." + std::to_string(i) + ".";
        f << p << "session=" << t.session << "\n";
        f << p << "number=" << t.number << "\n";
        f << p << "mode=" << t.mode << "\n";
        f << p << "start_fad=" << t.start_fad() << "\n";
        f << p << "end_fad=" << t.end_fad() << "\n";
        f << p << "file_base=" << t.data_position << "\n";
        f << p << "sector_size=" << t.sector_size << "\n";
        f << p << "user_offset=" << t.user_offset << "\n";
        f << p << "user_size=" << (t.data() ? kLogicalSector : t.sector_size) << "\n";
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cout << "dc_disc_probe 0.1.1 <image.cdi|image.iso> [--extract-boot=FILE] [--extract-ip=FILE] [--extract-disc-map=FILE] [--extract-file=NAME=FILE]\n";
            return 1;
        }
        std::filesystem::path in = argv[1], boot_out, ip_out, map_out;
        std::vector<std::pair<std::string, std::filesystem::path>> file_outs;
        for (int i = 2; i < argc; ++i) {
            const std::string a = argv[i];
            if (a.rfind("--extract-boot=", 0) == 0) boot_out = a.substr(15);
            else if (a.rfind("--extract-ip=", 0) == 0) ip_out = a.substr(13);
            else if (a.rfind("--extract-disc-map=", 0) == 0) map_out = a.substr(19);
            else if (a.rfind("--extract-file=", 0) == 0) {
                const auto spec = a.substr(15);
                const auto eq = spec.find('=');
                if (eq == std::string::npos || eq == 0u || eq + 1u >= spec.size())
                    throw std::runtime_error("--extract-file expects NAME=FILE");
                file_outs.emplace_back(spec.substr(0, eq), std::filesystem::path(spec.substr(eq + 1u)));
            }
            else throw std::runtime_error("Unknown option: " + a);
        }

        std::ifstream image(in, std::ios::binary);
        if (!image) throw std::runtime_error("Unable to open input image");
        auto cdi = parse_cdi(image);
        DiscLayout layout = cdi ? *cdi : raw_2048_layout(image);
        std::optional<BootTrack> boot_track;
        for (std::size_t i = 0; i < layout.tracks.size(); ++i) {
            if (!layout.tracks[i].data()) continue;
            if (auto candidate = inspect_boot_track(image, layout, i)) { boot_track = std::move(candidate); break; }
        }
        if (!boot_track)
            throw std::runtime_error("No supported Dreamcast boot track/ISO9660 filesystem found");
        const auto& bt = *boot_track;
        const auto& t = layout.tracks[bt.track_index];

        image.clear(); image.seekg(0, std::ios::end); const auto end = image.tellg();
        std::cout << "DreamcastRecomp Disc Probe 0.1.1\n=================================\n"
                  << "Input:              " << in.string() << "\n"
                  << "Container bytes:    " << (end < 0 ? 0 : static_cast<std::uint64_t>(end)) << "\n"
                  << "Container:          " << (layout.from_cdi ? "DiscJuggler CDI" : "raw/ISO") << "\n"
                  << "Tracks:             " << layout.tracks.size() << "\n"
                  << "Boot track:         S" << t.session << " T" << t.number << "\n"
                  << "Physical sector:    " << t.sector_size << " bytes\n"
                  << "User-data offset:   " << t.user_offset << "\n"
                  << "Logical sector:     " << kLogicalSector << " bytes\n"
                  << "Track FAD range:    " << t.start_fad() << ".." << t.end_fad() << "\n"
                  << "Track data base:    0x" << std::hex << std::uppercase << t.data_position << std::dec << "\n"
                  << "ISO extent mode:    " << (bt.absolute_extents ? "absolute disc LBA" : "ISO-relative LBA") << "\n"
                  << "Volume ID:          " << bt.volume << "\n"
                  << "Root entries:       " << bt.root.size() << "\n\n"
                  << "IP.BIN\n"
                  << "  hardware:         " << trim(bt.ip.data(), 16) << "\n"
                  << "  maker:            " << trim(bt.ip.data() + 0x10, 16) << "\n"
                  << "  device:           " << trim(bt.ip.data() + 0x20, 16) << "\n"
                  << "  regions:          " << trim(bt.ip.data() + 0x30, 8) << "\n"
                  << "  product/version:  " << trim(bt.ip.data() + 0x40, 16) << "\n"
                  << "  release date:     " << trim(bt.ip.data() + 0x50, 8) << "\n"
                  << "  boot filename:    " << bt.boot_name << "\n"
                  << "  title:            " << bt.title << "\n\n"
                  << "Boot binary\n"
                  << "  image offset:     0x" << std::hex << bt.boot_offset << std::dec << "\n"
                  << "  ISO extent:       " << bt.boot_entry.extent << "\n"
                  << "  size:             " << bt.boot_entry.size << " bytes\n"
                  << "  load address:     0x8C010000\n";

        std::cout << "\nDisc track table:\n";
        for (const auto& track : layout.tracks) {
            std::cout << "  S" << track.session << " T" << track.number
                      << " " << (track.data() ? "data" : "audio")
                      << " fad=" << track.start_fad() << ".." << track.end_fad()
                      << " sectors=" << track.length
                      << " pregap=" << track.pregap
                      << " raw=" << track.sector_size
                      << " base=0x" << std::hex << std::uppercase << track.data_position << std::dec
                      << "\n";
        }

        constexpr std::array<std::string_view, 13> markers{
            "KAMUI Ver", "Ninja Ver", "Shinobi Ver", "gdFs Ver", "pd Ver", "pdKbd Ver", "pdLcd Ver",
            "pdTmr Ver", "pdVib Ver", "sd Ver", "bu Ver", "syStart", "syG2"};
        std::cout << "\nSDK markers:\n";
        bool any = false;
        for (const auto s : markers) if (has(bt.boot, s)) { std::cout << "  + " << s << "\n"; any = true; }
        if (!any) std::cout << "  (none)\n";
        std::cout << "\nFirst SH-4 words:";
        for (std::size_t i = 0; i < std::min<std::size_t>(8, bt.boot.size() / 2); ++i) {
            const auto w = std::uint16_t(bt.boot[i * 2]) | (std::uint16_t(bt.boot[i * 2 + 1]) << 8);
            std::cout << " 0x" << std::hex << std::setw(4) << std::setfill('0') << w;
        }
        std::cout << std::dec << "\n";
        if (!ip_out.empty()) { save(ip_out, bt.ip); std::cout << "[OK] IP.BIN -> " << ip_out.string() << "\n"; }
        if (!boot_out.empty()) { save(boot_out, bt.boot); std::cout << "[OK] " << bt.boot_name << " -> " << boot_out.string() << "\n"; }
        if (!map_out.empty()) { save_disc_map(map_out, in, layout, bt); std::cout << "[OK] Disc map V2 -> " << map_out.string() << "\n"; }
        for (const auto& [name, out_path] : file_outs) {
            const auto* entry = find_entry(bt.root, name);
            if (!entry || entry->dir)
                throw std::runtime_error("Root file not found: " + name);
            const auto data = read_iso_extent(image, t, bt, entry->extent, entry->size);
            if (data.size() != entry->size)
                throw std::runtime_error("Unable to read complete root file: " + name);
            save(out_path, data);
            std::cout << "[OK] " << entry->name << " -> " << out_path.string()
                      << " (" << data.size() << " bytes)\n";
        }
        std::cout << "\n[OK] Native Dreamcast commercial disc recognized.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[dc_disc_probe ERROR] " << e.what() << "\n";
        return 2;
    }
}
