#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr std::size_t kMaxChunk = 2048u * 1024u;

struct DcRand {
    std::uint32_t seed{};
    explicit DcRand(std::uint32_t n) : seed(n & 0xFFFFu) {}
    std::uint32_t next() {
        seed = (seed * 2109u + 9273u) & 0x7FFFu;
        return (seed + 0xC000u) & 0xFFFFu;
    }
};

std::vector<std::uint8_t> read_all(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("unable to open: " + path.string());
    const auto end = f.tellg();
    if (end < 0) throw std::runtime_error("unable to size: " + path.string());
    std::vector<std::uint8_t> out(static_cast<std::size_t>(end));
    f.seekg(0);
    if (!out.empty()) f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
    if (!f && !out.empty()) throw std::runtime_error("read failed: " + path.string());
    return out;
}

void write_all(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("unable to create: " + path.string());
    if (!bytes.empty()) f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!f) throw std::runtime_error("write failed: " + path.string());
}

std::vector<std::uint8_t> descramble(const std::vector<std::uint8_t>& src) {
    std::vector<std::uint8_t> dst(src.size());
    DcRand rng(static_cast<std::uint32_t>(src.size()));
    std::size_t src_pos = 0;
    std::size_t dst_pos = 0;
    std::size_t remain = src.size();
    std::vector<std::size_t> idx(kMaxChunk / 32u);

    for (std::size_t chunk = kMaxChunk; chunk >= 32u; chunk >>= 1u) {
        while (remain >= chunk) {
            const std::size_t slices = chunk / 32u;
            for (std::size_t i = 0; i < slices; ++i) idx[i] = i;
            for (std::size_t n = slices; n-- > 0;) {
                const std::size_t x = (static_cast<std::uint64_t>(rng.next()) * n) >> 16u;
                std::swap(idx[n], idx[x]);
                std::copy_n(src.data() + src_pos, 32u, dst.data() + dst_pos + 32u * idx[n]);
                src_pos += 32u;
            }
            remain -= chunk;
            dst_pos += chunk;
        }
        if (chunk == 32u) break;
    }
    if (remain != 0u) std::copy_n(src.data() + src_pos, remain, dst.data() + dst_pos);
    return dst;
}

std::uint32_t fnv1a(const std::vector<std::uint8_t>& bytes) {
    std::uint32_t h = 2166136261u;
    for (const auto b : bytes) { h ^= b; h *= 16777619u; }
    return h;
}

enum class BootMode { Auto, GdRom, SelfBoot };

struct Options {
    std::filesystem::path ip;
    std::filesystem::path disc_boot;
    std::filesystem::path boot_out{"generated/commercial_recompiled/BOOT.BIN"};
    std::filesystem::path combined_out{"generated/commercial_recompiled/BOOTSTRAP.BIN"};
    BootMode mode{BootMode::Auto};
};

std::string trim_ascii(const std::uint8_t* p, std::size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

Options parse(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "dc_boot_prepare 0.0.170 <IP.BIN> <DISC_BOOT.BIN> [--mode=auto|gdrom|selfboot] [--boot-out=FILE] [--combined-out=FILE]\n";
        std::exit(1);
    }
    Options o;
    o.ip = argv[1];
    o.disc_boot = argv[2];
    for (int i = 3; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--boot-out=", 0) == 0) o.boot_out = a.substr(11);
        else if (a.rfind("--combined-out=", 0) == 0) o.combined_out = a.substr(15);
        else if (a == "--mode=auto") o.mode = BootMode::Auto;
        else if (a == "--mode=gdrom") o.mode = BootMode::GdRom;
        else if (a == "--mode=selfboot") o.mode = BootMode::SelfBoot;
        else throw std::runtime_error("unknown option: " + a);
    }
    return o;
}
} // namespace

int main(int argc, char** argv) {
    try {
        const auto o = parse(argc, argv);
        const auto ip = read_all(o.ip);
        const auto disc_boot = read_all(o.disc_boot);
        if (ip.size() != 32768u) throw std::runtime_error("IP.BIN must be exactly 32768 bytes for the console bootstrap image");
        if (disc_boot.empty()) throw std::runtime_error("boot image is empty");

        const std::string device = ip.size() >= 0x30u ? trim_ascii(ip.data() + 0x20u, 16u) : std::string{};
        BootMode resolved = o.mode;
        if (resolved == BootMode::Auto) {
            // Retail GD-ROM images carry an already executable 1ST_READ.BIN.
            // Scrambling is the MIL-CD/self-boot convention, not a universal
            // Dreamcast executable transform.  The IP.BIN device field gives us
            // a stable distinction for the normal console cases.
            resolved = device.find("GD-ROM") != std::string::npos ? BootMode::GdRom : BootMode::SelfBoot;
        }
        const bool transformed = resolved == BootMode::SelfBoot;
        const auto boot = transformed ? descramble(disc_boot) : disc_boot;
        std::vector<std::uint8_t> combined;
        combined.reserve(ip.size() + boot.size());
        combined.insert(combined.end(), ip.begin(), ip.end());
        combined.insert(combined.end(), boot.begin(), boot.end());
        write_all(o.boot_out, boot);
        write_all(o.combined_out, combined);

        std::cout << "DreamcastRecomp Commercial Boot Prepare 0.0.170\n"
                     "==============================================\n"
                  << "IP.BIN bytes:          " << ip.size() << "\n"
                  << "Device field:          " << device << "\n"
                  << "Boot mode:             " << (resolved == BootMode::GdRom ? "GD-ROM / pass-through" : "MIL-CD selfboot / descramble") << "\n"
                  << "Disc boot bytes:       " << disc_boot.size() << "\n"
                  << "Prepared boot bytes:   " << boot.size() << "\n"
                  << "Combined image bytes:  " << combined.size() << "\n"
                  << "Combined base:         0x8C008000\n"
                  << "Bootstrap entry:       0x8C008300 (P1 canonical of BIOS P2 0xAC008300)\n"
                  << "Game load address:     0x8C010000\n"
                  << "Disc boot FNV1a:       0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << fnv1a(disc_boot) << "\n"
                  << "Prepared FNV1a:        0x" << std::setw(8) << fnv1a(boot) << std::dec << "\n"
                  << "BOOT.BIN:              " << o.boot_out.string() << "\n"
                  << "BOOTSTRAP.BIN:         " << o.combined_out.string() << "\n\n"
                  << (transformed
                        ? "[OK] Self-boot executable descrambled and IP.BIN bootstrap prepended.\n"
                        : "[OK] Retail GD-ROM executable preserved byte-for-byte and IP.BIN bootstrap prepended.\n");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[dc_boot_prepare ERROR] " << e.what() << "\n";
        return 2;
    }
}
