#include "dcrecomp/cpp_emitter.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"
#include "dcrecomp/program_analysis.hpp"
#include "dcrecomp/dcir.hpp"
#include "dcrecomp/sh4_decoder.hpp"
#include "dcrecomp/raw_data.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
    std::filesystem::path path;
    std::filesystem::path output{"generated/commercial_raw"};
    std::filesystem::path map_path;
    std::uint32_t base{0x8C010000u};
    std::uint32_t entry{0x8C010000u};
    std::size_t max_functions{8192u};
    std::size_t max_closure_entries{0u}; // 0 = reserve automatic headroom for final ProgramAnalysis
    std::size_t closure_passes{32u};
    std::vector<std::uint32_t> seed_entries;
    std::vector<std::filesystem::path> seed_files;
    bool no_emit{};
};

std::uint32_t parse_u32(const std::string& text) {
    std::size_t used = 0;
    const auto value = std::stoull(text, &used, 0);
    if (used != text.size() || value > 0xFFFFFFFFull) throw std::runtime_error("invalid 32-bit value: " + text);
    return static_cast<std::uint32_t>(value);
}

std::size_t parse_size(const std::string& text) {
    std::size_t used = 0;
    const auto value = std::stoull(text, &used, 0);
    if (used != text.size()) throw std::runtime_error("invalid size: " + text);
    return static_cast<std::size_t>(value);
}

Options parse(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "dc_raw_recomp 0.1.0 <BOOT.BIN> [--base=0x8C010000] [--entry=0x8C010000] "
                     "[--seed=ADDR ...] [--seed-file=FILE ...] [--output=DIR] [--map=FILE] [--max-functions=N] [--max-closure-entries=N] [--closure-passes=N] [--no-emit]\n";
        std::exit(1);
    }
    Options o;
    o.path = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("--base=", 0) == 0) o.base = parse_u32(a.substr(7));
        else if (a.rfind("--entry=", 0) == 0) o.entry = parse_u32(a.substr(8));
        else if (a.rfind("--output=", 0) == 0) o.output = a.substr(9);
        else if (a.rfind("--map=", 0) == 0) o.map_path = a.substr(6);
        else if (a.rfind("--max-functions=", 0) == 0) o.max_functions = parse_size(a.substr(16));
        else if (a.rfind("--max-closure-entries=", 0) == 0) o.max_closure_entries = parse_size(a.substr(22));
        else if (a.rfind("--closure-passes=", 0) == 0) o.closure_passes = parse_size(a.substr(17));
        else if (a.rfind("--seed=", 0) == 0) o.seed_entries.push_back(parse_u32(a.substr(7)));
        else if (a.rfind("--seed-file=", 0) == 0) o.seed_files.emplace_back(a.substr(12));
        else if (a == "--no-emit") o.no_emit = true;
        else throw std::runtime_error("unknown option: " + a);
    }
    for (const auto& seed_file : o.seed_files) {
        std::ifstream f(seed_file);
        if (!f) throw std::runtime_error("unable to open seed file: " + seed_file.string());
        std::string line;
        std::size_t line_no = 0u;
        while (std::getline(f, line)) {
            ++line_no;
            if (const auto hash = line.find('#'); hash != std::string::npos) line.resize(hash);
            const auto first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            const auto last = line.find_last_not_of(" \t\r\n");
            const auto token = line.substr(first, last - first + 1u);
            try {
                o.seed_entries.push_back(parse_u32(token));
            } catch (const std::exception& e) {
                throw std::runtime_error("invalid seed in " + seed_file.string() + ":" +
                                         std::to_string(line_no) + ": " + e.what());
            }
        }
    }
    std::sort(o.seed_entries.begin(), o.seed_entries.end());
    o.seed_entries.erase(std::unique(o.seed_entries.begin(), o.seed_entries.end()), o.seed_entries.end());
    if (o.max_functions == 0u || o.closure_passes == 0u) throw std::runtime_error("limits must be > 0");
    if (o.max_closure_entries != 0u && o.max_closure_entries >= o.max_functions)
        throw std::runtime_error("--max-closure-entries must be smaller than --max-functions");
    return o;
}

std::size_t closure_entry_limit(const Options& o) {
    if (o.max_closure_entries != 0u) return o.max_closure_entries;
    // The raw seed closure and final ProgramAnalysis used to share one hard
    // ceiling.  If raw discovery filled that ceiling exactly, merely seeding the
    // final pass left no room for cross-symbol CFG fragments and ProgramAnalysis
    // failed with "--max-functions" even when the true executable closure was
    // much smaller. Reserve 25%% (at least 1024 entries) for the authoritative
    // final pass. This also contains accidental data/table expansion.
    const std::size_t reserve = std::max<std::size_t>(1024u, o.max_functions / 4u);
    return o.max_functions > reserve ? o.max_functions - reserve : std::max<std::size_t>(1u, o.max_functions / 2u);
}

std::vector<std::uint8_t> read_all(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("unable to open: " + path.string());
    const auto end = f.tellg();
    if (end <= 0) throw std::runtime_error("empty raw image");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!f) throw std::runtime_error("error reading raw image");
    return bytes;
}

std::string hex8(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
    return out.str();
}

std::string synthetic_name(std::uint32_t address) {
    std::ostringstream out;
    out << "sub_" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << address;
    return out.str();
}

bool local_address(std::uint32_t base, std::size_t size, std::uint32_t address) {
    if ((address & 1u) != 0u || address < base) return false;
    return static_cast<std::uint64_t>(address - base) + 2u <= size;
}

std::optional<std::uint32_t> canonical_local_address(std::uint32_t base,
                                                      std::size_t size,
                                                      std::uint32_t address) {
    if ((address & 1u) != 0u) return std::nullopt;
    if (local_address(base, size, address)) return address;

    // Katana startup code deliberately executes cache-sensitive helpers through
    // the SH-4 P2 (uncached) alias. The flat image is normally mapped at its P1
    // address (0x8C...), so fold P1/P2 aliases to the image's canonical alias
    // before deciding that a resolved call is external.
    const std::uint32_t base_phys = base & 0x1FFFFFFFu;
    const std::uint32_t addr_phys = address & 0x1FFFFFFFu;
    if (addr_phys < base_phys || static_cast<std::uint64_t>(addr_phys - base_phys) + 2u > size) {
        return std::nullopt;
    }
    const std::uint32_t canonical = (base & 0xE0000000u) | addr_phys;
    if (!local_address(base, size, canonical)) return std::nullopt;
    return canonical;
}



std::uint16_t raw16_at(const std::vector<std::uint8_t>& bytes, std::uint32_t base, std::uint32_t address) {
    const auto off = static_cast<std::size_t>(address - base);
    return static_cast<std::uint16_t>(bytes[off]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[off + 1u]) << 8u);
}

bool looks_like_code_entry(const std::vector<std::uint8_t>& bytes, std::uint32_t base, std::uint32_t address) {
    if (!local_address(base, bytes.size(), address)) return false;
    std::size_t known = 0u;
    bool has_return_or_branch = false;
    const std::uint64_t end64 = std::min<std::uint64_t>(
        static_cast<std::uint64_t>(address) + 0x80u,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto end = static_cast<std::uint32_t>(end64);
    for (std::uint32_t pc = address; pc + 2u <= end; pc += 2u) {
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (dcrecomp::sh4::is_known(d)) ++known;
        if (d.opcode == dcrecomp::sh4::Opcode::Rts ||
            d.opcode == dcrecomp::sh4::Opcode::Bra ||
            d.opcode == dcrecomp::sh4::Opcode::Jmp) {
            has_return_or_branch = true;
        }
        if (known >= 8u && has_return_or_branch) return true;
    }
    return false;
}

bool raw_instruction_writes_register(const dcrecomp::sh4::Instruction& i, std::uint8_t reg) {
    using dcrecomp::sh4::Opcode;
    switch (i.opcode) {
        // Integer/logical operations with an explicit GPR destination.
        case Opcode::MovImm: case Opcode::MovReg:
        case Opcode::AddImm: case Opcode::AddReg: case Opcode::Addc: case Opcode::Addv:
        case Opcode::SubReg: case Opcode::Subc: case Opcode::Subv:
        case Opcode::Neg: case Opcode::Negc:
        case Opcode::AndReg: case Opcode::XorReg: case Opcode::OrReg: case Opcode::Not:
        case Opcode::Dt: case Opcode::Div1:
        case Opcode::SwapB: case Opcode::SwapW: case Opcode::Xtrct:
        case Opcode::Shll: case Opcode::Shlr: case Opcode::Shal: case Opcode::Shar:
        case Opcode::Shll2: case Opcode::Shlr2: case Opcode::Shll8: case Opcode::Shlr8:
        case Opcode::Shll16: case Opcode::Shlr16:
        case Opcode::Rotl: case Opcode::Rotr: case Opcode::Rotcl: case Opcode::Rotcr:
        case Opcode::Shld: case Opcode::Shad:
        case Opcode::ExtuB: case Opcode::ExtuW: case Opcode::ExtsB: case Opcode::ExtsW:
        case Opcode::MovBLoad: case Opcode::MovWLoad: case Opcode::MovLLoad:
        case Opcode::MovBDispLoad: case Opcode::MovWDispLoad: case Opcode::MovLDispLoad:
        case Opcode::MovBIndexedLoad: case Opcode::MovWIndexedLoad: case Opcode::MovLIndexedLoad:
        case Opcode::MovWPcRel: case Opcode::MovLPcRel: case Opcode::Movt:
        case Opcode::StsPr: case Opcode::StsMach: case Opcode::StsMacl:
        case Opcode::StsFpul: case Opcode::StsFpscr:
        case Opcode::StcSr: case Opcode::StcGbr: case Opcode::StcVbr: case Opcode::StcSsr:
        case Opcode::StcSpc: case Opcode::StcSgr: case Opcode::StcDbr: case Opcode::StcBank:
            return i.rn == reg;

        // Immediate logical instructions are architecturally R0-only.
        case Opcode::AndImm: case Opcode::XorImm: case Opcode::OrImm:
            return reg == 0u;

        case Opcode::MovBPostinc: case Opcode::MovWPostinc: case Opcode::MovLPostinc:
            return i.rn == reg || i.rm == reg;
        case Opcode::MovBPredec: case Opcode::MovWPredec: case Opcode::MovLPredec:
        case Opcode::StsLPr: case Opcode::StsLMach: case Opcode::StsLMacl:
        case Opcode::StsLFpul: case Opcode::StsLFpscr:
        case Opcode::StcLSr: case Opcode::StcLGbr: case Opcode::StcLVbr: case Opcode::StcLSsr:
        case Opcode::StcLSpc: case Opcode::StcLSgr: case Opcode::StcLDbr: case Opcode::StcLBank:
            return i.rn == reg;
        case Opcode::Mova:
            return reg == 0u;
        default:
            return false;
    }
}

bool preserved_register_is_invoked_soon(const std::vector<std::uint8_t>& bytes,
                                        std::uint32_t base,
                                        std::uint32_t load_address,
                                        std::uint8_t reg) {
    const std::uint64_t end64 = std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + 0x100u,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto end = static_cast<std::uint32_t>(end64);
    for (std::uint32_t pc = load_address + 2u; pc + 2u <= end; pc += 2u) {
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if ((d.opcode == dcrecomp::sh4::Opcode::Jsr || d.opcode == dcrecomp::sh4::Opcode::Jmp) &&
            d.rm == reg) {
            return true;
        }
        if (raw_instruction_writes_register(d, reg)) return false;
        if (d.opcode == dcrecomp::sh4::Opcode::Bra ||
            d.opcode == dcrecomp::sh4::Opcode::Braf ||
            d.opcode == dcrecomp::sh4::Opcode::Jmp ||
            d.opcode == dcrecomp::sh4::Opcode::Rts ||
            d.opcode == dcrecomp::sh4::Opcode::Rte) {
            return false;
        }
    }
    return false;
}

bool preserved_register_is_invoked_on_local_path(const std::vector<std::uint8_t>& bytes,
                                                  std::uint32_t base,
                                                  std::uint32_t load_address,
                                                  std::uint8_t reg) {
    // R8-R14 are callee-saved in the SH-4 ABI. Retail state selectors often load
    // several function pointers into those registers, branch on a mode value, and
    // invoke only the pointer selected by the taken path. A linear byte scan sees
    // an unrelated BRA from another selector arm and gives up even though another
    // reachable arm later executes JSR @Rn. Explore a deliberately tiny local CFG
    // instead: the value must survive every instruction on the successful path,
    // all direct branches must remain near the defining load, and the proof ends
    // only at an actual JSR/JMP through the same register.
    if (reg < 8u || reg > 14u) return false;
    constexpr std::uint32_t kMaxSpan = 0x100u;
    constexpr std::size_t kMaxStates = 192u;
    const auto region_begin = load_address + 2u;
    const auto region_end64 = std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + kMaxSpan,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto region_end = static_cast<std::uint32_t>(region_end64);

    std::vector<std::uint32_t> pending{region_begin};
    std::set<std::uint32_t> visited;
    auto enqueue = [&](std::uint32_t pc) {
        if (pc < region_begin || pc + 2u > region_end || (pc & 1u) != 0u) return;
        if (!visited.contains(pc)) pending.push_back(pc);
    };

    while (!pending.empty() && visited.size() < kMaxStates) {
        const auto pc = pending.back();
        pending.pop_back();
        if (!visited.insert(pc).second) continue;
        if (!local_address(base, bytes.size(), pc)) continue;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) continue;

        if ((d.opcode == dcrecomp::sh4::Opcode::Jsr || d.opcode == dcrecomp::sh4::Opcode::Jmp) &&
            d.rm == reg) return true;
        if (raw_instruction_writes_register(d, reg)) continue;

        using dcrecomp::sh4::Opcode;
        const bool has_delay_slot = d.opcode == Opcode::Bra || d.opcode == Opcode::Bsr ||
                                    d.opcode == Opcode::BtS || d.opcode == Opcode::BfS ||
                                    d.opcode == Opcode::Jsr || d.opcode == Opcode::Jmp ||
                                    d.opcode == Opcode::Bsrf || d.opcode == Opcode::Braf ||
                                    d.opcode == Opcode::Rts || d.opcode == Opcode::Rte;
        if (has_delay_slot) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc) || slot_pc + 2u > region_end) continue;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            if (!dcrecomp::sh4::is_known(slot) || raw_instruction_writes_register(slot, reg)) continue;
        }

        switch (d.opcode) {
            case Opcode::Bra: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (target) enqueue(*target);
                break;
            }
            case Opcode::Bt:
            case Opcode::Bf: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (target) enqueue(*target);
                enqueue(pc + 2u);
                break;
            }
            case Opcode::BtS:
            case Opcode::BfS: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (target) enqueue(*target);
                enqueue(pc + 4u);
                break;
            }
            case Opcode::Bsr:
            case Opcode::Bsrf:
            case Opcode::Jsr:
                // Calls may clobber caller-saved registers but must preserve R8-R14.
                // Continue after the delay slot without trying to enter the callee.
                enqueue(pc + 4u);
                break;
            case Opcode::Jmp:
            case Opcode::Braf:
            case Opcode::Rts:
            case Opcode::Rte:
                break;
            default:
                enqueue(pc + 2u);
                break;
        }
    }
    return false;
}

bool literal_register_is_invoked_immediately(const std::vector<std::uint8_t>& bytes,
                                            std::uint32_t base,
                                            std::uint32_t load_address,
                                            std::uint8_t reg) {
    // Canonical raw-binary callback idiom: MOV.L @(disp,PC),Rn followed by a
    // nearby JSR/JMP @Rn. Retail compilers also commonly share the actual invoke
    // block between adjacent selector cases:
    //
    //     MOV.L callback_a,Rn
    //     BRA   invoke
    //      NOP
    //     MOV.L callback_b,Rn
    //   invoke:
    //     JSR   @Rn
    //
    // Follow at most one short, forward BRA while proving that neither its delay
    // slot nor any setup instruction redefines Rn. This recovers the shared-tail
    // form without turning the detector into general control-flow propagation.
    constexpr std::uint32_t kLinearWindow = 0x0Au;
    constexpr std::uint32_t kMaxForwardBraDistance = 0x28u;
    constexpr std::uint32_t kPostBraInstructions = 6u;

    std::uint32_t pc = load_address + 2u;
    std::uint32_t linear_end = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + kLinearWindow,
        static_cast<std::uint64_t>(base) + bytes.size()));
    bool followed_bra = false;
    std::uint32_t post_bra_left = 0u;

    while (pc + 2u <= linear_end) {
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if ((d.opcode == dcrecomp::sh4::Opcode::Jsr || d.opcode == dcrecomp::sh4::Opcode::Jmp) &&
            d.rm == reg) return true;

        // Do not carry a literal value through a reassignment.
        if (raw_instruction_writes_register(d, reg)) return false;

        if (d.opcode == dcrecomp::sh4::Opcode::Bra) {
            if (followed_bra) return false;
            const auto target = canonical_local_address(base, bytes.size(), d.target);
            if (!target || *target <= pc || *target - load_address > kMaxForwardBraDistance) return false;

            // BRA has a delay slot. It executes before control reaches target, so
            // the callback register must survive it unchanged as well.
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc)) return false;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            if (!dcrecomp::sh4::is_known(slot) || raw_instruction_writes_register(slot, reg)) return false;

            followed_bra = true;
            post_bra_left = kPostBraInstructions;
            pc = *target;
            linear_end = static_cast<std::uint32_t>(std::min<std::uint64_t>(
                static_cast<std::uint64_t>(*target) + kPostBraInstructions * 2u,
                static_cast<std::uint64_t>(base) + bytes.size()));
            continue;
        }

        if (d.opcode == dcrecomp::sh4::Opcode::Bsr ||
            d.opcode == dcrecomp::sh4::Opcode::Bsrf || d.opcode == dcrecomp::sh4::Opcode::Rts ||
            d.opcode == dcrecomp::sh4::Opcode::Rte) return false;

        pc += 2u;
        if (followed_bra && post_bra_left != 0u) {
            --post_bra_left;
            if (post_bra_left == 0u) return false;
        }
    }
    return false;
}

// 0.0.170: prove literal function-pointer provenance through a tiny local CFG.
// Unlike the legacy immediate detector this follows MOV Rm,Rn copies and short
// branch joins for *all* GPRs, while clearing any carrier register that is
// overwritten. Calls only preserve the SH-4 ABI callee-saved R8-R14 set.  This
// makes address-taken Katana SDK helpers discoverable without scanning arbitrary
// data as code or inventing values at runtime.
bool literal_value_reaches_indirect_call_on_local_path(const std::vector<std::uint8_t>& bytes,
                                                       std::uint32_t base,
                                                       std::uint32_t load_address,
                                                       std::uint8_t initial_reg) {
    if (initial_reg > 14u) return false;
    constexpr std::uint32_t kMaxSpan = 0x180u;
    constexpr std::size_t kMaxStates = 384u;
    const std::uint32_t region_begin = load_address + 2u;
    const auto region_end = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + kMaxSpan,
        static_cast<std::uint64_t>(base) + bytes.size()));

    using State = std::pair<std::uint32_t, std::uint16_t>;
    std::vector<State> pending{{region_begin, static_cast<std::uint16_t>(1u << initial_reg)}};
    std::set<State> visited;

    auto transfer = [&](const dcrecomp::sh4::Instruction& insn, std::uint16_t mask) {
        const bool mov_copy = insn.opcode == dcrecomp::sh4::Opcode::MovReg;
        const bool source_has_value = mov_copy && insn.rm <= 14u && (mask & (1u << insn.rm)) != 0u;
        for (std::uint8_t r = 0u; r <= 14u; ++r) {
            if (raw_instruction_writes_register(insn, r))
                mask = static_cast<std::uint16_t>(mask & ~(1u << r));
        }
        if (source_has_value && insn.rn <= 14u)
            mask = static_cast<std::uint16_t>(mask | (1u << insn.rn));
        return mask;
    };
    auto enqueue = [&](std::uint32_t pc, std::uint16_t mask) {
        if (mask == 0u || pc < region_begin || pc + 2u > region_end || (pc & 1u) != 0u) return;
        const State st{pc, mask};
        if (!visited.contains(st)) pending.push_back(st);
    };

    while (!pending.empty() && visited.size() < kMaxStates) {
        const auto [pc, incoming] = pending.back();
        pending.pop_back();
        if (!visited.insert({pc, incoming}).second) continue;
        if (!local_address(base, bytes.size(), pc)) continue;

        const auto insn = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(insn)) continue;
        if ((insn.opcode == dcrecomp::sh4::Opcode::Jsr || insn.opcode == dcrecomp::sh4::Opcode::Jmp) &&
            insn.rm <= 14u && (incoming & (1u << insn.rm)) != 0u) {
            return true;
        }

        std::uint16_t mask = transfer(insn, incoming);
        using dcrecomp::sh4::Opcode;
        const bool delayed = insn.opcode == Opcode::Bra || insn.opcode == Opcode::Bsr ||
                             insn.opcode == Opcode::BtS || insn.opcode == Opcode::BfS ||
                             insn.opcode == Opcode::Jsr || insn.opcode == Opcode::Jmp ||
                             insn.opcode == Opcode::Bsrf || insn.opcode == Opcode::Braf ||
                             insn.opcode == Opcode::Rts || insn.opcode == Opcode::Rte;
        if (delayed) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc) || slot_pc + 2u > region_end) continue;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            if (!dcrecomp::sh4::is_known(slot)) continue;
            mask = transfer(slot, mask);
        }
        if (mask == 0u) continue;

        switch (insn.opcode) {
            case Opcode::Bra: {
                if (const auto target = canonical_local_address(base, bytes.size(), insn.target)) enqueue(*target, mask);
                break;
            }
            case Opcode::Bt:
            case Opcode::Bf: {
                if (const auto target = canonical_local_address(base, bytes.size(), insn.target)) enqueue(*target, mask);
                enqueue(pc + 2u, mask);
                break;
            }
            case Opcode::BtS:
            case Opcode::BfS: {
                if (const auto target = canonical_local_address(base, bytes.size(), insn.target)) enqueue(*target, mask);
                enqueue(pc + 4u, mask);
                break;
            }
            case Opcode::Bsr:
            case Opcode::Bsrf:
            case Opcode::Jsr:
                // R0-R7 are caller-saved. R8-R14 retain a proven literal value.
                mask = static_cast<std::uint16_t>(mask & 0x7F00u);
                enqueue(pc + 4u, mask);
                break;
            case Opcode::Jmp:
            case Opcode::Braf:
            case Opcode::Rts:
            case Opcode::Rte:
                break;
            default:
                enqueue(pc + 2u, mask);
                break;
        }
    }
    return false;
}

bool literal_register_is_stored_soon(const std::vector<std::uint8_t>& bytes,
                                     std::uint32_t base,
                                     std::uint32_t load_address,
                                     std::uint8_t reg) {
    // SDK driver/interface initializers commonly materialize a callback address
    // from a PC-relative literal and immediately store it into an ops table.
    // That function may never have a direct call edge in the raw image. Keep the
    // dataflow deliberately local: the loaded register must reach a 32-bit store
    // unchanged within a handful of instructions.
    const std::uint64_t end64 = std::min<std::uint64_t>(
        // A compact Katana registration descriptor can interleave a few scalar
        // field stores between loading the callback literal and committing it
        // into the structure (Lodoss: 0x8C03CE4C -> store at +0x0E). Keep this
        // bounded to eight following SH-4 instructions and still require the
        // exact register to survive unchanged.
        static_cast<std::uint64_t>(load_address) + 0x10u,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto end = static_cast<std::uint32_t>(end64);
    for (std::uint32_t pc = load_address + 2u; pc + 2u <= end; pc += 2u) {
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if ((d.opcode == dcrecomp::sh4::Opcode::MovLStore ||
             d.opcode == dcrecomp::sh4::Opcode::MovLDispStore ||
             d.opcode == dcrecomp::sh4::Opcode::MovLIndexedStore) &&
            d.rm == reg) return true;

        const bool writes_reg = d.rn == reg && (
            d.opcode == dcrecomp::sh4::Opcode::MovImm || d.opcode == dcrecomp::sh4::Opcode::MovReg ||
            d.opcode == dcrecomp::sh4::Opcode::MovLPcRel || d.opcode == dcrecomp::sh4::Opcode::MovWPcRel ||
            d.opcode == dcrecomp::sh4::Opcode::MovLLoad || d.opcode == dcrecomp::sh4::Opcode::MovLDispLoad ||
            d.opcode == dcrecomp::sh4::Opcode::MovLIndexedLoad || d.opcode == dcrecomp::sh4::Opcode::MovLPostinc);
        if (writes_reg) return false;

        // Delayed control transfers execute their following instruction before
        // changing PC. Katana callback installers commonly exploit that slot to
        // commit a literal function pointer into an ops table:
        //
        //     MOV.L callback,Rn
        //     BRA   shared_tail
        //      MOV.L Rn,@Rm
        //
        // The old linear detector stopped at BRA before examining the delay slot,
        // so a callback that was unambiguously stored at runtime could remain out
        // of the static closure (ChuChu gameplay first exposed this at 0x8C0E5AD2).
        // Inspect exactly one architectural delay slot and keep the same short
        // dataflow proof: Rn must be stored unchanged, never guessed from nearby
        // data.
        const bool delayed_transfer = d.opcode == dcrecomp::sh4::Opcode::Bra ||
            d.opcode == dcrecomp::sh4::Opcode::Bsr || d.opcode == dcrecomp::sh4::Opcode::Bsrf ||
            d.opcode == dcrecomp::sh4::Opcode::Jsr || d.opcode == dcrecomp::sh4::Opcode::Jmp ||
            d.opcode == dcrecomp::sh4::Opcode::Braf || d.opcode == dcrecomp::sh4::Opcode::Rts ||
            d.opcode == dcrecomp::sh4::Opcode::Rte || d.opcode == dcrecomp::sh4::Opcode::BtS ||
            d.opcode == dcrecomp::sh4::Opcode::BfS;
        if (delayed_transfer) {
            const auto slot_pc = pc + 2u;
            if (local_address(base, bytes.size(), slot_pc)) {
                const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
                if (dcrecomp::sh4::is_known(slot) &&
                    (slot.opcode == dcrecomp::sh4::Opcode::MovLStore ||
                     slot.opcode == dcrecomp::sh4::Opcode::MovLDispStore ||
                     slot.opcode == dcrecomp::sh4::Opcode::MovLIndexedStore) &&
                    slot.rm == reg) return true;
            }
            return false;
        }
    }
    return false;
}

bool argument_register_reaches_call_soon(const std::vector<std::uint8_t>& bytes,
                                        std::uint32_t base,
                                        std::uint32_t load_address,
                                        std::uint8_t reg) {
    const std::uint64_t end64 = std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + 0x20u,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto end = static_cast<std::uint32_t>(end64);
    for (std::uint32_t pc = load_address + 2u; pc + 2u <= end; pc += 2u) {
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (d.opcode == dcrecomp::sh4::Opcode::Bsr || d.opcode == dcrecomp::sh4::Opcode::Bsrf ||
            d.opcode == dcrecomp::sh4::Opcode::Jsr || d.opcode == dcrecomp::sh4::Opcode::Jmp) return true;
        // Stop if the argument register is overwritten before the call. Cover the
        // common register-writing forms used by GCC/Katana around call setup.
        const bool writes_rn = d.rn == reg && (
            d.opcode == dcrecomp::sh4::Opcode::MovImm || d.opcode == dcrecomp::sh4::Opcode::MovReg ||
            d.opcode == dcrecomp::sh4::Opcode::MovLPcRel || d.opcode == dcrecomp::sh4::Opcode::MovWPcRel ||
            d.opcode == dcrecomp::sh4::Opcode::MovLLoad || d.opcode == dcrecomp::sh4::Opcode::MovLDispLoad ||
            d.opcode == dcrecomp::sh4::Opcode::MovLIndexedLoad || d.opcode == dcrecomp::sh4::Opcode::MovLPostinc);
        if (writes_rn) return false;
        if (d.opcode == dcrecomp::sh4::Opcode::Bra ||
            d.opcode == dcrecomp::sh4::Opcode::Rts || d.opcode == dcrecomp::sh4::Opcode::Rte) return false;
    }
    return false;
}

bool argument_register_reaches_call_on_local_path(const std::vector<std::uint8_t>& bytes,
                                                   std::uint32_t base,
                                                   std::uint32_t load_address,
                                                   std::uint8_t reg) {
    // R4-R7 are SH-4 argument registers. Retail state/object builders may load a
    // callback into one of them, perform a sizeable branch-local structure copy,
    // then tail into a shared registration call. A short linear scan misses that
    // pattern (ChuChu gameplay exposed a R5 load at 0x8C0198B8 which branches to
    // the common JSR at 0x8C019988). Explore only a small local CFG and accept the
    // value when an actual call/tail-call is reached without redefining the
    // argument register on that path.
    if (reg < 4u || reg > 7u) return false;
    constexpr std::uint32_t kMaxSpan = 0x100u;
    constexpr std::size_t kMaxStates = 192u;
    const auto region_begin = load_address + 2u;
    const auto region_end64 = std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + kMaxSpan,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto region_end = static_cast<std::uint32_t>(region_end64);

    std::vector<std::uint32_t> pending{region_begin};
    std::set<std::uint32_t> visited;
    auto enqueue = [&](std::uint32_t pc) {
        if (pc < region_begin || pc + 2u > region_end || (pc & 1u) != 0u) return;
        if (!visited.contains(pc)) pending.push_back(pc);
    };

    while (!pending.empty() && visited.size() < kMaxStates) {
        const auto pc = pending.back();
        pending.pop_back();
        if (!visited.insert(pc).second) continue;
        if (!local_address(base, bytes.size(), pc)) continue;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) continue;
        if (raw_instruction_writes_register(d, reg)) continue;

        using dcrecomp::sh4::Opcode;
        const bool delayed = d.opcode == Opcode::Bra || d.opcode == Opcode::Bsr ||
            d.opcode == Opcode::BtS || d.opcode == Opcode::BfS || d.opcode == Opcode::Jsr ||
            d.opcode == Opcode::Jmp || d.opcode == Opcode::Bsrf || d.opcode == Opcode::Braf ||
            d.opcode == Opcode::Rts || d.opcode == Opcode::Rte;
        if (delayed) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc) || slot_pc + 2u > region_end) continue;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            if (!dcrecomp::sh4::is_known(slot) || raw_instruction_writes_register(slot, reg)) continue;
        }

        // The unchanged R4-R7 value is live as a call argument at this point.
        if (d.opcode == Opcode::Bsr || d.opcode == Opcode::Bsrf || d.opcode == Opcode::Jsr ||
            d.opcode == Opcode::Jmp) return true;

        switch (d.opcode) {
            case Opcode::Bra: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (target) enqueue(*target);
                break;
            }
            case Opcode::Bt:
            case Opcode::Bf: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (target) enqueue(*target);
                enqueue(pc + 2u);
                break;
            }
            case Opcode::BtS:
            case Opcode::BfS: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (target) enqueue(*target);
                enqueue(pc + 4u);
                break;
            }
            case Opcode::Braf:
            case Opcode::Rts:
            case Opcode::Rte:
                break;
            default:
                enqueue(pc + 2u);
                break;
        }
    }
    return false;
}

bool literal_source_is_copied_from_vbr_setup(const std::vector<std::uint8_t>& bytes,
                                             std::uint32_t base,
                                             std::uint32_t load_address,
                                             std::uint8_t reg) {
    const std::uint32_t begin = load_address >= 0x20u ? load_address - 0x20u : base;
    const std::uint64_t end64 = std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + 0x30u,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto end = static_cast<std::uint32_t>(end64);
    bool saw_vbr = false;
    bool saw_source_load = false;
    bool saw_store = false;
    for (std::uint32_t pc = begin; pc + 2u <= end; pc += 2u) {
        if (!local_address(base, bytes.size(), pc)) continue;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (d.opcode == dcrecomp::sh4::Opcode::StcVbr) saw_vbr = true;
        if (pc > load_address && d.opcode == dcrecomp::sh4::Opcode::MovLPostinc && d.rm == reg) saw_source_load = true;
        if (pc > load_address && d.opcode == dcrecomp::sh4::Opcode::MovLStore) saw_store = true;
    }
    return saw_vbr && saw_source_load && saw_store;
}

struct IndexedRelocatedCopy {
    std::uint32_t source{};
    std::uint32_t destination{};
    std::uint32_t size{};
};

std::optional<IndexedRelocatedCopy> indexed_relocated_copy_from_literal(
    const std::vector<std::uint8_t>& bytes,
    std::uint32_t base,
    const dcrecomp::FunctionAnalysis& analysis,
    const dcrecomp::LiteralReference& source_literal,
    std::uint8_t source_reg) {
    using O = dcrecomp::sh4::Opcode;

    const auto source = canonical_local_address(base, bytes.size(), source_literal.value);
    if (!source) return std::nullopt;

    // Recognize the compact, compiler-generated byte-copy idiom used by retail
    // bootstrap code to move an executable helper to another RAM alias/region:
    //
    //   mov.l source,rS
    //   mov.l dest,rD
    //   mov #0,r0
    //   mov.w bound,rB
    // loop:
    //   mov.b @(r0,rS),rV
    //   mov.b rV,@(r0,rD)
    //   ... increment r0 ...
    //   cmp/hi rB,r0
    //   bf loop
    //
    // The old relocation detector only handled VBR setup copies.  This form is
    // equally strong evidence of executable relocation once the copied source is
    // a clean local SH-4 entry and the destination is another main-RAM address.
    const std::uint32_t window_begin =
        source_literal.instruction_address >= 0x20u ? source_literal.instruction_address - 0x20u : base;
    const std::uint32_t window_end = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        static_cast<std::uint64_t>(source_literal.instruction_address) + 0x30u,
        static_cast<std::uint64_t>(base) + bytes.size()));

    const dcrecomp::sh4::Instruction* indexed_load = nullptr;
    const dcrecomp::sh4::Instruction* indexed_store = nullptr;
    for (const auto& insn : analysis.instructions) {
        if (insn.address < source_literal.instruction_address || insn.address > window_end) continue;
        if (!indexed_load && insn.opcode == O::MovBIndexedLoad && insn.rm == source_reg) {
            indexed_load = &insn;
            continue;
        }
        if (indexed_load && insn.address >= indexed_load->address &&
            insn.address <= indexed_load->address + 6u &&
            insn.opcode == O::MovBIndexedStore && insn.rm == indexed_load->rn) {
            indexed_store = &insn;
            break;
        }
    }
    if (!indexed_load || !indexed_store) return std::nullopt;
    const std::uint8_t destination_reg = indexed_store->rn;

    const dcrecomp::LiteralReference* destination_literal = nullptr;
    for (const auto& literal : analysis.literals) {
        if (literal.kind != dcrecomp::LiteralKind::Long32 || literal.destination_register != destination_reg) continue;
        if (literal.instruction_address < window_begin ||
            literal.instruction_address > source_literal.instruction_address + 8u) continue;
        // Prefer the closest defining literal before the copy loop.
        if (!destination_literal || literal.instruction_address > destination_literal->instruction_address)
            destination_literal = &literal;
    }
    if (!destination_literal) return std::nullopt;

    const std::uint32_t destination_phys = destination_literal->value & 0x1FFFFFFFu;
    const std::uint32_t source_phys = (*source) & 0x1FFFFFFFu;
    // Both ends must be in Dreamcast main RAM, and this must actually relocate
    // rather than merely copy bytes in-place inside the source image.
    if (destination_phys < 0x0C000000u || destination_phys >= 0x10000000u ||
        source_phys < 0x0C000000u || source_phys >= 0x10000000u ||
        destination_phys == source_phys) return std::nullopt;

    const dcrecomp::sh4::Instruction* compare = nullptr;
    const dcrecomp::sh4::Instruction* back_branch = nullptr;
    for (const auto& insn : analysis.instructions) {
        if (insn.address < indexed_store->address || insn.address > indexed_store->address + 0x18u) continue;
        if (!compare && insn.opcode == O::CmpHi && insn.rn == 0u) compare = &insn;
        if ((insn.opcode == O::Bf || insn.opcode == O::Bt ||
             insn.opcode == O::BfS || insn.opcode == O::BtS) &&
            insn.target <= indexed_load->address && insn.target + 8u >= indexed_load->address) {
            back_branch = &insn;
        }
    }
    if (!compare || !back_branch) return std::nullopt;

    std::optional<std::uint32_t> inclusive_bound;
    for (const auto& literal : analysis.literals) {
        if (literal.destination_register != compare->rm ||
            literal.instruction_address > compare->address ||
            literal.instruction_address + 0x20u < source_literal.instruction_address) continue;
        if (literal.kind == dcrecomp::LiteralKind::Word16) {
            inclusive_bound = literal.value & 0xFFFFu;
        } else if (literal.kind == dcrecomp::LiteralKind::Long32) {
            inclusive_bound = literal.value;
        }
    }
    if (!inclusive_bound) {
        for (const auto& insn : analysis.instructions) {
            if (insn.opcode != O::MovImm || insn.rn != compare->rm ||
                insn.address > compare->address ||
                insn.address + 0x20u < source_literal.instruction_address) continue;
            const auto imm = static_cast<std::int32_t>(insn.immediate);
            if (imm >= 0) inclusive_bound = static_cast<std::uint32_t>(imm);
        }
    }
    if (!inclusive_bound || *inclusive_bound >= 0x10000u) return std::nullopt;
    const std::uint32_t copy_size = *inclusive_bound + 1u;
    if (copy_size < 16u ||
        static_cast<std::uint64_t>(*source - base) + copy_size > bytes.size()) return std::nullopt;

    return IndexedRelocatedCopy{*source, destination_literal->value, copy_size};
}

bool literal_target_is_installed_in_vbr_vector(const std::vector<std::uint8_t>& bytes,
                                               std::uint32_t base,
                                               std::uint32_t load_address,
                                               std::uint8_t value_reg) {
    // Katana installs some interrupt/event callbacks by loading VBR into a base
    // register, loading a literal function pointer, then storing that pointer into
    // a VBR-relative vector slot. The vector dispatcher later invokes the stored
    // address dynamically, so the target has no direct static call edge.
    std::optional<std::uint8_t> vbr_reg;
    const std::uint32_t begin = load_address >= 0x20u ? load_address - 0x20u : base;
    for (std::uint32_t pc = begin; pc < load_address; pc += 2u) {
        if (!local_address(base, bytes.size(), pc)) continue;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (d.opcode == dcrecomp::sh4::Opcode::StcVbr) vbr_reg = d.rn;
    }
    if (!vbr_reg) return false;

    const std::uint64_t end64 = std::min<std::uint64_t>(
        static_cast<std::uint64_t>(load_address) + 0x0Cu,
        static_cast<std::uint64_t>(base) + bytes.size());
    const auto end = static_cast<std::uint32_t>(end64);
    for (std::uint32_t pc = load_address + 2u; pc + 2u <= end; pc += 2u) {
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if ((d.opcode == dcrecomp::sh4::Opcode::MovLIndexedStore ||
             d.opcode == dcrecomp::sh4::Opcode::MovLDispStore ||
             d.opcode == dcrecomp::sh4::Opcode::MovLStore) &&
            d.rm == value_reg && d.rn == *vbr_reg) {
            return true;
        }
        // If the literal register is overwritten before being stored, this load
        // was data rather than the vector callback we are looking for.
        if (d.rn == value_reg &&
            (d.opcode == dcrecomp::sh4::Opcode::MovImm ||
             d.opcode == dcrecomp::sh4::Opcode::MovReg ||
             d.opcode == dcrecomp::sh4::Opcode::MovLPcRel ||
             d.opcode == dcrecomp::sh4::Opcode::MovLLoad ||
             d.opcode == dcrecomp::sh4::Opcode::MovLDispLoad ||
             d.opcode == dcrecomp::sh4::Opcode::MovLIndexedLoad ||
             d.opcode == dcrecomp::sh4::Opcode::MovLPostinc)) return false;
    }
    return false;
}

bool looks_like_pointer_table(const std::vector<std::uint8_t>& bytes, std::uint32_t base, std::uint32_t address) {
    if (address < base) return false;

    // A canonical RTS;NOP is itself a complete callable SH-4 function. Katana
    // SDK tables frequently place the next callback/literal table immediately
    // after this 4-byte no-op body. Looking at the following 28 bytes as u32s
    // therefore makes a real callback appear to be a dense pointer table. Give
    // the architectural return + valid delay slot precedence over data-density
    // heuristics. This is generic and does not depend on any title address.
    if (local_address(base, bytes.size(), address + 2u)) {
        const auto first = dcrecomp::sh4::decode(raw16_at(bytes, base, address), address);
        const auto delay_raw = raw16_at(bytes, base, address + 2u);
        const auto delay = dcrecomp::sh4::decode(delay_raw, address + 2u);
        const bool canonical_leaf_delay = delay_raw == 0x0009u ||
            (delay.opcode == dcrecomp::sh4::Opcode::MovImm && delay.rn == 0u);
        if (first.opcode == dcrecomp::sh4::Opcode::Rts && canonical_leaf_delay) return false;
    }

    // Do not reject compact compiler thunks merely because their literal pool
    // begins immediately after a short return/jump sequence. Interpreting the
    // first 32 bytes as aligned u32s can make those literal halfwords look like
    // a dense table of in-image addresses. A clean op + RTS/JMP/BRA + delay-slot
    // prefix is stronger evidence that the address itself is executable code.
    if (local_address(base, bytes.size(), address + 4u)) {
        const auto d0 = dcrecomp::sh4::decode(raw16_at(bytes, base, address), address);
        const auto d1 = dcrecomp::sh4::decode(raw16_at(bytes, base, address + 2u), address + 2u);
        const auto d2 = dcrecomp::sh4::decode(raw16_at(bytes, base, address + 4u), address + 4u);
        const bool short_exit = d1.opcode == dcrecomp::sh4::Opcode::Rts ||
                                d1.opcode == dcrecomp::sh4::Opcode::Jmp ||
                                d1.opcode == dcrecomp::sh4::Opcode::Bra ||
                                d1.opcode == dcrecomp::sh4::Opcode::Braf;
        if (short_exit && dcrecomp::sh4::is_known(d0) && dcrecomp::sh4::is_known(d2)) return false;
    }
    return dcrecomp::looks_like_dense_dreamcast_ram_pointer_words(bytes, base, address, 32u);
}

std::uint32_t raw32_at(const std::vector<std::uint8_t>& bytes, std::uint32_t base, std::uint32_t address) {
    const auto off = static_cast<std::size_t>(address - base);
    return static_cast<std::uint32_t>(bytes[off]) |
           (static_cast<std::uint32_t>(bytes[off + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes[off + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes[off + 3u]) << 24u);
}

bool looks_like_clean_code_prefix(const std::vector<std::uint8_t>& bytes,
                                  std::uint32_t base,
                                  std::uint32_t address,
                                  std::uint32_t halfwords = 16u) {
    if (!local_address(base, bytes.size(), address) || halfwords == 0u) return false;
    for (std::uint32_t i = 0u; i < halfwords; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;
    }
    return true;
}


bool looks_like_local_cfg_callable_entry(const std::vector<std::uint8_t>& bytes,
                                         std::uint32_t base,
                                         std::uint32_t address) {
    // A literal passed in R4-R7 immediately before a registration call is strong
    // address-taken evidence, but real retail callbacks often branch around an
    // inline literal pool. A linear "N clean halfwords" test then rejects valid
    // functions even though every *reachable* instruction is normal SH-4.
    // Walk a deliberately small local CFG instead: conditional branches explore
    // both successors, BRA follows only its local destination, calls continue at
    // the architectural return point, and literal/data gaps are never decoded.
    if (!local_address(base, bytes.size(), address)) return false;
    constexpr std::uint32_t kMaxSpan = 0x200u;
    constexpr std::size_t kMaxVisited = 160u;
    std::vector<std::uint32_t> work{address};
    std::set<std::uint32_t> visited;
    bool saw_terminal = false;
    bool saw_real_control = false;

    const auto enqueue = [&](std::uint32_t pc) {
        if (pc < address || pc - address > kMaxSpan) return;
        if (!local_address(base, bytes.size(), pc)) return;
        if (!visited.contains(pc)) work.push_back(pc);
    };

    while (!work.empty() && visited.size() < kMaxVisited) {
        const auto pc = work.back();
        work.pop_back();
        if (visited.contains(pc)) continue;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;
        visited.insert(pc);

        using dcrecomp::sh4::Opcode;
        const bool delayed = d.opcode == Opcode::Bra || d.opcode == Opcode::Bsr ||
            d.opcode == Opcode::BtS || d.opcode == Opcode::BfS || d.opcode == Opcode::Jsr ||
            d.opcode == Opcode::Jmp || d.opcode == Opcode::Bsrf || d.opcode == Opcode::Braf ||
            d.opcode == Opcode::Rts || d.opcode == Opcode::Rte;
        if (delayed) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc)) return false;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            if (!dcrecomp::sh4::is_known(slot)) return false;
            visited.insert(slot_pc);
        }

        switch (d.opcode) {
            case Opcode::Bra: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (!target || *target < address || *target - address > kMaxSpan) return false;
                saw_real_control = true;
                enqueue(*target);
                break;
            }
            case Opcode::Bt:
            case Opcode::Bf: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (!target || *target < address || *target - address > kMaxSpan) return false;
                saw_real_control = true;
                enqueue(*target);
                enqueue(pc + 2u);
                break;
            }
            case Opcode::BtS:
            case Opcode::BfS: {
                const auto target = canonical_local_address(base, bytes.size(), d.target);
                if (!target || *target < address || *target - address > kMaxSpan) return false;
                saw_real_control = true;
                enqueue(*target);
                enqueue(pc + 4u);
                break;
            }
            case Opcode::Bsr:
            case Opcode::Bsrf:
            case Opcode::Jsr:
                saw_real_control = true;
                enqueue(pc + 4u);
                break;
            case Opcode::Rts:
            case Opcode::Rte:
            case Opcode::Jmp:
            case Opcode::Braf:
                saw_terminal = true;
                saw_real_control = true;
                break;
            default:
                enqueue(pc + 2u);
                break;
        }
    }
    return saw_terminal && saw_real_control && visited.size() >= 4u && visited.size() < kMaxVisited;
}

bool looks_like_indexed_tail_dispatch_entry(const std::vector<std::uint8_t>& bytes,
                                               std::uint32_t base,
                                               std::uint32_t address) {
    if (!local_address(base, bytes.size(), address + 12u)) return false;
    std::array<dcrecomp::sh4::Instruction, 7> d{};
    for (std::uint32_t i = 0u; i < d.size(); ++i) {
        const auto pc = address + i * 2u;
        d[i] = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d[i])) return false;
    }
    return d[0].opcode == dcrecomp::sh4::Opcode::MovLPcRel &&
           d[1].opcode == dcrecomp::sh4::Opcode::MovLPcRel && d[1].rn == 0u &&
           d[2].opcode == dcrecomp::sh4::Opcode::MovLLoad &&
           d[3].opcode == dcrecomp::sh4::Opcode::Shll2 && d[3].rn == d[2].rn &&
           d[4].opcode == dcrecomp::sh4::Opcode::MovLIndexedLoad &&
           d[4].rn == d[5].rm &&
           d[5].opcode == dcrecomp::sh4::Opcode::Jmp;
}

bool looks_like_dense_dispatch_entry(const std::vector<std::uint8_t>& bytes,
                                      std::uint32_t base,
                                      std::uint32_t address) {
    // This compact tail dispatcher is a real callable entry, but promoting it
    // through the ordinary dense-target path recursively opens an unbounded
    // selector table. Keep it as structural evidence in FunctionAnalysis while
    // leaving runtime-selected handlers to be added when their call site proves
    // them. This preserves RAW_SH4=0 for commercial closures.
    if (looks_like_indexed_tail_dispatch_entry(bytes, base, address)) return false;
    if (looks_like_clean_code_prefix(bytes, base, address)) return true;
    if (!local_address(base, bytes.size(), address) ||
        !local_address(base, bytes.size(), address + 2u)) return false;
    const auto first = dcrecomp::sh4::decode(raw16_at(bytes, base, address), address);
    const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, address + 2u), address + 2u);
    // Dense absolute tables are already very strong executable evidence, so a
    // compact RTS with any valid delay slot is a legitimate callable entry even
    // when literal/data bytes immediately follow and defeat a 32-byte prefix test.
    if (first.opcode == dcrecomp::sh4::Opcode::Rts && dcrecomp::sh4::is_known(slot)) return true;

    // Retail SDK method tables also contain tiny literal-fed tail veneers:
    // MOV.L @(disp,PC),Rn; JMP @Rn; <delay slot>. Their literal pool starts
    // immediately after the veneer, so requiring 32 clean bytes rejects them.
    if (local_address(base, bytes.size(), address + 4u)) {
        const auto jump = dcrecomp::sh4::decode(raw16_at(bytes, base, address + 2u), address + 2u);
        const auto delay = dcrecomp::sh4::decode(raw16_at(bytes, base, address + 4u), address + 4u);
        if (first.opcode == dcrecomp::sh4::Opcode::MovLPcRel &&
            jump.opcode == dcrecomp::sh4::Opcode::Jmp && jump.rm == first.rn &&
            dcrecomp::sh4::is_known(delay)) return true;
    }
    return false;
}

bool looks_like_callback_entry(const std::vector<std::uint8_t>& bytes,
                               std::uint32_t base,
                               std::uint32_t address) {
    if (!local_address(base, bytes.size(), address)) return false;
    // Callback objects and copied VBR templates are deliberately held to the
    // original conservative code-entry test. Their surrounding data can contain
    // many values that happen to decode as legal SH-4, so do not widen this
    // predicate just to accommodate long address-taken functions.
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        if (!dcrecomp::sh4::is_known(dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc))) return false;
    }
    return looks_like_code_entry(bytes, base, address);
}

bool looks_like_probable_ascii_data(const std::vector<std::uint8_t>& bytes,
                                    std::uint32_t base,
                                    std::uint32_t address) {
    if (!local_address(base, bytes.size(), address)) return false;
    const auto off = static_cast<std::size_t>(address - base);
    const auto available = std::min<std::size_t>(64u, bytes.size() - off);
    if (available < 24u) return false;
    std::size_t printable = 0u;
    std::size_t nul = 0u;
    for (std::size_t i = 0u; i < available; ++i) {
        const auto c = bytes[off + i];
        if (c >= 0x20u && c <= 0x7Eu) ++printable;
        else if (c == 0u) ++nul;
    }
    // Symbol-free retail images contain many string literals whose byte pairs can
    // accidentally decode as legal SH-4 for a surprisingly long prefix.  Reject
    // a callback candidate only when the bytes are overwhelmingly plain ASCII
    // text (allowing a small number of NUL terminators).
    return printable * 100u >= available * 80u && printable + nul + 2u >= available;
}

bool looks_like_argument_callback_entry(const std::vector<std::uint8_t>& bytes,
                                        std::uint32_t base,
                                        std::uint32_t address) {
    if (!local_address(base, bytes.size(), address)) return false;

    // R4-R7 callback arguments can point at long functions whose first RTS/JMP is
    // well beyond the short raw-code heuristic window. Scan a longer clean prefix
    // and require real control flow, but stop once a terminal control transfer plus
    // its delay slot has been validated. Compact Katana callbacks commonly place a
    // literal pool immediately after RTS; those data words must not be required to
    // decode as instructions just because they are adjacent to the callback body.
    bool saw_control = false;
    bool saw_call = false;
    // A compact callback may be nothing more than a small ABI adapter ending in
    // JMP @Rn where Rn was loaded from a PC-relative executable literal. Track
    // those literal-fed registers so the tail transfer itself can prove the entry
    // callable without requiring an earlier JSR. This remains deliberately local:
    // the target must stay inside the raw image and begin with clean SH-4 code.
    std::array<std::optional<std::uint32_t>, 16> literal_code_target{};
    constexpr std::uint32_t kPrefixHalfwords = 24u; // inspect at most 48 bytes
    for (std::uint32_t i = 0u; i < kPrefixHalfwords; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;

        if (d.opcode == dcrecomp::sh4::Opcode::MovLPcRel && d.rn < literal_code_target.size() &&
            local_address(base, bytes.size(), d.effective_address)) {
            const auto target = canonical_local_address(base, bytes.size(),
                                                        raw32_at(bytes, base, d.effective_address));
            if (target && !looks_like_pointer_table(bytes, base, *target) &&
                looks_like_clean_code_prefix(bytes, base, *target, 8u) &&
                !looks_like_probable_ascii_data(bytes, base, *target)) {
                literal_code_target[d.rn] = *target;
            } else {
                literal_code_target[d.rn].reset();
            }
        }

        // A callback/task entry may be a two-instruction tail thunk: BRA to a
        // shared worker with an argument selector in the delay slot. Accept this
        // only at the candidate entry itself, and only when both the delay slot
        // and the direct local destination have a clean SH-4 prefix. This covers
        // packed SDK task-entry thunks without treating arbitrary BRA-looking data
        // later in the scan as a callable boundary.
        if (i == 0u && d.opcode == dcrecomp::sh4::Opcode::Bra) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc)) return false;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            const auto target = canonical_local_address(base, bytes.size(), d.target);
            return dcrecomp::sh4::is_known(slot) && target &&
                   !looks_like_pointer_table(bytes, base, *target) &&
                   looks_like_clean_code_prefix(bytes, base, *target, 8u);
        }

        // Architectural returns prove that bytes immediately following the
        // delay slot may be a literal pool. A second compact Katana callback form
        // performs one or more real calls and then tail-calls a shared worker with
        // JMP/BRAF. Treat that indirect tail transfer as terminal only after a
        // call has already established convincing executable control flow; this
        // avoids promoting arbitrary JMP-looking data or internal jump-only code.
        const bool terminal_return = d.opcode == dcrecomp::sh4::Opcode::Rts ||
                                     d.opcode == dcrecomp::sh4::Opcode::Rte;
        if (terminal_return) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc)) return false;
            const auto slot_raw = raw16_at(bytes, base, slot_pc);
            const auto slot = dcrecomp::sh4::decode(slot_raw, slot_pc);
            if (!dcrecomp::sh4::is_known(slot)) return false;
            if (i >= 1u) return true;
            // A two-instruction RTS;NOP stub is a legitimate no-op callback and
            // appears in SDK initializer/fill APIs. Restrict the zero-length body
            // case to the canonical NOP delay slot so arbitrary data beginning in
            // 0x000B is not broadly promoted as callable code.
            return d.opcode == dcrecomp::sh4::Opcode::Rts &&
                   (slot_raw == 0x0009u ||
                    (slot.opcode == dcrecomp::sh4::Opcode::MovImm && slot.rn == 0u));
        }

        const bool terminal_tail_call = d.opcode == dcrecomp::sh4::Opcode::Jmp ||
                                        d.opcode == dcrecomp::sh4::Opcode::Braf;
        if (terminal_tail_call && i >= 1u) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc)) return false;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            if (!dcrecomp::sh4::is_known(slot)) return false;
            if (saw_call) return true;

            // Literal-fed JMP wrappers are common in Katana object callbacks:
            // they adapt one argument and tail-call a shared implementation. The
            // literal target is executable evidence, unlike an arbitrary indirect
            // JMP found while decoding adjacent data. BRAF is intentionally not
            // accepted by this shortcut because its destination is PC-relative.
            if (d.opcode == dcrecomp::sh4::Opcode::Jmp && d.rm < literal_code_target.size() &&
                literal_code_target[d.rm].has_value()) return true;
            return false;
        }

        switch (d.opcode) {
            case dcrecomp::sh4::Opcode::Bsr:
            case dcrecomp::sh4::Opcode::Bt:
            case dcrecomp::sh4::Opcode::Bf:
            case dcrecomp::sh4::Opcode::BtS:
            case dcrecomp::sh4::Opcode::BfS:
            case dcrecomp::sh4::Opcode::Bsrf:
            case dcrecomp::sh4::Opcode::Jsr:
                saw_control = true;
                saw_call = true;
                break;
            default:
                break;
        }
    }
    return saw_control;
}

std::vector<std::uint32_t> packed_bra_selector_family(const std::vector<std::uint8_t>& bytes,
                                                       std::uint32_t base,
                                                       std::uint32_t member) {
    // Katana SDK code sometimes packs a selector into a run of four-byte tail
    // thunks instead of using a pointer table:
    //
    //   BRA shared_worker      BRA shared_worker      ...
    //    MOV #0,R4             MOV #1,R4
    //
    // A symbol-free closure can discover one or several members independently,
    // while a runtime-selected final member remains invisible because no literal
    // contains its address.  Recover the complete family structurally.  Requiring
    // at least three consecutive BRA/MOV-immediate pairs, one argument register,
    // consecutive selector values and one identical local worker is much stronger
    // evidence than treating arbitrary adjacent BRA opcodes as function entries.
    if ((member & 3u) != 0u || !local_address(base, bytes.size(), member) ||
        !local_address(base, bytes.size(), member + 2u)) return {};

    const auto head_bra = dcrecomp::sh4::decode(raw16_at(bytes, base, member), member);
    const auto head_slot = dcrecomp::sh4::decode(raw16_at(bytes, base, member + 2u), member + 2u);
    if (head_bra.opcode != dcrecomp::sh4::Opcode::Bra ||
        head_slot.opcode != dcrecomp::sh4::Opcode::MovImm ||
        head_slot.rn < 4u || head_slot.rn > 7u) return {};
    const auto worker = canonical_local_address(base, bytes.size(), head_bra.target);
    if (!worker || looks_like_pointer_table(bytes, base, *worker) ||
        looks_like_probable_ascii_data(bytes, base, *worker) ||
        !looks_like_clean_code_prefix(bytes, base, *worker, 8u)) return {};

    const auto selector_reg = head_slot.rn;
    auto matches = [&](std::uint32_t address, std::int32_t selector) {
        if ((address & 3u) != 0u || !local_address(base, bytes.size(), address) ||
            !local_address(base, bytes.size(), address + 2u)) return false;
        const auto bra = dcrecomp::sh4::decode(raw16_at(bytes, base, address), address);
        const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, address + 2u), address + 2u);
        if (bra.opcode != dcrecomp::sh4::Opcode::Bra ||
            slot.opcode != dcrecomp::sh4::Opcode::MovImm || slot.rn != selector_reg ||
            slot.immediate != selector) return false;
        const auto target = canonical_local_address(base, bytes.size(), bra.target);
        return target && *target == *worker;
    };

    std::uint32_t first = member;
    std::int32_t first_selector = head_slot.immediate;
    constexpr std::size_t kMaxFamilyMembers = 32u;
    std::size_t backward = 0u;
    while (backward + 1u < kMaxFamilyMembers && first >= base + 4u &&
           matches(first - 4u, first_selector - 1)) {
        first -= 4u;
        --first_selector;
        ++backward;
    }

    std::vector<std::uint32_t> family;
    family.reserve(kMaxFamilyMembers);
    for (std::size_t i = 0u; i < kMaxFamilyMembers; ++i) {
        const auto address64 = static_cast<std::uint64_t>(first) + i * 4u;
        if (address64 > 0xFFFFFFFFull) break;
        const auto address = static_cast<std::uint32_t>(address64);
        if (!matches(address, first_selector + static_cast<std::int32_t>(i))) break;
        family.push_back(address);
    }
    if (family.size() < 3u ||
        std::find(family.begin(), family.end(), member) == family.end()) return {};
    return family;
}

bool looks_like_saved_gpr_tail_bra_entry(const std::vector<std::uint8_t>& bytes,
                                          std::uint32_t base,
                                          std::uint32_t address) {
    if (!local_address(base, bytes.size(), address)) return false;
    const auto first = dcrecomp::sh4::decode(raw16_at(bytes, base, address), address);
    // ABI-preserving callback adapters commonly save one callee-saved GPR, adapt
    // arguments, then restore that same GPR in the delay slot of a local tail BRA.
    // Requiring this matching save/restore pair makes the pattern substantially
    // stronger than accepting an arbitrary branch inside code-looking data.
    if (first.opcode != dcrecomp::sh4::Opcode::MovLPredec || first.rn != 15u ||
        first.rm < 8u || first.rm > 14u) return false;
    const auto saved_reg = first.rm;
    constexpr std::uint32_t kMaxHalfwords = 24u;
    for (std::uint32_t i = 1u; i < kMaxHalfwords; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;
        if (d.opcode == dcrecomp::sh4::Opcode::Bra) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc)) return false;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            const auto target = canonical_local_address(base, bytes.size(), d.target);
            return slot.opcode == dcrecomp::sh4::Opcode::MovLPostinc &&
                   slot.rn == saved_reg && slot.rm == 15u && target &&
                   *target < address && address - *target <= 0x200u &&
                   !looks_like_pointer_table(bytes, base, *target) &&
                   looks_like_clean_code_prefix(bytes, base, *target, 8u);
        }
        // This helper intentionally recognizes a straight-line adapter only.
        if (d.opcode == dcrecomp::sh4::Opcode::Bsr || d.opcode == dcrecomp::sh4::Opcode::Bsrf ||
            d.opcode == dcrecomp::sh4::Opcode::Jsr || d.opcode == dcrecomp::sh4::Opcode::Jmp ||
            d.opcode == dcrecomp::sh4::Opcode::Rts || d.opcode == dcrecomp::sh4::Opcode::Rte ||
            d.opcode == dcrecomp::sh4::Opcode::Bt || d.opcode == dcrecomp::sh4::Opcode::Bf ||
            d.opcode == dcrecomp::sh4::Opcode::BtS || d.opcode == dcrecomp::sh4::Opcode::BfS) return false;
    }
    return false;
}

bool looks_like_strong_abi_prologue(const std::vector<std::uint8_t>& bytes,
                                    std::uint32_t base,
                                    std::uint32_t address,
                                    bool allow_fpu_only = false) {
    if (!local_address(base, bytes.size(), address)) return false;
    constexpr std::uint32_t kWindowHalfwords = 12u;
    std::size_t gpr_saves = 0u;
    std::size_t fpu_saves = 0u;
    bool pr_save = false;
    bool saw_stack_action = false;
    std::optional<std::uint32_t> first_stack_action_index;

    for (std::uint32_t i = 0u; i < kWindowHalfwords; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;
        if (d.opcode == dcrecomp::sh4::Opcode::MovLPredec && d.rn == 15u &&
            d.rm >= 8u && d.rm <= 14u) {
            ++gpr_saves;
            saw_stack_action = true;
            if (!first_stack_action_index) first_stack_action_index = i;
        } else if (d.opcode == dcrecomp::sh4::Opcode::FmovStorePreDec && d.rn == 15u &&
                   d.rm >= 12u && d.rm <= 15u) {
            ++fpu_saves;
            saw_stack_action = true;
            if (!first_stack_action_index) first_stack_action_index = i;
        } else if (d.opcode == dcrecomp::sh4::Opcode::StsLPr && d.rn == 15u) {
            pr_save = true;
            saw_stack_action = true;
            if (!first_stack_action_index) first_stack_action_index = i;
        }

        // A terminal before the save signature completes means this was not a
        // normal function prologue. Calls are allowed: leaf wrappers can save a
        // register immediately before invoking a helper.
        if (!pr_save && (d.opcode == dcrecomp::sh4::Opcode::Rts ||
                         d.opcode == dcrecomp::sh4::Opcode::Rte ||
                         d.opcode == dcrecomp::sh4::Opcode::Jmp ||
                         d.opcode == dcrecomp::sh4::Opcode::Braf)) return false;
    }
    if (!saw_stack_action || !pr_save || !first_stack_action_index) return false;
    // A raw-data halfword immediately before a real function can itself decode as
    // a valid SH-4 ALU instruction.  Do not let such a word shift the synthetic
    // entry backwards: a strong ABI prologue may have at most one setup
    // instruction (typically a PC-relative literal load) before its first save.
    if (*first_stack_action_index > 1u) return false;
    if (gpr_saves >= 1u) return true;
    return allow_fpu_only && fpu_saves >= 2u;
}

// 0.0.181: a function pointer that is proven to be stored into an object/table is
// stronger evidence than an arbitrary pointer-looking word in raw data. Retail
// Katana code can perform one or two harmless setup instructions before saving
// callee-saved state; the generic ABI predicate intentionally rejects that shape
// to avoid sliding a synthetic entry backwards into data. For *stored callbacks*
// only, allow two setup instructions while still requiring a clean, unmistakable
// ABI signature (R8-R14 save + PR save) in the first few instructions.
bool looks_like_stored_callback_abi_entry(const std::vector<std::uint8_t>& bytes,
                                          std::uint32_t base,
                                          std::uint32_t address) {
    if (!local_address(base, bytes.size(), address) ||
        looks_like_probable_ascii_data(bytes, base, address)) return false;

    constexpr std::uint32_t kWindowHalfwords = 8u;
    std::size_t gpr_saves = 0u;
    bool pr_save = false;
    std::optional<std::uint32_t> first_stack_action_index;

    for (std::uint32_t i = 0u; i < kWindowHalfwords; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;

        if (d.opcode == dcrecomp::sh4::Opcode::MovLPredec && d.rn == 15u &&
            d.rm >= 8u && d.rm <= 14u) {
            ++gpr_saves;
            if (!first_stack_action_index) first_stack_action_index = i;
        } else if (d.opcode == dcrecomp::sh4::Opcode::FmovStorePreDec && d.rn == 15u) {
            if (!first_stack_action_index) first_stack_action_index = i;
        } else if (d.opcode == dcrecomp::sh4::Opcode::StsLPr && d.rn == 15u) {
            pr_save = true;
            if (!first_stack_action_index) first_stack_action_index = i;
        }

        // A terminal transfer before the ABI signature is complete is not a
        // normal callback prologue. Calls are fine after the save sequence.
        if ((!pr_save || gpr_saves == 0u) &&
            (d.opcode == dcrecomp::sh4::Opcode::Rts || d.opcode == dcrecomp::sh4::Opcode::Rte ||
             d.opcode == dcrecomp::sh4::Opcode::Jmp || d.opcode == dcrecomp::sh4::Opcode::Braf)) {
            return false;
        }
    }

    return first_stack_action_index.has_value() && *first_stack_action_index <= 2u &&
           gpr_saves >= 1u && pr_save;
}

// Forward declaration: stored callback provenance is strong enough to reuse the
// compact callable validator defined below. This covers tiny Katana callback
// veneers such as PUSH_PR; JSR @Rn; ...; JMP @Rm; POP_PR that intentionally
// save no callee-saved GPRs.
bool looks_like_compact_callable_entry(const std::vector<std::uint8_t>& bytes,
                                       std::uint32_t base,
                                       std::uint32_t address);

bool looks_like_stored_callback_entry(const std::vector<std::uint8_t>& bytes,
                                      std::uint32_t base,
                                      std::uint32_t address) {
    return looks_like_argument_callback_entry(bytes, base, address) ||
           looks_like_local_cfg_callable_entry(bytes, base, address) ||
           looks_like_stored_callback_abi_entry(bytes, base, address) ||
           looks_like_compact_callable_entry(bytes, base, address);
}

bool looks_like_fpu_only_abi_prologue(const std::vector<std::uint8_t>& bytes,
                                      std::uint32_t base,
                                      std::uint32_t address) {
    if (!local_address(base, bytes.size(), address)) return false;
    constexpr std::uint32_t kWindowHalfwords = 12u;
    std::size_t gpr_saves = 0u;
    std::size_t fpu_saves = 0u;
    bool pr_save = false;
    for (std::uint32_t i = 0u; i < kWindowHalfwords; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;
        if (d.opcode == dcrecomp::sh4::Opcode::MovLPredec && d.rn == 15u &&
            d.rm >= 8u && d.rm <= 14u) {
            ++gpr_saves;
        } else if (d.opcode == dcrecomp::sh4::Opcode::FmovStorePreDec && d.rn == 15u &&
                   d.rm >= 12u && d.rm <= 15u) {
            ++fpu_saves;
        } else if (d.opcode == dcrecomp::sh4::Opcode::StsLPr && d.rn == 15u) {
            pr_save = true;
        }
        if (!pr_save && (d.opcode == dcrecomp::sh4::Opcode::Rts ||
                         d.opcode == dcrecomp::sh4::Opcode::Rte ||
                         d.opcode == dcrecomp::sh4::Opcode::Jmp ||
                         d.opcode == dcrecomp::sh4::Opcode::Braf)) return false;
    }
    return pr_save && gpr_saves == 0u && fpu_saves >= 2u;
}

bool looks_like_compact_callable_entry(const std::vector<std::uint8_t>& bytes,
                                       std::uint32_t base,
                                       std::uint32_t address) {
    if (!local_address(base, bytes.size(), address)) return false;
    constexpr std::uint32_t kMaxHalfwords = 24u;
    for (std::uint32_t i = 0u; i < kMaxHalfwords; ++i) {
        const auto pc = address + i * 2u;
        if (!local_address(base, bytes.size(), pc)) return false;
        const auto d = dcrecomp::sh4::decode(raw16_at(bytes, base, pc), pc);
        if (!dcrecomp::sh4::is_known(d)) return false;
        const bool terminal = d.opcode == dcrecomp::sh4::Opcode::Rts ||
                              d.opcode == dcrecomp::sh4::Opcode::Jmp ||
                              d.opcode == dcrecomp::sh4::Opcode::Bra ||
                              d.opcode == dcrecomp::sh4::Opcode::Braf;
        if (terminal) {
            const auto slot_pc = pc + 2u;
            if (!local_address(base, bytes.size(), slot_pc)) return false;
            const auto slot = dcrecomp::sh4::decode(raw16_at(bytes, base, slot_pc), slot_pc);
            return dcrecomp::sh4::is_known(slot) && i >= 1u;
        }
    }
    return looks_like_argument_callback_entry(bytes, base, address);
}

std::vector<std::uint32_t> sparse_callback_table_targets(const std::vector<std::uint8_t>& bytes,
                                                          std::uint32_t base,
                                                          std::uint32_t table_address) {
    std::vector<std::uint32_t> targets;
    if ((table_address & 3u) != 0u || table_address < base) return targets;
    constexpr std::size_t kMaxWords = 16u;
    std::size_t zeros = 0u;
    for (std::size_t i = 0u; i < kMaxWords; ++i) {
        const auto a64 = static_cast<std::uint64_t>(table_address) + i * 4u;
        if (a64 + 4u > static_cast<std::uint64_t>(base) + bytes.size()) break;
        const auto a = static_cast<std::uint32_t>(a64);
        const auto value = raw32_at(bytes, base, a);
        if (value == 0u || value == 0xFFFFFFFFu) {
            ++zeros;
            continue;
        }
        const auto canonical = canonical_local_address(base, bytes.size(), value);
        if (!canonical || looks_like_pointer_table(bytes, base, *canonical) ||
            !looks_like_compact_callable_entry(bytes, base, *canonical)) {
            break;
        }
        targets.push_back(*canonical);
    }
    // Require a convincing sparse init/dispatch table: at least three callable
    // entries, and all scanned scalar gaps must be null/sentinel words.
    if (targets.size() < 3u || zeros == 0u) targets.clear();
    return targets;
}

std::vector<std::uint32_t> callback_object_targets(const std::vector<std::uint8_t>& bytes,
                                                   std::uint32_t base,
                                                   std::uint32_t object_address) {
    std::vector<std::uint32_t> targets;
    if ((object_address & 3u) != 0u || object_address < base) return targets;
    const auto off = static_cast<std::size_t>(object_address - base);
    if (off + 12u > bytes.size()) return targets;

    // Katana objects may place a small scalar/header prefix before a dense ops
    // table. Search only the first few words for the beginning of a run of code
    // pointers, then require at least three consecutive, conservative code-looking
    // entries. This still rejects arbitrary mixed structs while covering objects
    // shaped as {state,state,state,fn0,fn1,fn2,...}.
    constexpr std::size_t kMaxHeaderWords = 4u;
    // Retail SDK interface objects can expose substantially more than 24
    // consecutive methods (ChuChu's active driver table reaches +0x78). The
    // run still terminates on the first non-code pointer, so a wider bound does
    // not relax the dense-object evidence; it only avoids truncating valid tails.
    constexpr std::size_t kMaxWords = 64u;
    for (std::size_t start_word = 0; start_word <= kMaxHeaderWords; ++start_word) {
        std::vector<std::uint32_t> run;
        for (std::size_t i = start_word; i < kMaxWords; ++i) {
            const auto address64 = static_cast<std::uint64_t>(object_address) + i * 4u;
            if (address64 + 4u > static_cast<std::uint64_t>(base) + bytes.size()) break;
            const auto address = static_cast<std::uint32_t>(address64);
            const auto value = raw32_at(bytes, base, address);
            const auto canonical = canonical_local_address(base, bytes.size(), value);
            if (!canonical || looks_like_pointer_table(bytes, base, *canonical)) break;
            bool callable = looks_like_callback_entry(bytes, base, *canonical);
            // Once an object has already established a long, dense run of
            // conservative methods, allow later long methods whose first 32
            // bytes are decoder-clean (or a compact return thunk). This avoids
            // truncating large SDK vtables while keeping short/mixed objects on
            // the stricter early-return predicate.
            if (!callable && run.size() >= 8u)
                callable = looks_like_dense_dispatch_entry(bytes, base, *canonical);
            if (!callable) break;
            run.push_back(*canonical);
        }
        if (run.size() >= 3u) return run;
    }
    return targets;
}


std::vector<std::uint32_t> anchored_dense_callback_table_targets(
    const std::vector<std::uint8_t>& bytes,
    std::uint32_t base,
    const std::set<std::uint32_t>& known_entries) {
    std::vector<std::uint32_t> targets;
    if (bytes.size() < 32u) return targets;

    // Some Katana SDK objects are materialized/copied from static vtables rather
    // than referenced directly by a PC-relative literal.  A runtime method load
    // can therefore select a perfectly valid callback whose static table was
    // never visited by the ordinary literal-driven closure.  Recover only long,
    // aligned runs of local code pointers that are *anchored* by several entries
    // already proven executable by the current closure.  This is deliberately
    // much stronger evidence than a global pointer scan: at least eight
    // consecutive local pointers and at least four already-known members are
    // required before any missing member is considered.
    constexpr std::size_t kMinRun = 8u;
    constexpr std::size_t kMinAnchors = 4u;
    const std::size_t word_count = bytes.size() / 4u;

    std::size_t wi = 0u;
    while (wi < word_count) {
        std::vector<std::uint32_t> run;
        const std::size_t run_start = wi;
        while (wi < word_count) {
            const auto address64 = static_cast<std::uint64_t>(base) + wi * 4u;
            if (address64 + 4u > static_cast<std::uint64_t>(base) + bytes.size()) break;
            const auto address = static_cast<std::uint32_t>(address64);
            const auto canonical = canonical_local_address(base, bytes.size(), raw32_at(bytes, base, address));
            if (!canonical) break;
            run.push_back(*canonical);
            ++wi;
        }

        if (run.size() >= kMinRun) {
            std::size_t anchors = 0u;
            for (const auto target : run) anchors += known_entries.contains(target) ? 1u : 0u;
            // A third of the table (bounded to the first 24-word confidence
            // window) must already be known.  This keeps the rule useful for
            // large SDK vtables without letting four accidental pointers anchor
            // an arbitrarily long data region.
            const auto confidence_words = std::min<std::size_t>(run.size(), 24u);
            if (anchors >= kMinAnchors && anchors * 2u >= confidence_words) {
                for (const auto target : run) {
                    if (known_entries.contains(target) || looks_like_pointer_table(bytes, base, target)) continue;
                    const bool callable = looks_like_local_cfg_callable_entry(bytes, base, target) ||
                                          looks_like_compact_callable_entry(bytes, base, target) ||
                                          looks_like_argument_callback_entry(bytes, base, target) ||
                                          looks_like_dense_dispatch_entry(bytes, base, target);
                    if (callable) targets.push_back(target);
                }
            }
        }

        // If no local-pointer word was consumed, advance one word.  Otherwise wi
        // already names the first non-pointer word after the maximal run.
        if (wi == run_start) ++wi;
    }

    // Short SDK method rows need a separate rule from the long-vtable scan.
    // Katana object/state tables commonly contain exactly three callable methods
    // followed by metadata.  If two members of an aligned 3-pointer row are
    // already proven closure entries, a missing literal-fed tail veneer in the
    // remaining slot is strong address-taken executable evidence.
    //
    // Keep this deliberately narrow: only accept the exact
    //   MOV.L @(disp,PC),Rn
    //   JMP   @Rn
    //   <valid delay slot>
    // shape, and require the veneer destination to be already known (or to pass
    // the independent dense callable predicate).  This recovers runtime-selected
    // callbacks such as 0x8C02CE0C without a game-specific seed.
    if (word_count >= 3u) {
        for (std::size_t row = 0u; row + 2u < word_count; ++row) {
            std::array<std::optional<std::uint32_t>, 3> members{};
            std::size_t anchors = 0u;
            bool all_local = true;
            for (std::size_t i = 0u; i < members.size(); ++i) {
                const auto address64 = static_cast<std::uint64_t>(base) + (row + i) * 4u;
                if (address64 + 4u > static_cast<std::uint64_t>(base) + bytes.size()) {
                    all_local = false;
                    break;
                }
                const auto address = static_cast<std::uint32_t>(address64);
                members[i] = canonical_local_address(
                    base, bytes.size(), raw32_at(bytes, base, address));
                if (!members[i]) {
                    all_local = false;
                    break;
                }
                anchors += known_entries.contains(*members[i]) ? 1u : 0u;
            }
            if (!all_local || anchors < 2u) continue;

            for (const auto& member : members) {
                if (!member || known_entries.contains(*member) ||
                    looks_like_pointer_table(bytes, base, *member) ||
                    looks_like_probable_ascii_data(bytes, base, *member) ||
                    !local_address(base, bytes.size(), *member + 4u)) continue;

                const auto load = dcrecomp::sh4::decode(
                    raw16_at(bytes, base, *member), *member);
                const auto jump = dcrecomp::sh4::decode(
                    raw16_at(bytes, base, *member + 2u), *member + 2u);
                const auto delay = dcrecomp::sh4::decode(
                    raw16_at(bytes, base, *member + 4u), *member + 4u);
                if (load.opcode != dcrecomp::sh4::Opcode::MovLPcRel ||
                    jump.opcode != dcrecomp::sh4::Opcode::Jmp ||
                    jump.rm != load.rn || !dcrecomp::sh4::is_known(delay) ||
                    !local_address(base, bytes.size(), load.effective_address)) continue;

                const auto tail = canonical_local_address(
                    base, bytes.size(),
                    raw32_at(bytes, base, load.effective_address));
                if (!tail || looks_like_pointer_table(bytes, base, *tail)) continue;
                if (!known_entries.contains(*tail) &&
                    !looks_like_dense_dispatch_entry(bytes, base, *tail)) continue;

                targets.push_back(*member);
            }
        }
    }

    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    return targets;
}


std::vector<std::uint32_t> long_dense_callback_table_targets(
    const std::vector<std::uint8_t>& bytes,
    std::uint32_t base) {
    std::vector<std::uint32_t> targets;
    if (bytes.size() < 64u) return targets;

    // Long, contiguous arrays of absolute pointers into decoder-clean SH-4 are
    // a common SDK interface/vtable representation. Some tables are reached only
    // through runtime data structures, so none of their members is necessarily
    // in the initial call closure. A run of at least 8 local pointers with at
    // least 3/4 of its confidence window independently looking callable is strong
    // executable evidence without relying on any title-specific address.
    constexpr std::size_t kMinRun = 8u;
    constexpr std::size_t kConfidenceMax = 32u;
    const std::size_t words = bytes.size() / 4u;
    std::size_t wi = 0u;
    while (wi < words) {
        const std::size_t run_start = wi;
        std::vector<std::uint32_t> run;
        while (wi < words) {
            const auto address = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(base) + wi * 4u);
            const auto canonical = canonical_local_address(base, bytes.size(), raw32_at(bytes, base, address));
            if (!canonical) break;
            run.push_back(*canonical);
            ++wi;
        }
        if (run.size() >= kMinRun) {
            const auto confidence = std::min<std::size_t>(run.size(), kConfidenceMax);
            std::size_t callable = 0u;
            for (std::size_t i = 0; i < confidence; ++i) {
                if (!looks_like_pointer_table(bytes, base, run[i]) &&
                    looks_like_dense_dispatch_entry(bytes, base, run[i])) ++callable;
            }
            if (callable * 4u >= confidence * 3u) {
                for (const auto target : run) {
                    if (looks_like_pointer_table(bytes, base, target)) continue;
                    if (looks_like_dense_dispatch_entry(bytes, base, target)) targets.push_back(target);
                }
            }
        }
        if (wi == run_start) ++wi;
    }
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    return targets;
}


std::vector<std::uint32_t> clustered_clean_callback_table_targets(
    const std::vector<std::uint8_t>& bytes,
    std::uint32_t base,
    const dcrecomp::Elf32Image& elf) {
    std::vector<std::uint32_t> targets;
    if (bytes.size() < 32u) return targets;

    // 0.1.0: recover sparse state/handler tables whose members are mostly
    // function pointers but may be interleaved with flags, nulls or data.
    //
    // Daytona exposed the important failure mode: many runtime-selected handlers
    // occurred only once in static data, so the repeated-address proof used by
    // global_address_taken_evidence() could never see them.  A *cluster of source
    // pointers* is nevertheless strong evidence of a dispatch table even when
    // every target is unique.  Qualify the source window first, then independently
    // require each promoted destination to decode as a clean callable fragment.
    // This keeps the rule title-agnostic and avoids turning arbitrary 0x8Cxxxxxx
    // asset words into code.
    constexpr std::size_t kWindowWords = 24u;       // 96-byte local table window
    constexpr std::size_t kMinPointers = 4u;
    constexpr std::size_t kMinClean = 3u;
    constexpr std::size_t kMaxPromotions = 384u;
    const std::size_t word_count = bytes.size() / 4u;

    std::vector<std::optional<std::uint32_t>> local_words(word_count);
    std::vector<std::size_t> prefix(word_count + 1u, 0u);
    for (std::size_t wi = 0u; wi < word_count; ++wi) {
        const auto source = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(base) + wi * 4u);
        local_words[wi] = canonical_local_address(
            base, bytes.size(), raw32_at(bytes, base, source));
        prefix[wi + 1u] = prefix[wi] + (local_words[wi].has_value() ? 1u : 0u);
    }

    struct Validation { bool clean{}; bool strong{}; };
    const bool cluster_debug = std::getenv("DCR_CLUSTER_DEBUG") != nullptr;
    std::map<std::uint32_t, Validation> validation_cache;
    auto validate = [&](std::uint32_t target) -> Validation {
        if (const auto it = validation_cache.find(target); it != validation_cache.end())
            return it->second;

        Validation v{};
        const bool pointer_table = looks_like_pointer_table(bytes, base, target);
        const bool ascii_data = looks_like_probable_ascii_data(bytes, base, target);
        if (pointer_table || ascii_data) {
            if (cluster_debug) std::cerr << "[DCR CLUSTER] " << hex8(target)
                << " reject=" << (pointer_table ? "pointer-table" : "ascii-data") << "\n";
            validation_cache.emplace(target, v);
            return v;
        }

        v.strong = looks_like_strong_abi_prologue(bytes, base, target, true) ||
                   looks_like_compact_callable_entry(bytes, base, target) ||
                   looks_like_local_cfg_callable_entry(bytes, base, target) ||
                   looks_like_argument_callback_entry(bytes, base, target) ||
                   looks_like_dense_dispatch_entry(bytes, base, target) ||
                   looks_like_saved_gpr_tail_bra_entry(bytes, base, target);
        try {
            const auto fragment = dcrecomp::analyze_code_fragment_at(elf, target);
            bool terminal = false;
            if (fragment.unknown == 0u && fragment.instructions.size() >= 2u) {
                for (const auto& insn : fragment.instructions) {
                    using O = dcrecomp::sh4::Opcode;
                    if (insn.opcode == O::Rts || insn.opcode == O::Rte ||
                        insn.opcode == O::Jmp || insn.opcode == O::Braf ||
                        insn.opcode == O::Bra) {
                        terminal = true;
                        break;
                    }
                }
                // Strong entry shapes may be fragments whose terminal is outside
                // the small analysis view.  Otherwise demand a real control-flow
                // terminator/call and a little body length to reject clean-looking
                // scalar data.
                v.clean = v.strong || terminal ||
                          (!fragment.calls.empty() && fragment.instructions.size() >= 4u);
            }
            if (cluster_debug) std::cerr << "[DCR CLUSTER] " << hex8(target)
                << " clean=" << (v.clean ? 1 : 0) << " strong=" << (v.strong ? 1 : 0)
                << " insn=" << fragment.instructions.size() << " unknown=" << fragment.unknown
                << " calls=" << fragment.calls.size() << " terminal=" << (terminal ? 1 : 0) << "\n";
        } catch (const std::exception& e) {
            v.clean = false;
            if (cluster_debug) std::cerr << "[DCR CLUSTER] " << hex8(target)
                << " reject=analysis-exception what=" << e.what() << "\n";
        }
        validation_cache.emplace(target, v);
        return v;
    };

    std::set<std::uint32_t> promoted;
    for (std::size_t start = 0u; start < word_count; ++start) {
        const auto end = std::min<std::size_t>(word_count, start + kWindowWords);
        const auto pointer_count = prefix[end] - prefix[start];
        if (pointer_count < kMinPointers) continue;

        std::set<std::uint32_t> candidates;
        for (std::size_t wi = start; wi < end; ++wi)
            if (local_words[wi]) candidates.insert(*local_words[wi]);
        if (candidates.size() < kMinPointers) continue;

        std::vector<std::uint32_t> clean;
        std::size_t strong = 0u;
        for (const auto target : candidates) {
            const auto v = validate(target);
            if (!v.clean) continue;
            clean.push_back(target);
            strong += v.strong ? 1u : 0u;
        }
        if (clean.size() < kMinClean) continue;

        // A sparse window must still look predominantly executable.  Three clean
        // entries are enough for small Katana state rows; larger windows need at
        // least half their unique local pointers to validate.  At least one strong
        // callable shape is required so a coincidental clean data run cannot anchor
        // a table by itself.
        if (candidates.size() > 5u && clean.size() * 2u < candidates.size()) continue;
        if (strong == 0u) continue;

        for (const auto target : clean) {
            promoted.insert(target);
            if (promoted.size() >= kMaxPromotions) break;
        }
        if (promoted.size() >= kMaxPromotions) break;
    }

    targets.assign(promoted.begin(), promoted.end());
    return targets;
}

enum class AddressTakenEvidenceStage : std::uint8_t { Candidate = 0u, Proven = 1u };

struct AddressTakenEvidence {
    std::uint32_t target{};
    std::size_t references{};
    std::size_t clustered_references{};
    std::size_t instructions{};
    std::size_t calls{};
    AddressTakenEvidenceStage stage{AddressTakenEvidenceStage::Candidate};
};

std::vector<AddressTakenEvidence> global_address_taken_evidence(
    const std::vector<std::uint8_t>& bytes, std::uint32_t base,
    const dcrecomp::Elf32Image& elf, const std::set<std::uint32_t>& known_entries,
    std::size_t promotion_budget) {
    struct Counts { std::size_t refs{}; std::size_t clustered{}; };
    std::map<std::uint32_t, Counts> counts;
    if (bytes.size() < 16u) return {};

    const auto local_pointer_at_offset = [&](std::size_t off) -> bool {
        if (off + 4u > bytes.size() || (off & 3u) != 0u) return false;
        const auto a = static_cast<std::uint32_t>(static_cast<std::uint64_t>(base) + off);
        return canonical_local_address(base, bytes.size(), raw32_at(bytes, base, a)).has_value();
    };

    // dcrecomp-style global address-taken scan, kept deliberately conservative:
    // only aligned words that occur repeatedly and live inside a small cluster of
    // other in-image code pointers become candidates.  A random scalar that happens
    // to look like 0x8Cxxxxxx therefore cannot open the closure by itself.
    for (std::size_t off = 0u; off + 4u <= bytes.size(); off += 4u) {
        const auto source = static_cast<std::uint32_t>(static_cast<std::uint64_t>(base) + off);
        const auto target = canonical_local_address(base, bytes.size(), raw32_at(bytes, base, source));
        if (!target) continue;
        auto& c = counts[*target];
        ++c.refs;
        std::size_t neighbours = 0u;
        const std::size_t begin = off > 16u ? off - 16u : 0u;
        const std::size_t end = std::min<std::size_t>(bytes.size() - 4u, off + 16u);
        for (std::size_t n = begin & ~std::size_t{3u}; n <= end; n += 4u)
            neighbours += local_pointer_at_offset(n) ? 1u : 0u;
        if (neighbours >= 3u) ++c.clustered;
    }

    std::vector<AddressTakenEvidence> evidence;
    evidence.reserve(counts.size());
    for (const auto& [target, c] : counts) {
        if (known_entries.contains(target) || c.refs < 2u || c.clustered < 2u) continue;
        evidence.push_back({target, c.refs, c.clustered, 0u, 0u, AddressTakenEvidenceStage::Candidate});
    }
    std::sort(evidence.begin(), evidence.end(), [](const auto& a, const auto& b) {
        if (a.clustered_references != b.clustered_references) return a.clustered_references > b.clustered_references;
        if (a.references != b.references) return a.references > b.references;
        return a.target < b.target;
    });

    std::size_t promoted = 0u;
    for (auto& e : evidence) {
        if (promoted >= promotion_budget) break;
        if (looks_like_pointer_table(bytes, base, e.target) ||
            looks_like_probable_ascii_data(bytes, base, e.target)) continue;

        // 0.0.203 containment: the global scan is a safety net, not a second
        // function finder.  Repeated pointer-looking words are cheap evidence in
        // retail assets, so require at least four clustered references and an
        // unmistakable callable shape before opening the static closure.  The
        // broader local-CFG/argument predicates remain available to anchored
        // call-site rules, where provenance is much stronger.
        if (e.references < 4u || e.clustered_references < 4u) continue;
        const bool callable_shape =
            looks_like_strong_abi_prologue(bytes, base, e.target, false) ||
            looks_like_compact_callable_entry(bytes, base, e.target);
        if (!callable_shape) continue;
        try {
            const auto fragment = dcrecomp::analyze_code_fragment_at(elf, e.target);
            e.instructions = fragment.instructions.size();
            e.calls = fragment.calls.size();
            if (fragment.unknown != 0u || fragment.instructions.size() < 4u) continue;
            e.stage = AddressTakenEvidenceStage::Proven;
            ++promoted;
        } catch (const std::exception&) {
            continue;
        }
    }
    return evidence;
}

std::vector<std::uint32_t> strided_callback_record_targets(const std::vector<std::uint8_t>& bytes,
                                                            std::uint32_t base,
                                                            std::uint32_t table_address) {
    std::vector<std::uint32_t> targets;
    if ((table_address & 3u) != 0u || table_address < base) return targets;

    // Katana render/driver tables can be arrays of 16-byte records rather than
    // contiguous vtables. The observed generic shape is:
    //   { scalar_flags0, scalar_flags1, callback0, callback1 }
    // and runtime code indexes the record before JSR'ing one callback field.
    // This is invisible to ordinary dense-pointer scanning. Require eight
    // consecutive records with two non-code scalar words followed by two
    // conservative local callable entries before promoting anything.
    constexpr std::size_t kRecordBytes = 16u;
    constexpr std::size_t kMinRecords = 8u;
    constexpr std::size_t kMaxRecords = 1024u;
    std::vector<std::uint32_t> run;
    std::size_t records = 0u;
    for (; records < kMaxRecords; ++records) {
        const auto rec64 = static_cast<std::uint64_t>(table_address) + records * kRecordBytes;
        if (rec64 + kRecordBytes > static_cast<std::uint64_t>(base) + bytes.size()) break;
        const auto rec = static_cast<std::uint32_t>(rec64);
        const std::uint32_t scalar0 = raw32_at(bytes, base, rec + 0u);
        const std::uint32_t scalar1 = raw32_at(bytes, base, rec + 4u);
        const auto cb0 = canonical_local_address(base, bytes.size(), raw32_at(bytes, base, rec + 8u));
        const auto cb1 = canonical_local_address(base, bytes.size(), raw32_at(bytes, base, rec + 12u));

        // A local pointer in either scalar slot means we have reached another
        // object/table family rather than the callback-record array.
        if (canonical_local_address(base, bytes.size(), scalar0) ||
            canonical_local_address(base, bytes.size(), scalar1) || !cb0 || !cb1) break;
        if (looks_like_pointer_table(bytes, base, *cb0) || looks_like_pointer_table(bytes, base, *cb1)) break;
        if (!looks_like_dense_dispatch_entry(bytes, base, *cb0) ||
            !looks_like_dense_dispatch_entry(bytes, base, *cb1)) break;
        run.push_back(*cb0);
        run.push_back(*cb1);
    }
    if (records < kMinRecords) {
        // 0.0.211: Katana also uses compact 12-byte callback descriptors:
        //   { scalar0, scalar1, callback }
        // Record of Lodoss War has long arrays of exactly this shape.  The callback
        // can legitimately point two bytes past a leading alignment NOP, so requiring
        // the target to be an already-known function start loses the real runtime
        // address (for example 0x8C02B580).  Keep this generic and conservative:
        // search only the first three word alignments relative to an anchored literal,
        // require at least eight consecutive records, reject local pointers in either
        // scalar field, and require every callback to have a decoder-clean callable
        // prefix.  This turns a proven data-table layout into exact dynamic entry roots
        // without scanning arbitrary in-image words as code.
        constexpr std::size_t kCompactRecordBytes = 12u;
        for (const std::uint32_t adjust : {0u, 4u, 8u}) {
            std::vector<std::uint32_t> compact_run;
            std::size_t compact_records = 0u;
            for (; compact_records < kMaxRecords; ++compact_records) {
                const auto rec64 = static_cast<std::uint64_t>(table_address) + adjust +
                                   compact_records * kCompactRecordBytes;
                if (rec64 + kCompactRecordBytes >
                    static_cast<std::uint64_t>(base) + bytes.size()) break;
                const auto rec = static_cast<std::uint32_t>(rec64);
                const std::uint32_t scalar0 = raw32_at(bytes, base, rec + 0u);
                const std::uint32_t scalar1 = raw32_at(bytes, base, rec + 4u);
                const auto cb = canonical_local_address(base, bytes.size(),
                                                        raw32_at(bytes, base, rec + 8u));
                if (canonical_local_address(base, bytes.size(), scalar0) ||
                    canonical_local_address(base, bytes.size(), scalar1) || !cb) break;
                if (looks_like_pointer_table(bytes, base, *cb) ||
                    looks_like_probable_ascii_data(bytes, base, *cb) ||
                    !looks_like_code_entry(bytes, base, *cb)) break;
                compact_run.push_back(*cb);
            }
            if (compact_records >= kMinRecords) {
                std::sort(compact_run.begin(), compact_run.end());
                compact_run.erase(std::unique(compact_run.begin(), compact_run.end()), compact_run.end());
                return compact_run;
            }
        }
        // 0.0.211 preliminary: another Katana state/method table uses 36-byte records:
        //   { cb0, cb1, cb2, scalar0, scalar1, scalar2, scalar3, scalar4, scalar5 }
        //
        // The runtime computes selector * 36 and loads one of the first callback
        // fields before an indirect JMP/JSR (Record of Lodoss War uses this exact
        // ABI shape).  Keep the rule generic: it is only attempted from an
        // executable PC-relative literal anchor, requires at least eight consecutive
        // records, all six scalar words must remain non-pointer values, and every
        // non-null callback must independently pass a conservative callable-entry
        // predicate.  Testing word-aligned adjustments lets a literal point at a
        // small scalar prefix immediately before the first record without teaching
        // the recompiler any title-specific address.
        constexpr std::size_t kMethod36RecordBytes = 36u;
        for (std::uint32_t adjust = 0u; adjust < kMethod36RecordBytes; adjust += 4u) {
            std::vector<std::uint32_t> method36_run;
            std::size_t method36_records = 0u;
            std::size_t method36_callbacks = 0u;
            bool method36_valid = true;
            for (; method36_records < kMaxRecords; ++method36_records) {
                const auto rec64 = static_cast<std::uint64_t>(table_address) + adjust +
                                   method36_records * kMethod36RecordBytes;
                if (rec64 + kMethod36RecordBytes >
                    static_cast<std::uint64_t>(base) + bytes.size()) break;
                const auto rec = static_cast<std::uint32_t>(rec64);

                for (std::uint32_t scalar_field = 12u; scalar_field < 36u; scalar_field += 4u) {
                    const auto scalar = raw32_at(bytes, base, rec + scalar_field);
                    if (canonical_local_address(base, bytes.size(), scalar)) {
                        method36_valid = false;
                        break;
                    }
                }
                if (!method36_valid) break;

                bool record_has_callback = false;
                for (const std::uint32_t field : {0u, 4u, 8u}) {
                    const auto raw = raw32_at(bytes, base, rec + field);
                    if (raw == 0u) continue;
                    const auto cb = canonical_local_address(base, bytes.size(), raw);
                    if (!cb || looks_like_pointer_table(bytes, base, *cb) ||
                        looks_like_probable_ascii_data(bytes, base, *cb) ||
                        (!looks_like_stored_callback_entry(bytes, base, *cb) &&
                         !looks_like_dense_dispatch_entry(bytes, base, *cb) &&
                         !looks_like_code_entry(bytes, base, *cb))) {
                        method36_valid = false;
                        break;
                    }
                    record_has_callback = true;
                    ++method36_callbacks;
                    method36_run.push_back(*cb);
                }
                if (!method36_valid || !record_has_callback) break;
            }
            // A mismatch after a completed run terminates this table family; it
            // does not invalidate the records already proven.
            if (method36_records >= kMinRecords && method36_callbacks >= 16u) {
                std::sort(method36_run.begin(), method36_run.end());
                method36_run.erase(std::unique(method36_run.begin(), method36_run.end()),
                                   method36_run.end());
                return method36_run;
            }
        }

        // 0.0.211: a second Katana descriptor family uses 32-byte records:
        //   { cb0, cb1, cb2, cb3, scalar0, scalar1, metadata_ptr, scalar2 }
        //
        // The record array is reached through a real PC-relative literal, so the
        // source address is already anchored by executable code.  Require a long
        // run (>=8 records), keep the scalar positions non-pointer-valued, require
        // a local metadata pointer in every record, and accept only callback fields
        // that independently look callable.  Null callback slots are allowed.
        // A 75% callable ratio across at least 12 non-null callback pointers keeps
        // this from turning arbitrary mixed object data into code while recovering
        // exact runtime entries selected from copied descriptor objects.
        constexpr std::size_t kWideRecordBytes = 32u;
        std::vector<std::uint32_t> wide_run;
        std::size_t wide_records = 0u;
        std::size_t records_with_callbacks = 0u;
        std::size_t callback_pointers = 0u;
        std::size_t callable_callbacks = 0u;
        for (; wide_records < kMaxRecords; ++wide_records) {
            const auto rec64 = static_cast<std::uint64_t>(table_address) +
                               wide_records * kWideRecordBytes;
            if (rec64 + kWideRecordBytes >
                static_cast<std::uint64_t>(base) + bytes.size()) break;
            const auto rec = static_cast<std::uint32_t>(rec64);
            const auto scalar0 = raw32_at(bytes, base, rec + 16u);
            const auto scalar1 = raw32_at(bytes, base, rec + 20u);
            const auto metadata = canonical_local_address(
                base, bytes.size(), raw32_at(bytes, base, rec + 24u));
            const auto scalar2 = raw32_at(bytes, base, rec + 28u);
            if (canonical_local_address(base, bytes.size(), scalar0) ||
                canonical_local_address(base, bytes.size(), scalar1) ||
                canonical_local_address(base, bytes.size(), scalar2) || !metadata) break;

            bool layout_ok = true;
            bool has_callback = false;
            for (const std::uint32_t field : {0u, 4u, 8u, 12u}) {
                const auto raw = raw32_at(bytes, base, rec + field);
                if (raw == 0u) continue;
                const auto cb = canonical_local_address(base, bytes.size(), raw);
                if (!cb) {
                    layout_ok = false;
                    break;
                }
                has_callback = true;
                ++callback_pointers;
                if (looks_like_pointer_table(bytes, base, *cb) ||
                    looks_like_probable_ascii_data(bytes, base, *cb)) continue;
                if (looks_like_stored_callback_entry(bytes, base, *cb) ||
                    looks_like_dense_dispatch_entry(bytes, base, *cb)) {
                    wide_run.push_back(*cb);
                    ++callable_callbacks;
                }
            }
            if (!layout_ok) break;
            if (has_callback) ++records_with_callbacks;
        }
        if (wide_records >= kMinRecords && records_with_callbacks >= 6u &&
            callback_pointers >= 12u && callable_callbacks * 4u >= callback_pointers * 3u) {
            std::sort(wide_run.begin(), wide_run.end());
            wide_run.erase(std::unique(wide_run.begin(), wide_run.end()), wide_run.end());
            return wide_run;
        }
        return targets;
    }
    std::sort(run.begin(), run.end());
    run.erase(std::unique(run.begin(), run.end()), run.end());
    return run;
}

std::vector<std::uint32_t> literal_scaled_index_dispatch_targets(
    const std::vector<std::uint8_t>& bytes, std::uint32_t base,
    const dcrecomp::Elf32Image& elf,
    const dcrecomp::FunctionAnalysis& analysis) {
    std::vector<std::uint32_t> targets;

    // Retail Katana code also uses arrays-of-records where the callback field is
    // selected as: literal table base + (selector << N), then JSR @loaded_target.
    // This differs from the compact 4-byte indexed tail thunk and from the older
    // fixed 16-byte callback-record detector: the record can be 32/64/etc bytes
    // and several mutually-exclusive literal bases may merge at one call site.
    // Recover the stride from the SHLL* sequence itself; no title addresses or
    // fixed callback offsets participate in the decision.
    using O = dcrecomp::sh4::Opcode;
    constexpr std::uint32_t kSearchBytes = 0x50u;
    constexpr std::size_t kMaxSlots = 192u;
    constexpr std::size_t kMinCallableSlots = 4u;

    const auto shift_bits = [](O op) -> unsigned {
        switch (op) {
            case O::Shll: return 1u;
            case O::Shll2: return 2u;
            case O::Shll8: return 8u;
            case O::Shll16: return 16u;
            default: return 0u;
        }
    };

    for (const auto& literal : analysis.literals) {
        if (literal.kind != dcrecomp::LiteralKind::Long32) continue;
        const auto table = canonical_local_address(base, bytes.size(), literal.value);
        if (!table || ((*table) & 3u) != 0u) continue;

        const auto def_it = std::find_if(analysis.instructions.begin(), analysis.instructions.end(),
            [&](const dcrecomp::sh4::Instruction& insn) {
                return insn.address == literal.instruction_address && insn.opcode == O::MovLPcRel;
            });
        if (def_it == analysis.instructions.end()) continue;
        const std::uint8_t table_reg = def_it->rn;

        // Accumulate selector shifts in a small window around the literal.
        // Katana often schedules SHLL2 immediately *before* the PC-relative table
        // load (SHLL2 Rn; MOV.L table,R0; MOV.L @(R0,Rn),Rt), so looking only
        // after the literal misses a valid bounded dispatch. Branch delay slots
        // are represented at their real PCs and remain covered by this window.
        std::array<unsigned, 16> shifts{};
        for (const auto& insn : analysis.instructions) {
            if (insn.address + 0x18u < literal.instruction_address ||
                insn.address > literal.instruction_address + 0x18u) continue;
            const unsigned bits = shift_bits(insn.opcode);
            if (bits != 0u && insn.rn < shifts.size()) shifts[insn.rn] += bits;
        }

        for (const auto& load : analysis.instructions) {
            if (load.opcode != O::MovLIndexedLoad ||
                load.address < literal.instruction_address ||
                load.address > literal.instruction_address + kSearchBytes) continue;

            // The PC-relative table base must still be live at the indexed load.
            // Reject an earlier literal when the same register is overwritten by
            // another MOV.L @(disp,PC),Rn before the load; CT2 uses exactly this
            // scheduling pattern for adjacent data and callback table literals.
            bool table_reg_redefined = false;
            for (const auto& mid : analysis.instructions) {
                if (mid.address <= literal.instruction_address || mid.address >= load.address) continue;
                if (mid.opcode == O::MovLPcRel && mid.rn == table_reg) {
                    table_reg_redefined = true;
                    break;
                }
            }
            if (table_reg_redefined) continue;

            // MOV.L @(R0,Rm),Rn has an implicit R0. Either the literal is R0
            // and Rm carries the shifted selector (the common Katana form), or
            // the literal is Rm and R0 carries the shifted selector.
            std::uint8_t selector_reg = 0xFFu;
            if (table_reg == 0u && load.rm != 0u) selector_reg = load.rm;
            else if (table_reg == load.rm) selector_reg = 0u;
            else continue;
            if (selector_reg >= shifts.size()) continue;
            const unsigned bits = shifts[selector_reg];
            if (bits < 2u || bits > 8u) continue;
            const std::uint32_t stride = 1u << bits;
            if (stride < 4u || (stride & 3u) != 0u) continue;

            // The loaded word must feed an indirect call/jump almost immediately;
            // otherwise a scaled structure lookup is merely ordinary data access.
            bool invoked = false;
            for (const auto& use : analysis.instructions) {
                if (use.address <= load.address || use.address > load.address + 8u) continue;
                if ((use.opcode == O::Jsr || use.opcode == O::Jmp) && use.rm == load.rn) {
                    invoked = true;
                    break;
                }
            }
            if (!invoked) continue;

            const auto callable_at = [&](std::int64_t slot64) -> std::optional<std::uint32_t> {
                const std::int64_t image_begin = static_cast<std::int64_t>(base);
                const std::int64_t image_end = image_begin + static_cast<std::int64_t>(bytes.size());
                if (slot64 < image_begin || slot64 + 4 > image_end) return std::nullopt;
                const auto slot = static_cast<std::uint32_t>(slot64);
                const auto target = canonical_local_address(base, bytes.size(), raw32_at(bytes, base, slot));
                if (!target || looks_like_pointer_table(bytes, base, *target) ||
                    !looks_like_dense_dispatch_entry(bytes, base, *target)) return std::nullopt;
                return *target;
            };

            std::vector<std::uint32_t> run;
            for (std::size_t slot_index = 0u; slot_index < kMaxSlots; ++slot_index) {
                const std::int64_t slot64 = static_cast<std::int64_t>(*table) +
                    static_cast<std::int64_t>(slot_index) * stride;
                const auto target = callable_at(slot64);
                if (!target) break;
                run.push_back(*target);
            }

            // A selector explicitly sign-extended before scaling can index records
            // *before* the literal anchor. This is common in SDK state/method tables:
            // several branch paths choose different field anchors in the same record
            // family, then a signed selector indexes the shared JSR site. Scan the
            // negative side only when EXTS.W proves signed indexing, and stop at the
            // first non-callable slot exactly like the positive side.
            bool signed_selector = false;
            for (const auto& insn : analysis.instructions) {
                if (insn.address + 8u < literal.instruction_address ||
                    insn.address > literal.instruction_address + 0x18u) continue;
                if (insn.opcode == O::ExtsW && insn.rn == selector_reg) {
                    signed_selector = true;
                    break;
                }
            }
            if (signed_selector) {
                std::vector<std::uint32_t> negative_run;
                for (std::size_t slot_index = 1u; slot_index <= kMaxSlots; ++slot_index) {
                    const std::int64_t slot64 = static_cast<std::int64_t>(*table) -
                        static_cast<std::int64_t>(slot_index) * stride;
                    const auto target = callable_at(slot64);
                    if (!target) break;
                    negative_run.push_back(*target);
                }
                if (negative_run.size() >= kMinCallableSlots)
                    run.insert(run.end(), negative_run.begin(), negative_run.end());
            }

            if (run.size() < kMinCallableSlots) {
                // Small bounded jump tables are common in retail SDK code but were
                // intentionally excluded by the generic >=4-entry density rule.
                // Accept a 2..16 entry table only when the selector is *proven* to
                // come from the same memory word that is checked as:
                //
                //   mov.l @rBase,rValue
                //   cmp/pz rValue
                //   ...
                //   mov #N,rBound
                //   cmp/ge rBound,rValue
                //   ...
                //   mov.l @rBase,rSelector
                //   shll2 rSelector
                //   mov.l @(r0,rSelector),rTarget
                //   jmp/jsr @rTarget
                //
                // That gives an explicit 0 <= selector < N proof and avoids
                // treating arbitrary two-word data as callbacks. CT2's returned
                // method at 0x8C06C35C uses exactly this compiler idiom.
                std::optional<std::uint8_t> selector_base;
                for (const auto& insn : analysis.instructions) {
                    if (insn.opcode == O::MovLLoad && insn.rn == selector_reg &&
                        insn.address + 0x20u >= literal.instruction_address &&
                        insn.address <= load.address) {
                        selector_base = insn.rm;
                    }
                }

                std::optional<std::uint32_t> proven_bound;
                bool argument_bound = false;
                if (selector_base) {
                    for (const auto& value_load : analysis.instructions) {
                        if (value_load.opcode != O::MovLLoad || value_load.rm != *selector_base ||
                            value_load.address + 0x20u < literal.instruction_address ||
                            value_load.address > load.address) continue;
                        const std::uint8_t value_reg = value_load.rn;

                        bool non_negative = false;
                        for (const auto& check : analysis.instructions) {
                            if (check.address < value_load.address || check.address > load.address) continue;
                            if (check.opcode == O::CmpPz && check.rn == value_reg) {
                                non_negative = true;
                                break;
                            }
                        }
                        if (!non_negative) continue;

                        for (const auto& cmp : analysis.instructions) {
                            if (cmp.opcode != O::CmpGe || cmp.rn != value_reg ||
                                cmp.address < value_load.address || cmp.address > load.address) continue;
                            for (const auto& bound_def : analysis.instructions) {
                                if (bound_def.opcode != O::MovImm || bound_def.rn != cmp.rm ||
                                    bound_def.address < value_load.address || bound_def.address > cmp.address) continue;
                                const auto imm = static_cast<std::int32_t>(bound_def.immediate);
                                if (imm >= 2 && imm <= 16) {
                                    proven_bound = static_cast<std::uint32_t>(imm);
                                }
                            }
                        }
                    }
                }

                // Argument-selected callback tables are useful (CT2 uses an exact
                // three-entry R4->R14 table), but 0.0.201 let a decoder-clean CFG
                // alone validate every slot.  On ChuChu that was enough to seed raw
                // data, whose accidental branches recursively opened thousands of
                // false functions.  Keep this provenance class much narrower:
                // direct ABI argument copy, explicit 0<=index<N, four-byte entries,
                // N<=8, and every target must have an unmistakable callable shape.
                if (!proven_bound) {
                    for (const auto& copy : analysis.instructions) {
                        if (copy.opcode != O::MovReg || copy.rn != selector_reg ||
                            copy.rm < 4u || copy.rm > 7u ||
                            copy.address + 0x20u < literal.instruction_address ||
                            copy.address > load.address) continue;

                        bool non_negative = false;
                        std::uint32_t cmp_pz_address = 0u;
                        for (const auto& check : analysis.instructions) {
                            if (check.address < copy.address || check.address > load.address) continue;
                            if (check.opcode == O::CmpPz && check.rn == selector_reg) {
                                non_negative = true;
                                cmp_pz_address = check.address;
                                break;
                            }
                        }
                        if (!non_negative) continue;

                        for (const auto& cmp : analysis.instructions) {
                            if (cmp.opcode != O::CmpGe || cmp.rn != selector_reg ||
                                cmp.address < cmp_pz_address || cmp.address > load.address) continue;
                            for (const auto& bound_def : analysis.instructions) {
                                if (bound_def.opcode != O::MovImm || bound_def.rn != cmp.rm ||
                                    bound_def.address < copy.address || bound_def.address > cmp.address) continue;
                                const auto imm = static_cast<std::int32_t>(bound_def.immediate);
                                if (imm >= 2 && imm <= 8 && stride == 4u) {
                                    proven_bound = static_cast<std::uint32_t>(imm);
                                    argument_bound = true;
                                    break;
                                }
                            }
                            if (proven_bound) break;
                        }
                        if (proven_bound) break;
                    }
                }

                if (!proven_bound) continue;
                run.clear();
                bool all_callable = true;
                for (std::uint32_t slot_index = 0u; slot_index < *proven_bound; ++slot_index) {
                    const std::int64_t slot64 = static_cast<std::int64_t>(*table) +
                        static_cast<std::int64_t>(slot_index) * stride;
                    if (!argument_bound) {
                        // Preserve the pre-0.0.201 bounded-memory rule: the table
                        // shape itself must satisfy the dense callable predicate.
                        const auto target = callable_at(slot64);
                        if (!target) { all_callable = false; break; }
                        run.push_back(*target);
                        continue;
                    }

                    const std::int64_t image_begin = static_cast<std::int64_t>(base);
                    const std::int64_t image_end = image_begin + static_cast<std::int64_t>(bytes.size());
                    if (slot64 < image_begin || slot64 + 4 > image_end) {
                        all_callable = false;
                        break;
                    }
                    const auto slot = static_cast<std::uint32_t>(slot64);
                    const auto target = canonical_local_address(base, bytes.size(), raw32_at(bytes, base, slot));
                    if (!target || looks_like_pointer_table(bytes, base, *target) ||
                        looks_like_probable_ascii_data(bytes, base, *target) ||
                        !(looks_like_strong_abi_prologue(bytes, base, *target, false) ||
                          looks_like_compact_callable_entry(bytes, base, *target))) {
                        all_callable = false;
                        break;
                    }
                    try {
                        const auto fragment = dcrecomp::analyze_code_fragment_at(elf, *target);
                        if (fragment.instructions.size() < 4u || fragment.unknown != 0u) {
                            all_callable = false;
                            break;
                        }
                    } catch (const std::exception&) {
                        all_callable = false;
                        break;
                    }
                    run.push_back(*target);
                }
                if (!all_callable || run.size() != *proven_bound) continue;
            }
            targets.insert(targets.end(), run.begin(), run.end());
        }
    }

    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    return targets;
}

dcrecomp::Elf32Image make_synthetic_elf(const std::vector<std::uint8_t>& bytes,
                                         std::uint32_t base,
                                         std::uint32_t entry,
                                         const std::set<std::uint32_t>& starts) {
    dcrecomp::Elf32Image elf;
    elf.type = 2u;       // ET_EXEC
    elf.machine = 42u;   // EM_SH
    elf.entry = entry;
    elf.bytes = bytes;

    dcrecomp::ElfSection section;
    section.name = ".raw_boot";
    section.type = 1u;   // SHT_PROGBITS
    section.flags = 0x2u | 0x4u; // SHF_ALLOC | SHF_EXECINSTR
    section.address = base;
    section.offset = 0u;
    section.size = static_cast<std::uint32_t>(bytes.size());
    elf.sections.push_back(section);

    std::vector<std::uint32_t> ordered(starts.begin(), starts.end());
    std::sort(ordered.begin(), ordered.end());
    const std::uint64_t image_end64 = static_cast<std::uint64_t>(base) + bytes.size();
    const std::uint32_t image_end = static_cast<std::uint32_t>(std::min<std::uint64_t>(image_end64, 0xFFFFFFFFull));
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        const auto start = ordered[i];
        if (!local_address(base, bytes.size(), start)) continue;
        // Raw commercial images do not carry authoritative ELF st_size values.
        // A discovered entry is therefore only an entry point, not a boundary:
        // compiler jump tables and hand-written assembly can expose overlapping
        // suffixes or long shared continuations whose real epilogue lies beyond
        // several other discovered entries. Keep the executable section as the
        // analysis envelope and let CFG reachability/real SH-4 terminators decide
        // where each path stops. The analyzer suppresses whole-range padding scans
        // for this synthetic .raw_boot section, so this does not turn into an
        // O(functions * image-size) padding walk.
        const auto end = image_end;
        dcrecomp::ElfSymbol symbol;
        symbol.name = synthetic_name(start);
        symbol.value = start;
        symbol.size = end > start ? end - start : 0u;
        symbol.info = static_cast<std::uint8_t>((1u << 4u) | 2u); // GLOBAL | FUNC
        symbol.section_index = 0u;
        elf.symbols.push_back(std::move(symbol));
    }
    return elf;
}

struct ClosureStats {
    std::size_t passes{};
    std::size_t analyzed_functions{};
    std::size_t resolved_calls{};
    std::size_t unresolved_calls{};
    std::size_t rejected_targets{};
    std::size_t inline_bsrf_thunks{};
    std::size_t literal_callback_targets{};
    std::size_t stored_callback_targets{};
    std::size_t callback_object_targets{};
    std::size_t anchored_dense_callback_targets{};
    std::size_t long_dense_callback_targets{};
    std::size_t clustered_callback_targets{};
    std::size_t strided_callback_targets{};
    std::size_t direct_literal_targets{};
    std::size_t cfg_literal_call_targets{};
    std::size_t returned_literal_targets{};
    std::size_t relocatable_code_templates{};
    std::size_t argument_callback_targets{};
    std::size_t promoted_cfg_fragments{};
    std::size_t vbr_vector_callbacks{};
    std::size_t sparse_callback_targets{};
    std::size_t dense_dispatch_targets{};
    std::size_t indexed_tail_dispatchers{};
    std::size_t indexed_tail_targets{};
    std::size_t scaled_index_dispatch_targets{};
    std::size_t branch_selected_call_targets{};
    std::size_t packed_bra_selector_targets{};
    std::size_t abi_gap_entries{};
    std::size_t rts_boundary_entries{};
    std::size_t rejected_dirty_fragments{};
    std::size_t rejected_dirty_sources{};
    std::size_t rejected_dirty_candidates{};
    std::size_t global_address_candidates{};
    std::size_t global_address_targets{};
    std::size_t closure_limit{};
    bool closure_cap_hit{};
    std::vector<AddressTakenEvidence> address_taken_evidence;
};

std::set<std::uint32_t> discover_function_closure(const std::vector<std::uint8_t>& bytes,
                                                   const Options& o,
                                                   ClosureStats& stats) {
    std::set<std::uint32_t> starts{o.entry};
    const std::size_t closure_limit = closure_entry_limit(o);
    stats.closure_limit = closure_limit;
    for (const auto requested : o.seed_entries) {
        if (const auto canonical = canonical_local_address(o.base, bytes.size(), requested)) {
            starts.insert(*canonical);
        }
    }
    for (std::size_t pass = 0; pass < o.closure_passes; ++pass) {
        ++stats.passes;
        std::cout << "[SH4 closure] pass " << (pass + 1u) << "/" << o.closure_passes
                  << " entries=" << starts.size() << "/" << closure_limit << std::endl;
        const auto elf = make_synthetic_elf(bytes, o.base, o.entry, starts);
        std::set<std::uint32_t> additions;
        std::set<std::uint32_t> covered_code_addresses;
        std::set<std::uint32_t> abi_gap_candidates;
        std::set<std::uint32_t> rts_boundary_candidates;

        for (const auto address : starts) {
            if (stats.analyzed_functions >= closure_limit * o.closure_passes) break;
            dcrecomp::FunctionAnalysis analysis;
            try {
                analysis = dcrecomp::analyze_function_at(elf, address);
            } catch (const std::exception&) {
                continue;
            }
            ++stats.analyzed_functions;
            for (const auto& insn : analysis.instructions) covered_code_addresses.insert(insn.address);

            // A speculative entry that decodes UNKNOWN is not authoritative code
            // and, crucially, must never be allowed to contribute new BSR/JSR or
            // table targets. One dirty seed can otherwise interpret asset bytes as
            // branches and recursively flood the closure. Clean historical ChuChu
            // closures are RAW_SH4=0, so fail closed here.
            if (analysis.unknown != 0u || analysis.instructions.empty()) {
                ++stats.rejected_dirty_sources;
                continue;
            }

            for (const auto& call : analysis.calls) {
                if (!call.resolved || call.target == 0u) {
                    ++stats.unresolved_calls;
                    continue;
                }
                ++stats.resolved_calls;
                const auto canonical = canonical_local_address(o.base, bytes.size(), call.target);
                if (!canonical || looks_like_pointer_table(bytes, o.base, *canonical)) {
                    ++stats.rejected_targets;
                    continue;
                }
                // Direct BSR/BSRF edges carry architectural executable evidence.
                // A constant-propagated JSR/JMP target is weaker because synthetic
                // shared-tail functions can merge register values from different CFG
                // entry paths. Require at least that the candidate begins with a real
                // decoded SH-4 instruction before promoting it. This deliberately does
                // not require a long clean prefix: legitimate tiny thunks and functions
                // followed immediately by literal pools remain valid.
                if (!call.direct) {
                    const auto first = dcrecomp::sh4::decode(raw16_at(bytes, o.base, *canonical), *canonical);
                    if (!dcrecomp::sh4::is_known(first)) {
                        ++stats.rejected_targets;
                        continue;
                    }
                }
                if (!starts.contains(*canonical)) additions.insert(*canonical);
            }

            // Dense indexed JSR/JMP tables are recovered by FunctionAnalysis as
            // dynamic targets even when their entries lie outside the current
            // synthetic function range. Promote those cross-function targets into
            // the raw closure only when the target retains a fully clean 32-byte
            // SH-4 prefix. This is intentionally stronger evidence than ordinary
            // literal scanning and does not require an early RTS/branch.
            // A local conditional can select one of several literal callback
            // pointers and join at a single JSR. Promote each proven predecessor
            // value as a callable closure entry after bounded executable validation.
            // This is separate from dynamic jump-table discovery and does not
            // broaden the generic table heuristics.
            for (const auto& dispatch : analysis.branch_selected_calls) {
                for (const auto target : dispatch.targets) {
                    const auto canonical = canonical_local_address(o.base, bytes.size(), target);
                    if (!canonical || starts.contains(*canonical) || additions.contains(*canonical)) continue;
                    if (looks_like_pointer_table(bytes, o.base, *canonical)) continue;
                    if (!(looks_like_clean_code_prefix(bytes, o.base, *canonical, 8u) ||
                          looks_like_local_cfg_callable_entry(bytes, o.base, *canonical) ||
                          looks_like_compact_callable_entry(bytes, o.base, *canonical))) continue;
                    additions.insert(*canonical);
                    ++stats.branch_selected_call_targets;
                }
            }

            for (const auto& dispatch : analysis.dynamic_branches) {
                for (const auto target : dispatch.targets) {
                    const auto canonical = canonical_local_address(o.base, bytes.size(), target);
                    if (!canonical || starts.contains(*canonical) || additions.contains(*canonical)) continue;
                    if (looks_like_pointer_table(bytes, o.base, *canonical)) continue;
                    if (looks_like_indexed_tail_dispatch_entry(bytes, o.base, *canonical)) {
                        additions.insert(*canonical);
                        ++stats.indexed_tail_dispatchers;
                        continue;
                    }
                    if (!looks_like_dense_dispatch_entry(bytes, o.base, *canonical)) continue;
                    additions.insert(*canonical);
                    ++stats.dense_dispatch_targets;
                }
            }

            // A packed BRA selector table is executable code rather than data:
            // consecutive tiny entry points tail-branch to one worker and encode
            // the selected case in the delay slot. Expand the family only from an
            // already-proven synthetic entry. Merely seeing a BRA/MOV pair as an
            // internal block of a larger function is not enough evidence to split
            // it into new callable entries. This distinction is what keeps nearby
            // switch code/data from opening speculative closure branches.
            for (const auto target : packed_bra_selector_family(bytes, o.base, address)) {
                if (!starts.contains(target) && !additions.contains(target)) {
                    additions.insert(target);
                    ++stats.packed_bra_selector_targets;
                }
            }

            // Compact indexed tail dispatchers load a selector, scale it by four,
            // fetch a target from a literal-fed table and JMP to that target. Once
            // the dispatcher is a proven callable entry, recover only the contiguous
            // prefix of executable pointers. The first non-local/non-code word ends
            // the table, preventing adjacent state/object data from being scanned.
            if (looks_like_indexed_tail_dispatch_entry(bytes, o.base, address)) {
                const auto table_load = dcrecomp::sh4::decode(
                    raw16_at(bytes, o.base, address + 2u), address + 2u);
                if (table_load.opcode == dcrecomp::sh4::Opcode::MovLPcRel &&
                    local_address(o.base, bytes.size(), table_load.effective_address)) {
                    const auto table_address = raw32_at(bytes, o.base, table_load.effective_address);
                    constexpr std::size_t kMaxIndexedTailTargets = 32u;
                    std::vector<std::uint32_t> table_targets;
                    for (std::size_t ti = 0u; ti < kMaxIndexedTailTargets; ++ti) {
                        const auto slot64 = static_cast<std::uint64_t>(table_address) + ti * 4u;
                        if (slot64 + 4u > static_cast<std::uint64_t>(o.base) + bytes.size()) break;
                        const auto slot = static_cast<std::uint32_t>(slot64);
                        const auto target = canonical_local_address(
                            o.base, bytes.size(), raw32_at(bytes, o.base, slot));
                        if (!target || looks_like_pointer_table(bytes, o.base, *target)) break;
                        const bool callable = *target == address ||
                            looks_like_clean_code_prefix(bytes, o.base, *target, 8u) ||
                            looks_like_indexed_tail_dispatch_entry(bytes, o.base, *target);
                        if (!callable) break;
                        table_targets.push_back(*target);
                    }
                    if (table_targets.size() >= 3u) {
                        for (const auto target : table_targets) {
                            if (!starts.contains(target) && !additions.contains(target)) {
                                additions.insert(target);
                                ++stats.indexed_tail_targets;
                            }
                        }
                    }
                }
            }

            // General record-stride dispatch: prove a PC-relative table literal,
            // derive selector stride from SHLL* instructions, prove the indexed
            // word feeds JSR/JMP, then promote only a contiguous run of callable
            // local targets. This is the structural counterpart to the 4-byte
            // indexed-tail rule above and covers 32/64-byte method records.
            for (const auto target : literal_scaled_index_dispatch_targets(bytes, o.base, elf, analysis)) {
                if (!starts.contains(target) && !additions.contains(target)) {
                    additions.insert(target);
                    ++stats.scaled_index_dispatch_targets;
                }
            }

            // Some retail IP.BIN paths use BSRF as a compact local dispatch into a
            // tightly packed family of tiny RTS thunks. Even when constant
            // propagation resolves one observed BSRF target, the same instruction can
            // choose a different thunk on another runtime iteration. Raw binaries
            // have no symbols to make those alternate entries visible. Recover only
            // the distinctive pattern: a BSRF followed nearby by at least three
            // 3-word (op; RTS; delay-slot) helpers. The density requirement avoids
            // turning arbitrary epilogues elsewhere into synthetic entries.
            for (const auto& insn : analysis.instructions) {
                if (insn.opcode != dcrecomp::sh4::Opcode::Bsrf) continue;

                std::vector<std::uint32_t> nearby;
                const std::uint64_t scan_begin64 = static_cast<std::uint64_t>(insn.address) + 4u;
                const std::uint64_t scan_end64 = std::min<std::uint64_t>(
                    static_cast<std::uint64_t>(insn.address) + 0x80u,
                    static_cast<std::uint64_t>(o.base) + bytes.size());
                for (std::uint64_t a64 = scan_begin64; a64 + 6u <= scan_end64; a64 += 2u) {
                    const auto addr = static_cast<std::uint32_t>(a64);
                    if (!local_address(o.base, bytes.size(), addr) ||
                        !local_address(o.base, bytes.size(), addr + 4u)) continue;
                    const auto off = static_cast<std::size_t>(addr - o.base);
                    const auto raw0 = static_cast<std::uint16_t>(bytes[off]) |
                                      (static_cast<std::uint16_t>(bytes[off + 1u]) << 8u);
                    const auto raw1 = static_cast<std::uint16_t>(bytes[off + 2u]) |
                                      (static_cast<std::uint16_t>(bytes[off + 3u]) << 8u);
                    const auto raw2 = static_cast<std::uint16_t>(bytes[off + 4u]) |
                                      (static_cast<std::uint16_t>(bytes[off + 5u]) << 8u);
                    const auto op0 = dcrecomp::sh4::decode(raw0, addr);
                    const auto op1 = dcrecomp::sh4::decode(raw1, addr + 2u);
                    const auto op2 = dcrecomp::sh4::decode(raw2, addr + 4u);
                    if (op1.opcode == dcrecomp::sh4::Opcode::Rts &&
                        dcrecomp::sh4::is_known(op0) && dcrecomp::sh4::is_known(op2)) {
                        nearby.push_back(addr);
                    }
                }

                if (nearby.size() < 3u) continue;
                for (const auto target : nearby) {
                    if (!starts.contains(target) && !additions.contains(target)) {
                        additions.insert(target);
                        ++stats.inline_bsrf_thunks;
                    }
                }
            }

            // The most direct address-taken call idiom in retail code is a
            // PC-relative literal loaded into a register and invoked immediately
            // with JSR/JMP @Rn. Symbol-free images have no relocation telling us
            // that the literal is executable, so promote only exact short-range
            // load->invoke pairs whose target passes the conservative code-entry
            // predicate. This is generic and avoids seeding individual game PCs.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel || def.rn > 14u) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value);
                if (!canonical) continue;
                const bool immediate_proof = literal_register_is_invoked_immediately(bytes, o.base, def.address, def.rn);
                const bool cfg_proof = !immediate_proof &&
                    literal_value_reaches_indirect_call_on_local_path(bytes, o.base, def.address, def.rn);
                if (!immediate_proof && !cfg_proof) continue;

                // 0.0.207: an actual PC-relative literal value proven to reach
                // JSR/JMP is stronger executable-entry evidence than the generic
                // pointer-density heuristic. Tiny Katana leaf callbacks commonly
                // end in RTS after two setup instructions and place their literal
                // pool immediately afterwards; viewed as aligned u32 words that
                // pool can look like a pointer table even though the entry is
                // unambiguously invoked as code. Keep rejecting pointer-table-like
                // targets unless they also have the strict compact-callable shape.
                if (looks_like_pointer_table(bytes, o.base, *canonical) &&
                    !looks_like_compact_callable_entry(bytes, o.base, *canonical)) continue;

                // A proven literal value reaching an architectural JSR/JMP is
                // stronger evidence than an arbitrary code-looking pointer. The
                // short legacy path retains its stricter entry predicate; the CFG
                // provenance path only requires a real decoded first instruction
                // and non-text bytes so compact SDK helpers/literal-tail veneers are
                // not rejected solely because their pool begins early.
                bool callable = looks_like_argument_callback_entry(bytes, o.base, *canonical) ||
                                (looks_like_clean_code_prefix(bytes, o.base, *canonical, 32u) &&
                                 !looks_like_probable_ascii_data(bytes, o.base, *canonical));
                if (!callable && cfg_proof) {
                    const auto first = dcrecomp::sh4::decode(raw16_at(bytes, o.base, *canonical), *canonical);
                    callable = dcrecomp::sh4::is_known(first) &&
                               !looks_like_probable_ascii_data(bytes, o.base, *canonical);
                }
                if (!callable) continue;
                if (!starts.contains(*canonical) && !additions.contains(*canonical)) {
                    additions.insert(*canonical);
                    ++stats.direct_literal_targets;
                    if (cfg_proof) ++stats.cfg_literal_call_targets;
                }
            }

            // A compact and very common factory/getter idiom returns a function
            // pointer directly in R0:
            //   mov.l @(literal,pc),r0
            //   rts
            //   nop
            // The value is not invoked inside the getter, so the normal literal->JSR
            // provenance rules cannot see it.  Exact MOV.L/R0 + RTS + NOP is strong
            // enough evidence to promote a local, decoded target without scanning
            // arbitrary literal pools. This recovers address-taken callbacks such as
            // CT2's 0x8C06C35C while remaining title-agnostic.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel || def.rn != 0u) continue;
                if (!local_address(o.base, bytes.size(), def.address + 4u)) continue;
                const auto ret = dcrecomp::sh4::decode(raw16_at(bytes, o.base, def.address + 2u), def.address + 2u);
                const auto delay = dcrecomp::sh4::decode(raw16_at(bytes, o.base, def.address + 4u), def.address + 4u);
                if (ret.opcode != dcrecomp::sh4::Opcode::Rts || delay.opcode != dcrecomp::sh4::Opcode::Nop) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value);
                if (!canonical || looks_like_pointer_table(bytes, o.base, *canonical) ||
                    looks_like_probable_ascii_data(bytes, o.base, *canonical)) continue;
                const auto first = dcrecomp::sh4::decode(raw16_at(bytes, o.base, *canonical), *canonical);
                if (!dcrecomp::sh4::is_known(first)) continue;
                if (!starts.contains(*canonical) && !additions.contains(*canonical)) {
                    additions.insert(*canonical);
                    ++stats.returned_literal_targets;
                }
            }

            // Retail Katana code frequently keeps callback/function pointers in
            // callee-saved R8-R14 registers across unrelated calls, then invokes
            // them later with JSR @Rn/JMP @Rn. A per-basic-block constant pass can
            // see only the load, not the eventual dynamic call. Promote literal-fed
            // local code pointers loaded into those preserved registers so runtime
            // dispatch has a registered native target without game-specific seeds.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel || def.rn < 8u || def.rn > 14u) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value);
                if (!canonical || looks_like_pointer_table(bytes, o.base, *canonical)) continue;
                if (!preserved_register_is_invoked_soon(bytes, o.base, def.address, def.rn) &&
                    !preserved_register_is_invoked_on_local_path(bytes, o.base, def.address, def.rn)) continue;
                if (!looks_like_code_entry(bytes, o.base, *canonical)) continue;
                if (!starts.contains(*canonical) && !additions.contains(*canonical)) {
                    additions.insert(*canonical);
                    ++stats.literal_callback_targets;
                }
            }

            // Driver/ops-table initializers can store local function literals
            // directly into writable callback tables. Promote a target only when
            // the literal register reaches a nearby 32-bit store unchanged and
            // the target itself satisfies the strict callback-entry predicate.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel || def.rn > 14u) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value);
                if (!canonical || looks_like_pointer_table(bytes, o.base, *canonical)) continue;
                if (!literal_register_is_stored_soon(bytes, o.base, def.address, def.rn)) continue;
                if (!looks_like_stored_callback_entry(bytes, o.base, *canonical)) continue;
                if (!starts.contains(*canonical) && !additions.contains(*canonical)) {
                    additions.insert(*canonical);
                    ++stats.stored_callback_targets;
                }
            }

            // Callback installers often pass a function pointer as R4-R7 to a
            // registration helper. In raw binaries there is no relocation/symbol
            // record for that address-taken function. Promote only strict code-looking
            // literals that survive unchanged until the immediately following call
            // or ABI-preserving JMP tail-call into the registration helper.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel || def.rn < 4u || def.rn > 7u) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value);
                if (!canonical || looks_like_pointer_table(bytes, o.base, *canonical)) continue;
                const bool callable_argument = looks_like_argument_callback_entry(bytes, o.base, *canonical) ||
                                               looks_like_saved_gpr_tail_bra_entry(bytes, o.base, *canonical) ||
                                               looks_like_local_cfg_callable_entry(bytes, o.base, *canonical) ||
                                               (looks_like_clean_code_prefix(bytes, o.base, *canonical, 32u) &&
                                                !looks_like_probable_ascii_data(bytes, o.base, *canonical));
                if (!callable_argument) continue;
                const bool reaches_soon = argument_register_reaches_call_soon(bytes, o.base, def.address, def.rn);
                const bool reaches_local_path = !reaches_soon &&
                    argument_register_reaches_call_on_local_path(bytes, o.base, def.address, def.rn);
                if (!reaches_soon && !reaches_local_path) continue;
                // A branch-following proof spans substantially more bytes than the
                // legacy near-call check, so pair it with one of the structural
                // callable-entry predicates. A merely clean prefix is not strong
                // enough at that distance: retail data can decode as plausible SH-4.
                if (reaches_local_path &&
                    !looks_like_argument_callback_entry(bytes, o.base, *canonical) &&
                    !looks_like_saved_gpr_tail_bra_entry(bytes, o.base, *canonical) &&
                    !looks_like_local_cfg_callable_entry(bytes, o.base, *canonical)) continue;
                if (!starts.contains(*canonical) && !additions.contains(*canonical)) {
                    additions.insert(*canonical);
                    ++stats.argument_callback_targets;
                }
            }

            // Some Katana interrupt vectors contain a function pointer rather than
            // copied code. Recognize literal function addresses stored into a VBR-relative
            // slot so the later dynamic dispatcher has a registered native target.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value);
                if (!canonical || looks_like_pointer_table(bytes, o.base, *canonical) ||
                    !looks_like_argument_callback_entry(bytes, o.base, *canonical)) continue;
                if (!literal_target_is_installed_in_vbr_vector(bytes, o.base, def.address, def.rn)) continue;
                if (!starts.contains(*canonical) && !additions.contains(*canonical)) {
                    additions.insert(*canonical);
                    ++stats.vbr_vector_callbacks;
                }
            }

            // Katana installs exception/interrupt handlers by copying compact code
            // templates from the executable into VBR-relative RAM. Those copied
            // addresses do not exist in the static image, so compile the literal-fed
            // source templates when a nearby VBR setup performs a post-increment
            // copy from the same register. Runtime relocation matching can then bind
            // the copied destination back to this native template.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value);
                if (!canonical || looks_like_pointer_table(bytes, o.base, *canonical) ||
                    !looks_like_callback_entry(bytes, o.base, *canonical)) continue;
                if (!literal_source_is_copied_from_vbr_setup(bytes, o.base, def.address, def.rn)) continue;
                if (!starts.contains(*canonical) && !additions.contains(*canonical)) {
                    additions.insert(*canonical);
                    ++stats.relocatable_code_templates;
                }
            }

            // Some retail bootstraps copy a complete executable helper to a
            // fixed main-RAM address with an indexed byte loop and then jump to
            // that relocated address. Unlike the VBR installer above, the code
            // is not associated with VBR at all (Record of Lodoss War is one
            // example), so discover the source template from the copy itself.
            //
            // Internal absolute references in such a template are linked for
            // the *destination* address. Map only those referenced entrypoints
            // back to the source copy and require a clean SH-4 analysis before
            // promoting them. This avoids treating literal/data fields in the
            // copied blob as functions while still compiling every entrypoint
            // that the relocated helper can dispatch to.
            for (const auto& def : analysis.instructions) {
                if (def.opcode != dcrecomp::sh4::Opcode::MovLPcRel) continue;
                const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                    [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                if (lit == analysis.literals.end()) continue;
                const auto copy = indexed_relocated_copy_from_literal(bytes, o.base, analysis, *lit, def.rn);
                if (!copy) continue;

                bool promoted_template = false;
                if (!starts.contains(copy->source) && !additions.contains(copy->source)) {
                    additions.insert(copy->source);
                    promoted_template = true;
                }

                const std::uint32_t destination_phys = copy->destination & 0x1FFFFFFFu;
                const std::uint64_t source_end64 = static_cast<std::uint64_t>(copy->source) + copy->size;
                const std::uint32_t source_end = static_cast<std::uint32_t>(source_end64);
                for (std::uint32_t offset = 0u; offset + 4u <= copy->size; offset += 2u) {
                    const std::uint32_t word_address = copy->source + offset;
                    if (!local_address(o.base, bytes.size(), word_address) ||
                        static_cast<std::uint64_t>(word_address - o.base) + 4u > bytes.size()) continue;
                    const std::uint32_t value = raw32_at(bytes, o.base, word_address);
                    const std::uint32_t value_phys = value & 0x1FFFFFFFu;
                    if (value_phys < destination_phys ||
                        value_phys >= destination_phys + copy->size) continue;
                    const std::uint32_t candidate = copy->source + (value_phys - destination_phys);
                    if ((candidate & 1u) != 0u || candidate < copy->source || candidate >= source_end) continue;
                    if (starts.contains(candidate) || additions.contains(candidate)) continue;

                    dcrecomp::FunctionAnalysis relocated_entry;
                    try {
                        auto probe_starts = starts;
                        probe_starts.insert(candidate);
                        const auto probe_elf = make_synthetic_elf(bytes, o.base, o.entry, probe_starts);
                        relocated_entry = dcrecomp::analyze_function_at(probe_elf, candidate);
                    } catch (const std::exception&) {
                        continue;
                    }
                    if (relocated_entry.instructions.empty() || relocated_entry.unknown != 0u) continue;
                    const bool stays_inside_copy = std::all_of(
                        relocated_entry.instructions.begin(), relocated_entry.instructions.end(),
                        [&](const dcrecomp::sh4::Instruction& insn) {
                            return insn.address >= copy->source && insn.address < source_end;
                        });
                    if (!stays_inside_copy) continue;
                    additions.insert(candidate);
                }

                if (promoted_template) ++stats.relocatable_code_templates;
            }

            // A second common symbol-free pattern is a literal pointer to a small
            // callback object/vtable. The caller loads the object, then fetches one
            // member with MOV.L @(disp,Rn), and finally performs JSR/JMP @Rm. Unlike
            // the preserved-register case above, the literal names data rather than
            // code, so discover dense callback runs at the beginning of referenced
            // objects. The 3-entry minimum keeps this conservative and generic.
            for (const auto& literal : analysis.literals) {
                const auto object = canonical_local_address(o.base, bytes.size(), literal.value);
                if (!object) continue;
                for (const auto target : callback_object_targets(bytes, o.base, *object)) {
                    const bool already_local_block = std::any_of(analysis.instructions.begin(), analysis.instructions.end(),
                        [&](const dcrecomp::sh4::Instruction& insn) { return insn.address == target; });
                    if (already_local_block) continue;
                    if (!starts.contains(target) && !additions.contains(target)) {
                        additions.insert(target);
                        ++stats.callback_object_targets;
                    }
                }
            

                // Some Katana subsystems use arrays of 16-byte records with two
                // scalar flag words followed by two callback fields. A PC-relative
                // literal to the record-array base is strong data evidence, while the
                // repeated callable fields prove the executable targets. This covers
                // runtime-selected render/gameplay methods without title-specific seeds.
                for (const auto target : strided_callback_record_targets(bytes, o.base, *object)) {
                    if (!starts.contains(target) && !additions.contains(target)) {
                        additions.insert(target);
                        ++stats.strided_callback_targets;
                    }
                }

                // Some linker/SDK init lists expose a pointer cell as the literal;
                // that cell points at a sparse array of function pointers separated
                // only by null/sentinel words. Follow exactly one data indirection
                // and promote the callable entries when the whole prefix has the
                // expected sparse-table shape.
                if ((*object & 3u) == 0u && static_cast<std::uint64_t>(*object) + 4u <=
                    static_cast<std::uint64_t>(o.base) + bytes.size()) {
                    const auto nested_raw = raw32_at(bytes, o.base, *object);
                    if (const auto nested = canonical_local_address(o.base, bytes.size(), nested_raw)) {
                        for (const auto target : sparse_callback_table_targets(bytes, o.base, *nested)) {
                            if (!starts.contains(target) && !additions.contains(target)) {
                                additions.insert(target);
                                ++stats.sparse_callback_targets;
                            }
                        }
                    }
                }
            }

            // The retail IP.BIN bootstrap uses an uncached continuation idiom:
            //   mov.l @(literal,pc),Rn   ; continuation in P2 (0xAC...)
            //   lds Rn,PR
            //   jmp @Rm                 ; helper eventually RTS to that PR
            // In a normal ELF the continuation may already have a symbol, but
            // a raw commercial image has no symbols. Recover literal-fed PR
            // continuations as synthetic entries so bootstrap chains do not
            // incorrectly return to the host at the first helper boundary.
            for (std::size_t ii = 0; ii < analysis.instructions.size(); ++ii) {
                const auto& insn = analysis.instructions[ii];
                if (insn.opcode != dcrecomp::sh4::Opcode::LdsPr) continue;
                for (std::size_t back = 1u; back <= 4u && back <= ii; ++back) {
                    const auto& def = analysis.instructions[ii - back];
                    if (def.opcode == dcrecomp::sh4::Opcode::MovLPcRel && def.rn == insn.rm) {
                        const auto lit = std::find_if(analysis.literals.begin(), analysis.literals.end(),
                            [&](const dcrecomp::LiteralReference& r) { return r.instruction_address == def.address; });
                        if (lit != analysis.literals.end()) {
                            if (const auto canonical = canonical_local_address(o.base, bytes.size(), lit->value)) {
                                if (!starts.contains(*canonical) && !looks_like_pointer_table(bytes, o.base, *canonical))
                                    additions.insert(*canonical);
                            }
                        }
                        break;
                    }
                }
            }
            // Symbol-less retail images frequently place a complete function after
            // the predecessor's terminal delay slot and its PC-relative literal pool.
            // Do not promote anything here yet: raw synthetic entries overlap heavily,
            // so another already-proven function may cover this address.  Collect only
            // structural proposals during the pass and filter them against the union of
            // all reachable instruction addresses after every current entry is analyzed.
            using O = dcrecomp::sh4::Opcode;
            if (analysis.instructions.size() >= 2u) {
                const std::uint32_t last_reachable = analysis.instructions.back().address;
                const dcrecomp::sh4::Instruction* terminal = nullptr;
                for (const auto& insn : analysis.instructions) {
                    if (insn.address + 2u != last_reachable) continue;
                    if (insn.opcode == O::Rts || insn.opcode == O::Rte ||
                        insn.opcode == O::Jmp || insn.opcode == O::Braf ||
                        insn.opcode == O::Bra) terminal = &insn;
                }
                if (terminal && !(terminal->opcode == O::Bra && terminal->target >= analysis.start_address)) {
                    const auto gap_begin = terminal->address + 4u; // terminal + delay slot

                    // An immediately adjacent ABI prologue is a particularly strong
                    // function boundary.  Accept GPR prologues and the common SH-4 FPU
                    // leaf form that preserves at least two FR12-FR15 registers plus PR.
                    // This is kept separate from the literal-gap rule below so data-pool
                    // scanning never has to be relaxed to recover adjacent functions.
                    if (terminal->opcode == O::Rts) {
                        // A normal GPR ABI prologue can begin immediately after the
                        // predecessor's RTS delay slot with no literal pool between
                        // the two functions.  This is common in Katana retail code
                        // and is strong enough evidence to promote the exact boundary
                        // (R8-R14 save + PR save) without scanning arbitrary bytes.
                        // Crazy Taxi 2 exposed this at 0x8C16BFB6, directly after a
                        // 12-byte function rooted at 0x8C16BFAA.
                        if (looks_like_strong_abi_prologue(bytes, o.base, gap_begin, false)) {
                            abi_gap_candidates.insert(gap_begin);
                        } else if (looks_like_fpu_only_abi_prologue(bytes, o.base, gap_begin)) {
                            rts_boundary_candidates.insert(gap_begin);
                        }
                    }

                    // For a post-terminal literal/data pool, use the end of the furthest
                    // *referenced* literal as the anchor.  Only the first strong ABI
                    // prologue within 16 bytes of that boundary is proposed.  This is a
                    // bounded structural rule, not a linear executable-data scan.
                    std::uint32_t literal_end = 0u;
                    for (const auto& literal : analysis.literals) {
                        const std::uint32_t literal_size =
                            literal.kind == dcrecomp::LiteralKind::Long32 ? 4u : 2u;
                        if (literal.storage_address < gap_begin ||
                            literal.storage_address >= gap_begin + 0x100u) continue;
                        literal_end = std::max(literal_end, literal.storage_address + literal_size);
                    }
                    if (literal_end > gap_begin) {
                        const auto search_end64 = std::min<std::uint64_t>(
                            static_cast<std::uint64_t>(literal_end) + 0x10u,
                            static_cast<std::uint64_t>(o.base) + bytes.size());
                        const auto search_end = static_cast<std::uint32_t>(search_end64);
                        for (std::uint32_t candidate = (literal_end + 1u) & ~1u;
                             candidate <= search_end; candidate += 2u) {
                            if (!looks_like_strong_abi_prologue(bytes, o.base, candidate, false)) continue;
                            abi_gap_candidates.insert(candidate);
                            break;
                        }
                    }
                }
            }
        }

        // Global-but-anchored vtable recovery.  This runs after every currently
        // known entry has been analyzed so the table must be supported by the
        // complete closure state of this pass.  Address-taken callbacks are
        // allowed to be internal to another synthetic raw function: an explicit
        // function pointer is executable-entry evidence even when overlapping
        // AOT analysis already covers the same bytes.
        {
            std::set<std::uint32_t> anchored_entries = starts;
            anchored_entries.insert(additions.begin(), additions.end());
            for (const auto target : anchored_dense_callback_table_targets(bytes, o.base, anchored_entries)) {
                if (starts.contains(target) || additions.contains(target)) continue;
                try {
                    const auto fragment = dcrecomp::analyze_code_fragment_at(elf, target);
                    if (fragment.instructions.empty() || fragment.unknown != 0u) continue;
                } catch (const std::exception&) {
                    continue;
                }
                additions.insert(target);
                ++stats.anchored_dense_callback_targets;
            }
        }


        // Recover long SDK interface/vtable arrays even when no member is in the
        // initial closure. The table shape itself is the anchor: >=16 consecutive
        // local pointers and >=75% decoder-clean callable entries.
        if (pass == 0u) {
            for (const auto target : long_dense_callback_table_targets(bytes, o.base)) {
                if (starts.contains(target) || additions.contains(target)) continue;
                try {
                    const auto fragment = dcrecomp::analyze_code_fragment_at(elf, target);
                    if (fragment.instructions.empty() || fragment.unknown != 0u) continue;
                } catch (const std::exception&) {
                    continue;
                }
                additions.insert(target);
                ++stats.long_dense_callback_targets;
            }
        }

        // 0.1.0: sparse/clustered callback tables. Unlike the long dense-vtable
        // rule above, this allows metadata/null gaps and unique one-reference
        // handlers, but every promoted destination must independently decode as a
        // clean callable fragment. This replaces the need for title-specific seed
        // lists for state-machine dispatch tables.
        if (pass == 0u) {
            for (const auto target : clustered_clean_callback_table_targets(bytes, o.base, elf)) {
                if (starts.contains(target) || additions.contains(target)) continue;
                additions.insert(target);
                ++stats.clustered_callback_targets;
            }
        }

        // 0.0.202: global address-taken proof. dcrecomp demonstrated that many
        // retail callbacks exist only as function pointers in static data/vtables;
        // no nearby JSR literal necessarily exposes them. Scan repeated clustered
        // pointer values once, then promote only candidates with an independent
        // callable entry shape and a completely decoder-clean CFG. This is the
        // generic rule that recovers CT2 0x8C03650C (16 table references, 215/215
        // known SH-4) without turning every in-range scalar into executable code.
        if (pass == 0u) {
            std::set<std::uint32_t> known_entries = starts;
            known_entries.insert(additions.begin(), additions.end());
            const std::size_t budget = std::min<std::size_t>(128u,
                std::max<std::size_t>(32u, closure_limit / 64u));
            auto evidence = global_address_taken_evidence(bytes, o.base, elf, known_entries, budget);
            for (const auto& e : evidence) {
                ++stats.global_address_candidates;
                if (e.stage != AddressTakenEvidenceStage::Proven) continue;
                if (!starts.contains(e.target) && !additions.contains(e.target)) {
                    additions.insert(e.target);
                    ++stats.global_address_targets;
                }
            }
            stats.address_taken_evidence = std::move(evidence);
        }

        // Filter structural proposals against the complete code coverage of the
        // current closure pass.  This is critical for raw images: many synthetic
        // entries overlap and a byte sequence that appears to be "after" one entry
        // can already be a proven internal block of another.  Only genuinely
        // uncovered candidates are promoted, and each must decode to a clean CFG.
        auto promote_structural_entry = [&](std::uint32_t candidate, bool rts_boundary) -> bool {
            if (starts.contains(candidate) || additions.contains(candidate) ||
                covered_code_addresses.contains(candidate)) return false;
            if (looks_like_pointer_table(bytes, o.base, candidate) ||
                looks_like_probable_ascii_data(bytes, o.base, candidate)) return false;
            try {
                const auto fragment = dcrecomp::analyze_code_fragment_at(elf, candidate);
                if (fragment.unknown != 0u) return false;
                if (rts_boundary) {
                    if (fragment.instructions.size() < 32u || fragment.calls.size() < 3u) return false;
                } else if (fragment.instructions.size() < 8u) {
                    return false;
                }
            } catch (const std::exception&) {
                return false;
            }
            additions.insert(candidate);
            if (rts_boundary) ++stats.rts_boundary_entries;
            else ++stats.abi_gap_entries;
            return true;
        };
        std::size_t abi_promoted_this_pass = 0u;
        for (const auto candidate : abi_gap_candidates)
            abi_promoted_this_pass += promote_structural_entry(candidate, false) ? 1u : 0u;
        // RTS-boundary promotion is intentionally deferred until the ABI-gap
        // closure reaches a fixed point.  This prevents adjacent-function guesses
        // from changing which post-literal gaps are already proven code.
        if (abi_promoted_this_pass == 0u) {
            for (const auto candidate : rts_boundary_candidates) promote_structural_entry(candidate, true);
        }

        // ProgramAnalysis can discover reachable cross-symbol CFG fragments that are
        // not part of the raw synthetic-entry set yet. Promote those proven reachable
        // fragment entries back into the symbol-free closure. On the following pass,
        // the regular literal/callback rules analyze the fragment as a first-class
        // function, allowing address-taken callees referenced only from that fragment
        // to be discovered without scanning unrelated data as code.
        {
            dcrecomp::ProgramAnalysisOptions preview_options;
            preview_options.max_functions = o.max_functions;
            preview_options.native_override_symbols.clear();
            for (const auto seed : starts) {
                if (seed != o.entry) preview_options.seed_addresses.push_back(seed);
            }
            try {
                const auto preview = dcrecomp::analyze_reachable_program(
                    elf, synthetic_name(o.entry), preview_options);
                for (const auto& fn : preview.functions) {
                    const auto fragment = fn.analysis.start_address;
                    if (!local_address(o.base, bytes.size(), fragment) ||
                        starts.contains(fragment) || additions.contains(fragment)) continue;
                    // A raw synthetic section spans code and data. Cross-symbol CFG
                    // fragments are useful only when the recovered fragment itself
                    // is decoder-clean; otherwise ProgramAnalysis may have followed
                    // an accidental branch-shaped word into asset/string data.
                    if (fn.analysis.unknown != 0u || fn.analysis.instructions.empty()) {
                        ++stats.rejected_dirty_fragments;
                        continue;
                    }
                    additions.insert(fragment);
                    ++stats.promoted_cfg_fragments;
                }
            } catch (const std::exception&) {
                // The normal closure can still make progress even if a preview hits
                // the configured function cap. Keep this pass conservative.
            }
        }

        if (additions.empty()) break;

        // Final admission gate for every newly discovered entry, independent of
        // which heuristic produced it. This makes closure growth monotonic in
        // *decoder-clean executable evidence* instead of in pointer-like data.
        // It is intentionally redundant with stronger per-rule predicates: the
        // redundancy is what prevents one false-positive rule from cascading.
        std::set<std::uint32_t> clean_additions;
        for (const auto address : additions) {
            try {
                const auto fragment = dcrecomp::analyze_code_fragment_at(elf, address);
                if (fragment.unknown != 0u || fragment.instructions.empty()) {
                    ++stats.rejected_dirty_candidates;
                    continue;
                }
                clean_additions.insert(address);
            } catch (const std::exception&) {
                ++stats.rejected_dirty_candidates;
            }
        }
        if (clean_additions.empty()) break;
        for (const auto address : clean_additions) {
            if (starts.size() >= closure_limit) { stats.closure_cap_hit = true; break; }
            starts.insert(address);
        }
        if (starts.size() >= closure_limit) { stats.closure_cap_hit = true; break; }
    }
    return starts;
}

void write_address_taken_evidence(const std::filesystem::path& path,
                                  const std::vector<AddressTakenEvidence>& evidence) {
    if (path.empty()) return;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    if (!f) throw std::runtime_error("unable to create SH-4 evidence map: " + path.string());
    f << "target,references,clustered_references,instructions,calls,stage\n";
    for (const auto& e : evidence) {
        f << hex8(e.target) << ',' << e.references << ',' << e.clustered_references << ','
          << e.instructions << ',' << e.calls << ','
          << (e.stage == AddressTakenEvidenceStage::Proven ? "proven" : "candidate") << "\n";
    }
}

void write_map(const std::filesystem::path& path, const dcrecomp::ProgramAnalysis& program) {
    if (path.empty()) return;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    if (!f) throw std::runtime_error("unable to create map: " + path.string());
    f << "address,name,instructions,known,unknown,blocks,literals,calls,dynamic_branches\n";
    for (const auto& fn : program.functions) {
        f << hex8(fn.analysis.start_address) << ',' << fn.analysis.name << ','
          << fn.analysis.instructions.size() << ',' << fn.analysis.known << ',' << fn.analysis.unknown << ','
          << fn.cfg.blocks.size() << ',' << fn.analysis.literals.size() << ',' << fn.analysis.calls.size() << ','
          << fn.analysis.dynamic_branches.size() << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto o = parse(argc, argv);
        const auto bytes = read_all(o.path);
        if (!local_address(o.base, bytes.size(), o.entry)) throw std::runtime_error("entry lies outside raw image");

        ClosureStats closure;
        const auto starts = discover_function_closure(bytes, o, closure);
        const auto elf = make_synthetic_elf(bytes, o.base, o.entry, starts);

        dcrecomp::ProgramAnalysisOptions analysis_options;
        analysis_options.max_functions = o.max_functions;
        analysis_options.native_override_symbols.clear(); // raw commercial image has no trustworthy names yet
        // Every entry recovered by the symbol-free closure is a callable raw entry,
        // including alternate BSRF thunks that may only be selected at runtime.
        // Seed them all into the final program so codegen registers each target.
        for (const auto address : starts) {
            if (address != o.entry) analysis_options.seed_addresses.push_back(address);
        }
        const auto program = dcrecomp::analyze_reachable_program(elf, synthetic_name(o.entry), analysis_options);

        std::size_t total_instructions = 0u;
        std::size_t total_known = 0u;
        std::size_t total_unknown = 0u;
        std::size_t total_blocks = 0u;
        std::size_t total_ir = 0u;
        std::size_t total_raw = 0u;
        for (const auto& fn : program.functions) {
            total_instructions += fn.analysis.instructions.size();
            total_known += fn.analysis.known;
            total_unknown += fn.analysis.unknown;
            total_blocks += fn.cfg.blocks.size();
            for (const auto& block : fn.ir.blocks) {
                total_ir += block.instructions.size();
                for (const auto& op : block.instructions) if (op.op == dcrecomp::DCIROp::RawSH4) ++total_raw;
            }
        }

        write_map(o.map_path, program);
        write_address_taken_evidence(o.output / "sh4_address_taken_evidence.csv", closure.address_taken_evidence);

        std::optional<dcrecomp::CppProgramEmitResult> emitted;
        if (!o.no_emit) {
            emitted = dcrecomp::emit_cpp_program(elf, program, {o.output, true, true, true});
        }

        std::cout << "DreamcastRecomp Raw Commercial Recompiler 0.1.0\n"
                     "=============================================\n"
                  << "Input:                  " << o.path.string() << "\n"
                  << "Image bytes:            " << bytes.size() << "\n"
                  << "Load base:              " << hex8(o.base) << "\n"
                  << "Entry:                  " << hex8(o.entry) << "\n"
                  << "Manual seed entries:    " << o.seed_entries.size() << "\n"
                  << "Closure passes:         " << closure.passes << "\n"
                  << "Closure entry budget:   " << closure.closure_limit << " / " << o.max_functions << "\n"
                  << "Synthetic entries:      " << starts.size() << "\n"
                  << "Reachable functions:    " << program.functions.size() << "\n"
                  << "Call-graph edges:       " << program.edges.size() << "\n"
                  << "External/unresolved:    " << program.external_calls.size() << "\n"
                  << "Reachable instructions: " << total_instructions << "\n"
                  << "Known SH-4:             " << total_known << "\n"
                  << "Unknown SH-4:           " << total_unknown << "\n"
                  << "CFG blocks:             " << total_blocks << "\n"
                  << "DCIR ops:               " << total_ir << "\n"
                  << "RAW_SH4:                " << total_raw << "\n"
                  << "Closure resolved calls: " << closure.resolved_calls << "\n"
                  << "Closure unresolved:     " << closure.unresolved_calls << "\n"
                  << "Rejected nonlocal:      " << closure.rejected_targets << "\n"
                  << "Inline BSRF thunks:      " << closure.inline_bsrf_thunks << "\n"
                  << "Literal callback targets:" << std::setw(6) << closure.literal_callback_targets << "\n"
                  << "Stored callback targets:"  << std::setw(6) << closure.stored_callback_targets << "\n"
                  << "Direct literal targets:" << std::setw(8) << closure.direct_literal_targets << "\n"
                  << "CFG literal call targets:" << std::setw(5) << closure.cfg_literal_call_targets << "\n"
                  << "Returned literal targets:" << std::setw(6) << closure.returned_literal_targets << "\n"
                  << "Callback-object targets:" << std::setw(5) << closure.callback_object_targets << "\n"
                  << "Anchored dense callbacks:" << std::setw(5) << closure.anchored_dense_callback_targets << "\n"
                  << "Long dense callbacks:    " << std::setw(5) << closure.long_dense_callback_targets << "\n"
                  << "Clustered callbacks:     " << std::setw(5) << closure.clustered_callback_targets << "\n"
                  << "Strided callback targets:" << std::setw(5) << closure.strided_callback_targets << "\n"
                  << "Relocatable templates:" << std::setw(8) << closure.relocatable_code_templates << "\n"
                  << "Argument callbacks:" << std::setw(11) << closure.argument_callback_targets << "\n"
                  << "Promoted CFG fragments:" << std::setw(7) << closure.promoted_cfg_fragments << "\n"
                  << "VBR vector callbacks:" << std::setw(9) << closure.vbr_vector_callbacks << "\n"
                  << "Sparse callback targets:" << std::setw(6) << closure.sparse_callback_targets << "\n"
                  << "Dense dispatch targets:" << std::setw(8) << closure.dense_dispatch_targets << "\n"
                  << "Indexed tail dispatchers:" << std::setw(5) << closure.indexed_tail_dispatchers << "\n"
                  << "Indexed tail targets:" << std::setw(9) << closure.indexed_tail_targets << "\n"
                  << "Scaled-index targets:" << std::setw(10) << closure.scaled_index_dispatch_targets << "\n"
                  << "Branch-selected calls:" << std::setw(9) << closure.branch_selected_call_targets << "\n"
                  << "Packed BRA selectors: " << std::setw(9) << closure.packed_bra_selector_targets << "\n"
                  << "Rejected dirty fragments:" << std::setw(6) << closure.rejected_dirty_fragments << "\n"
                  << "Rejected dirty sources:  " << std::setw(6) << closure.rejected_dirty_sources << "\n"
                  << "Rejected dirty candidates:" << std::setw(5) << closure.rejected_dirty_candidates << "\n"
                  << "Address-taken candidates:" << std::setw(6) << closure.global_address_candidates << "\n"
                  << "Address-taken proven:   " << std::setw(8) << closure.global_address_targets << "\n";

        if (closure.closure_cap_hit)
            std::cout << "[WARN] Raw seed closure reached its reserved entry budget; final ProgramAnalysis still has protected headroom.\n";
        if (!o.map_path.empty()) std::cout << "Function map:            " << o.map_path.string() << "\n";
        std::cout << "SH-4 evidence map:       " << (o.output / "sh4_address_taken_evidence.csv").string() << "\n";
        if (emitted) {
            std::cout << "Generated output:        " << o.output.string() << "\n"
                      << "Embedded raw image:      " << emitted->embedded_bytes << " bytes\n"
                      << "Registered functions:    " << emitted->functions.size() << "\n";
        }

        if (total_raw == 0u && total_unknown == 0u) {
            std::cout << "\n[OK] Reachable commercial closure emitted without RAW_SH4.\n";
        } else {
            std::cout << "\n[PARTIAL] Commercial C++ was emitted, but unsupported/ambiguous reachable operations remain.\n";
        }
        if (!program.external_calls.empty()) {
            std::cout << "[NEXT] Runtime execution will identify the first unresolved dynamic/hardware dependency.\n";
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[dc_raw_recomp ERROR] " << e.what() << "\n";
        return 2;
    }
}
