#include "dcrecomp/sh4_decoder.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct Options {
    std::filesystem::path path;
    std::uint32_t base{0x8C010000u};
    std::uint32_t entry{0x8C010000u};
    std::size_t max_blocks{4096u};
    std::size_t max_instructions{250000u};
};

std::uint32_t parse_u32(const std::string& text) {
    std::size_t used = 0;
    const auto v = std::stoull(text, &used, 0);
    if (used != text.size() || v > 0xFFFFFFFFull) throw std::runtime_error("invalid 32-bit value: " + text);
    return static_cast<std::uint32_t>(v);
}

std::size_t parse_size(const std::string& text) {
    std::size_t used = 0;
    const auto v = std::stoull(text, &used, 0);
    if (used != text.size()) throw std::runtime_error("invalid size: " + text);
    return static_cast<std::size_t>(v);
}

Options parse(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "dc_raw_boot_probe 0.0.170 <BOOT.BIN> [--base=0x8C010000] [--entry=0x8C010000] "
                     "[--max-blocks=N] [--max-instructions=N]\n";
        std::exit(1);
    }
    Options o;
    o.path = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("--base=", 0) == 0) o.base = parse_u32(a.substr(7));
        else if (a.rfind("--entry=", 0) == 0) o.entry = parse_u32(a.substr(8));
        else if (a.rfind("--max-blocks=", 0) == 0) o.max_blocks = parse_size(a.substr(13));
        else if (a.rfind("--max-instructions=", 0) == 0) o.max_instructions = parse_size(a.substr(19));
        else throw std::runtime_error("unknown option: " + a);
    }
    return o;
}

std::vector<std::uint8_t> read_all(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("unable to open: " + p.string());
    const auto end = f.tellg();
    if (end <= 0) throw std::runtime_error("empty boot image");
    std::vector<std::uint8_t> b(static_cast<std::size_t>(end));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(b.data()), static_cast<std::streamsize>(b.size()));
    if (!f) throw std::runtime_error("error reading boot image");
    return b;
}

bool contains(const std::vector<std::uint8_t>& b, std::uint32_t base, std::uint32_t address, std::size_t size = 2u) {
    if (address < base) return false;
    const std::uint64_t off = static_cast<std::uint64_t>(address) - base;
    return off + size <= b.size();
}

std::uint16_t read16(const std::vector<std::uint8_t>& b, std::uint32_t base, std::uint32_t address) {
    if (!contains(b, base, address, 2u)) throw std::runtime_error("read16 out of boot image");
    const auto off = static_cast<std::size_t>(address - base);
    return static_cast<std::uint16_t>(b[off] | (static_cast<std::uint16_t>(b[off + 1]) << 8u));
}

std::uint32_t read32(const std::vector<std::uint8_t>& b, std::uint32_t base, std::uint32_t address) {
    if (!contains(b, base, address, 4u)) throw std::runtime_error("read32 out of boot image");
    const auto off = static_cast<std::size_t>(address - base);
    return static_cast<std::uint32_t>(b[off]) |
           (static_cast<std::uint32_t>(b[off + 1]) << 8u) |
           (static_cast<std::uint32_t>(b[off + 2]) << 16u) |
           (static_cast<std::uint32_t>(b[off + 3]) << 24u);
}

std::string hex8(std::uint32_t v) {
    std::ostringstream s;
    s << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << v;
    return s.str();
}

bool is_conditional(dcrecomp::sh4::Opcode op) {
    using O = dcrecomp::sh4::Opcode;
    return op == O::Bt || op == O::Bf || op == O::BtS || op == O::BfS;
}

bool is_terminal(dcrecomp::sh4::Opcode op) {
    using O = dcrecomp::sh4::Opcode;
    return op == O::Bra || op == O::Jmp || op == O::Rts || op == O::Rte;
}

struct Stats {
    std::size_t blocks{};
    std::size_t instructions{};
    std::size_t known{};
    std::size_t unknown{};
    std::size_t direct_calls{};
    std::size_t indirect_resolved{};
    std::size_t indirect_unresolved{};
    std::size_t out_of_image_edges{};
    std::set<std::uint32_t> discovered_targets;
    std::vector<std::uint32_t> unknown_addresses;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> indirect_edges;
};

void enqueue_if_local(std::queue<std::uint32_t>& pending,
                      std::unordered_set<std::uint32_t>& queued,
                      const std::vector<std::uint8_t>& bytes,
                      const Options& o,
                      Stats& stats,
                      std::uint32_t address) {
    if ((address & 1u) != 0u) address &= ~1u;
    stats.discovered_targets.insert(address);
    if (!contains(bytes, o.base, address)) {
        ++stats.out_of_image_edges;
        return;
    }
    if (queued.insert(address).second) pending.push(address);
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto o = parse(argc, argv);
        const auto bytes = read_all(o.path);
        if (!contains(bytes, o.base, o.entry)) throw std::runtime_error("entry lies outside boot image");

        std::queue<std::uint32_t> pending;
        std::unordered_set<std::uint32_t> queued;
        std::unordered_set<std::uint32_t> visited;
        Stats stats;
        enqueue_if_local(pending, queued, bytes, o, stats, o.entry);

        while (!pending.empty() && stats.blocks < o.max_blocks && stats.instructions < o.max_instructions) {
            const std::uint32_t block_start = pending.front();
            pending.pop();
            if (visited.contains(block_start)) continue;
            ++stats.blocks;

            std::array<std::optional<std::uint32_t>, 16> reg{};
            std::uint32_t pc = block_start;
            for (;;) {
                if (!contains(bytes, o.base, pc) || stats.instructions >= o.max_instructions) break;
                if (!visited.insert(pc).second) break;

                const auto ins = dcrecomp::sh4::decode(read16(bytes, o.base, pc), pc);
                ++stats.instructions;
                if (dcrecomp::sh4::is_known(ins)) ++stats.known;
                else {
                    ++stats.unknown;
                    if (stats.unknown_addresses.size() < 32u) stats.unknown_addresses.push_back(pc);
                }

                using O = dcrecomp::sh4::Opcode;
                switch (ins.opcode) {
                    case O::MovImm:
                        reg[ins.rn] = static_cast<std::uint32_t>(ins.immediate);
                        break;
                    case O::MovReg:
                        reg[ins.rn] = reg[ins.rm];
                        break;
                    case O::MovLPcRel:
                        if (contains(bytes, o.base, ins.effective_address, 4u)) reg[ins.rn] = read32(bytes, o.base, ins.effective_address);
                        else reg[ins.rn].reset();
                        break;
                    case O::MovWPcRel:
                        if (contains(bytes, o.base, ins.effective_address, 2u))
                            reg[ins.rn] = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(read16(bytes, o.base, ins.effective_address))));
                        else reg[ins.rn].reset();
                        break;
                    case O::Mova:
                        reg[0] = ins.effective_address;
                        break;
                    case O::AddImm:
                        if (reg[ins.rn]) *reg[ins.rn] = static_cast<std::uint32_t>(*reg[ins.rn] + ins.immediate);
                        break;
                    default:
                        break;
                }

                if (ins.opcode == O::Bsr) {
                    ++stats.direct_calls;
                    enqueue_if_local(pending, queued, bytes, o, stats, ins.target);
                } else if (is_conditional(ins.opcode)) {
                    enqueue_if_local(pending, queued, bytes, o, stats, ins.target);
                    const std::uint32_t fallthrough = pc + (ins.has_delay_slot ? 4u : 2u);
                    enqueue_if_local(pending, queued, bytes, o, stats, fallthrough);
                    if (ins.has_delay_slot && contains(bytes, o.base, pc + 2u)) {
                        const auto delay = dcrecomp::sh4::decode(read16(bytes, o.base, pc + 2u), pc + 2u);
                        if (visited.insert(pc + 2u).second) {
                            ++stats.instructions;
                            if (dcrecomp::sh4::is_known(delay)) ++stats.known;
                            else { ++stats.unknown; if (stats.unknown_addresses.size() < 32u) stats.unknown_addresses.push_back(pc + 2u); }
                        }
                    }
                    break;
                } else if (ins.opcode == O::Jsr || ins.opcode == O::Jmp) {
                    const auto target = reg[ins.rm];
                    if (target) {
                        ++stats.indirect_resolved;
                        stats.indirect_edges.push_back({pc, *target});
                        enqueue_if_local(pending, queued, bytes, o, stats, *target);
                    } else {
                        ++stats.indirect_unresolved;
                    }
                    if (ins.opcode == O::Jmp) {
                        if (ins.has_delay_slot && contains(bytes, o.base, pc + 2u)) visited.insert(pc + 2u);
                        break;
                    }
                    // A call can clobber caller-saved values; do not use stale constants.
                    for (auto& r : reg) r.reset();
                } else if (ins.opcode == O::Bsrf || ins.opcode == O::Braf) {
                    const auto add = reg[ins.rn];
                    if (add) {
                        const std::uint32_t target = pc + 4u + *add;
                        ++stats.indirect_resolved;
                        stats.indirect_edges.push_back({pc, target});
                        enqueue_if_local(pending, queued, bytes, o, stats, target);
                    } else ++stats.indirect_unresolved;
                    if (ins.opcode == O::Braf) break;
                }

                if (ins.opcode == O::Bra) {
                    enqueue_if_local(pending, queued, bytes, o, stats, ins.target);
                    if (ins.has_delay_slot && contains(bytes, o.base, pc + 2u)) visited.insert(pc + 2u);
                    break;
                }
                if (ins.opcode == O::Rts || ins.opcode == O::Rte) {
                    if (ins.has_delay_slot && contains(bytes, o.base, pc + 2u)) visited.insert(pc + 2u);
                    break;
                }
                pc += 2u;
            }
        }

        std::cout << "DreamcastRecomp Raw Boot Probe 0.0.170\n"
                     "======================================\n"
                  << "Input:               " << o.path.string() << "\n"
                  << "Image bytes:         " << bytes.size() << "\n"
                  << "Load base:           " << hex8(o.base) << "\n"
                  << "Entry:               " << hex8(o.entry) << "\n"
                  << "Reachable blocks:    " << stats.blocks << "\n"
                  << "Unique instructions: " << stats.instructions << "\n"
                  << "Known SH-4:          " << stats.known << "\n"
                  << "Unknown SH-4:        " << stats.unknown << "\n"
                  << "Direct calls:        " << stats.direct_calls << "\n"
                  << "Indirect resolved:   " << stats.indirect_resolved << "\n"
                  << "Indirect unresolved: " << stats.indirect_unresolved << "\n"
                  << "Local targets:       " << stats.discovered_targets.size() << "\n"
                  << "Out-of-image edges:  " << stats.out_of_image_edges << "\n";

        if (!stats.indirect_edges.empty()) {
            std::cout << "\nResolved indirect transfers (first 16)\n";
            std::size_t n = 0;
            for (const auto& [from, to] : stats.indirect_edges) {
                std::cout << "  " << hex8(from) << " -> " << hex8(to) << "\n";
                if (++n == 16u) break;
            }
        }
        if (!stats.unknown_addresses.empty()) {
            std::cout << "\nFirst unknown SH-4 addresses\n";
            for (const auto a : stats.unknown_addresses) std::cout << "  " << hex8(a) << "\n";
        }

        if (stats.blocks >= o.max_blocks || stats.instructions >= o.max_instructions)
            std::cout << "\n[LIMIT] Discovery stopped at configured safety limit.\n";
        if (stats.unknown == 0u)
            std::cout << "\n[OK] Reachable raw bootstrap sample is decoder-clean.\n";
        else
            std::cout << "\n[PARTIAL] Reachable raw bootstrap contains unsupported/ambiguous words.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[dc_raw_boot_probe ERROR] " << e.what() << "\n";
        return 2;
    }
}
