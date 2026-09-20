#include "dcrecomp/function_analysis.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dcrecomp {
namespace {

constexpr std::uint32_t SHF_EXECINSTR = 0x4u;

bool is_executable_symbol(const Elf32Image& elf, const ElfSymbol& symbol) {
    if (symbol.value == 0 || symbol.section_index >= elf.sections.size()) return false;
    const auto& section = elf.sections[symbol.section_index];
    return (section.flags & SHF_EXECINSTR) != 0u &&
           symbol.value >= section.address && symbol.value < section.address + section.size;
}

bool is_callable_code_symbol(const Elf32Image& elf, const ElfSymbol& symbol) {
    if (symbol.is_function()) return is_executable_symbol(elf, symbol);
    // Hand-written SH-4 assembly in KallistiOS/newlib often exports entry points
    // as GLOBAL/WEAK STT_NOTYPE while keeping local labels as STT_NOTYPE too.
    // Accept only non-local NOTYPE symbols in executable sections so labels such
    // as L_loop do not become fake function boundaries.
    return symbol.type() == 0u && symbol.binding() != 0u && is_executable_symbol(elf, symbol);
}

struct FunctionRange {
    const ElfSymbol* symbol{};
    std::size_t section_index{};
    std::uint32_t start{};
    std::uint32_t end{};
};

std::optional<std::size_t> file_offset_for_vaddr(const Elf32Image& elf,
                                                 std::uint32_t address,
                                                 std::size_t size) {
    const auto try_section = [&](const ElfSection& section, std::uint32_t candidate)
        -> std::optional<std::size_t> {
        const std::uint64_t begin = section.address;
        const std::uint64_t end = begin + section.size;
        const std::uint64_t req_end = static_cast<std::uint64_t>(candidate) + size;
        if (candidate < begin || req_end > end) return std::nullopt;
        const auto delta = static_cast<std::size_t>(candidate - section.address);
        const auto file_off = static_cast<std::size_t>(section.offset) + delta;
        if (file_off <= elf.bytes.size() && size <= elf.bytes.size() - file_off)
            return file_off;
        return std::nullopt;
    };

    for (const auto& section : elf.sections) {
        if (const auto exact = try_section(section, address)) return exact;

        // Dreamcast SH-4 P1 (cached, 0x8C...) and P2 (uncached, 0xAC...) are
        // aliases of the same physical RAM. Symbol-free retail bootstrap code
        // frequently keeps *pointer cells themselves* in P2, then dereferences
        // them before JSR/JMP. The raw image is normally mapped as P1, so an
        // exact virtual-address lookup misses otherwise file-backed bytes.
        // Translate only P1/P2 aliases and only when the section lives in the
        // corresponding SH-4 area; ordinary ELF virtual addresses are unchanged.
        const auto area = address & 0xE0000000u;
        const auto section_area = section.address & 0xE0000000u;
        if ((area == 0x80000000u || area == 0xA0000000u) &&
            (section_area == 0x80000000u || section_area == 0xA0000000u)) {
            const auto physical = address & 0x1FFFFFFFu;
            const auto aliased = (section.address & 0xE0000000u) | physical;
            if (const auto mapped = try_section(section, aliased)) return mapped;
        }
    }
    return std::nullopt;
}

std::optional<FunctionRange> function_range(const Elf32Image& elf, std::string_view name) {
    const ElfSymbol* selected = nullptr;
    for (const auto& symbol : elf.symbols) {
        if (symbol.name != name || !is_callable_code_symbol(elf, symbol)) continue;
        if (!selected || (symbol.is_function() && !selected->is_function())) selected = &symbol;
    }
    if (!selected) {
        return std::nullopt;
    }

    std::size_t section_index = static_cast<std::size_t>(-1);
    if (selected->section_index < elf.sections.size()) {
        const auto& s = elf.sections[selected->section_index];
        const std::uint64_t begin = s.address;
        const std::uint64_t end = begin + s.size;
        if (selected->value >= begin && selected->value < end) {
            section_index = selected->section_index;
        }
    }

    if (section_index == static_cast<std::size_t>(-1)) {
        for (std::size_t i = 0; i < elf.sections.size(); ++i) {
            const auto& s = elf.sections[i];
            const std::uint64_t begin = s.address;
            const std::uint64_t end = begin + s.size;
            if (selected->value >= begin && selected->value < end) {
                section_index = i;
                break;
            }
        }
    }

    if (section_index == static_cast<std::size_t>(-1)) {
        return std::nullopt;
    }

    const auto& section = elf.sections[section_index];
    const std::uint32_t section_end = section.address + section.size;
    std::uint32_t end = section_end;

    if (selected->size != 0) {
        const std::uint64_t requested_end = static_cast<std::uint64_t>(selected->value) + selected->size;
        end = static_cast<std::uint32_t>(std::min<std::uint64_t>(requested_end, section_end));
    } else {
        for (const auto& candidate : elf.symbols) {
            if (!is_callable_code_symbol(elf, candidate) || candidate.value <= selected->value) continue;
            if (candidate.value >= section.address && candidate.value < end) end = candidate.value;
        }
    }

    return FunctionRange{selected, section_index, selected->value & ~1u, end & ~1u};
}

std::optional<FunctionRange> function_range_at(const Elf32Image& elf, std::uint32_t address) {
    const ElfSymbol* selected = nullptr;
    for (const auto& symbol : elf.symbols) {
        if ((symbol.value & ~1u) != (address & ~1u) || !is_callable_code_symbol(elf, symbol)) continue;
        if (!selected || (symbol.is_function() && !selected->is_function())) selected = &symbol;
    }
    if (!selected) return std::nullopt;

    std::size_t section_index = static_cast<std::size_t>(-1);
    if (selected->section_index < elf.sections.size()) {
        const auto& section = elf.sections[selected->section_index];
        if (selected->value >= section.address && selected->value < section.address + section.size)
            section_index = selected->section_index;
    }
    if (section_index == static_cast<std::size_t>(-1)) return std::nullopt;

    const auto& section = elf.sections[section_index];
    const std::uint32_t section_end = section.address + section.size;
    std::uint32_t end = section_end;
    if (selected->size != 0u) {
        const std::uint64_t requested_end = static_cast<std::uint64_t>(selected->value) + selected->size;
        end = static_cast<std::uint32_t>(std::min<std::uint64_t>(requested_end, section_end));
    } else {
        for (const auto& candidate : elf.symbols) {
            if (!is_callable_code_symbol(elf, candidate) || candidate.value <= selected->value) continue;
            if (candidate.value >= section.address && candidate.value < end) end = candidate.value;
        }
    }
    return FunctionRange{selected, section_index, selected->value & ~1u, end & ~1u};
}

bool in_range(const FunctionRange& range, std::uint32_t address, std::size_t size = 2) {
    const std::uint64_t end = static_cast<std::uint64_t>(address) + size;
    return address >= range.start && end <= range.end;
}

struct LiteralSpan {
    std::uint32_t address{};
    std::uint8_t size{};
};

bool address_in_literal(const std::vector<LiteralSpan>& spans, std::uint32_t address) {
    for (const auto& span : spans) {
        if (address >= span.address && address < span.address + span.size) {
            return true;
        }
    }
    return false;
}

std::string symbol_name(const Elf32Image& elf, std::uint32_t value) {
    if (const auto* symbol = find_symbol_at(elf, value)) return symbol->name;
    // 0.0.170: report the P1/P2 twin symbol as well. Katana deliberately calls
    // startup/cache helpers through P2 while ELF/raw symbols normally live in P1.
    const std::uint32_t physical = value & 0x1FFFFFFFu;
    for (const auto& symbol : elf.symbols) {
        if ((symbol.value & 0x1FFFFFFFu) == physical && is_callable_code_symbol(elf, symbol) &&
            !symbol.name.empty() && symbol.name != "<invalid>") return symbol.name;
    }
    return {};
}

std::string section_name(const Elf32Image& elf, std::uint32_t value) {
    if (const auto* section = find_section_at(elf, value)) return section->name;
    const std::uint32_t physical = value & 0x1FFFFFFFu;
    for (const auto& section : elf.sections) {
        if (section.size == 0u) continue;
        const std::uint32_t begin_phys = section.address & 0x1FFFFFFFu;
        if (physical >= begin_phys && static_cast<std::uint64_t>(physical - begin_phys) < section.size)
            return section.name;
    }
    return {};
}

bool is_known_noreturn_symbol(std::string_view name) {
    // Conservative list of well-known C/newlib termination routines.
    // These functions never return to the instruction following the call, so
    // treating that address as a normal successor can make embedded literal
    // pools / jump tables after the call look like executable SH-4.
    static constexpr std::string_view kNames[] = {
        "abort", "_abort",
        "exit", "_exit", "_Exit", "__exit",
        "__assert", "___assert", "__assert_func", "___assert_func",
        "__stack_chk_fail", "___stack_chk_fail"
    };
    return std::find(std::begin(kNames), std::end(kNames), name) != std::end(kNames);
}

bool is_flow_barrier(const sh4::Instruction& i) {
    using sh4::Opcode;
    switch (i.opcode) {
        case Opcode::Bra:
        case Opcode::Bsr:
        case Opcode::Bt:
        case Opcode::Bf:
        case Opcode::BtS:
        case Opcode::BfS:
        case Opcode::Braf:
        case Opcode::Bsrf:
        case Opcode::Jmp:
        case Opcode::Jsr:
        case Opcode::Rts:
        case Opcode::Rte:
            return true;
        default:
            return false;
    }
}

bool writes_register(const sh4::Instruction& i, std::uint8_t reg) {
    using sh4::Opcode;
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

std::optional<std::uint32_t> literal_value_for_instruction(
    const std::vector<LiteralReference>& literals,
    std::uint32_t instruction_address) {
    for (const auto& literal : literals) {
        if (literal.instruction_address == instruction_address) {
            return literal.value;
        }
    }
    return std::nullopt;
}

std::optional<std::uint32_t> resolve_register_before(
    const std::vector<sh4::Instruction>& instructions,
    const std::vector<LiteralReference>& literals,
    std::size_t before_index,
    std::uint8_t reg,
    int depth = 0) {
    using sh4::Opcode;
    if (depth > 8) {
        return std::nullopt;
    }

    for (std::size_t pos = before_index; pos > 0; --pos) {
        const std::size_t i = pos - 1;
        const auto& insn = instructions[i];

        // Do not infer a constant through an earlier control-flow boundary. This
        // deliberately keeps the analysis conservative instead of inventing values at joins.
        if (is_flow_barrier(insn)) {
            return std::nullopt;
        }
        if (!writes_register(insn, reg)) {
            continue;
        }

        switch (insn.opcode) {
            case Opcode::MovImm:
                return static_cast<std::uint32_t>(insn.immediate);
            case Opcode::MovLPcRel:
            case Opcode::MovWPcRel:
                return literal_value_for_instruction(literals, insn.address);
            case Opcode::Mova:
                if (reg == 0) return insn.effective_address;
                return std::nullopt;
            case Opcode::MovReg:
                return resolve_register_before(instructions, literals, i, insn.rm, depth + 1);
            case Opcode::AddImm: {
                const auto base = resolve_register_before(instructions, literals, i, reg, depth + 1);
                if (!base) return std::nullopt;
                return static_cast<std::uint32_t>(*base + insn.immediate);
            }
            case Opcode::AddReg: {
                const auto a = resolve_register_before(instructions, literals, i, reg, depth + 1);
                const auto b = resolve_register_before(instructions, literals, i, insn.rm, depth + 1);
                if (!a || !b) return std::nullopt;
                return *a + *b;
            }
            case Opcode::OrReg: {
                const auto a = resolve_register_before(instructions, literals, i, reg, depth + 1);
                const auto b = resolve_register_before(instructions, literals, i, insn.rm, depth + 1);
                if (!a || !b) return std::nullopt;
                return *a | *b;
            }
            case Opcode::OrImm: {
                if (reg != 0u) return std::nullopt;
                const auto base = resolve_register_before(instructions, literals, i, reg, depth + 1);
                if (!base) return std::nullopt;
                return *base | static_cast<std::uint32_t>(insn.immediate);
            }
            default:
                return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::uint32_t> resolve_register_linear_before(
    const std::vector<sh4::Instruction>& instructions,
    const std::vector<LiteralReference>& literals,
    std::size_t before_index,
    std::uint8_t reg,
    int depth = 0) {
    using sh4::Opcode;
    if (depth > 8) return std::nullopt;
    for (std::size_t pos = before_index; pos > 0; --pos) {
        const std::size_t i = pos - 1;
        const auto& insn = instructions[i];
        if (!writes_register(insn, reg)) continue;
        switch (insn.opcode) {
            case Opcode::MovImm:
                return static_cast<std::uint32_t>(insn.immediate);
            case Opcode::MovLPcRel:
            case Opcode::MovWPcRel:
                return literal_value_for_instruction(literals, insn.address);
            case Opcode::Mova:
                if (reg == 0) return insn.effective_address;
                return std::nullopt;
            case Opcode::MovReg:
                return resolve_register_linear_before(instructions, literals, i, insn.rm, depth + 1);
            case Opcode::AddImm: {
                const auto base = resolve_register_linear_before(instructions, literals, i, reg, depth + 1);
                if (!base) return std::nullopt;
                return static_cast<std::uint32_t>(*base + insn.immediate);
            }
            case Opcode::AddReg: {
                const auto a = resolve_register_linear_before(instructions, literals, i, reg, depth + 1);
                const auto b = resolve_register_linear_before(instructions, literals, i, insn.rm, depth + 1);
                if (!a || !b) return std::nullopt;
                return *a + *b;
            }
            case Opcode::OrReg: {
                const auto a = resolve_register_linear_before(instructions, literals, i, reg, depth + 1);
                const auto b = resolve_register_linear_before(instructions, literals, i, insn.rm, depth + 1);
                if (!a || !b) return std::nullopt;
                return *a | *b;
            }
            case Opcode::OrImm: {
                if (reg != 0u) return std::nullopt;
                const auto base = resolve_register_linear_before(instructions, literals, i, reg, depth + 1);
                if (!base) return std::nullopt;
                return *base | static_cast<std::uint32_t>(insn.immediate);
            }
            default:
                return std::nullopt;
        }
    }
    return std::nullopt;
}


// 0.0.170: commercial Katana code frequently resolves an indirect call through
// one extra pointer load rather than keeping the final function address in the
// PC-relative literal itself.  Examples include SDK ops tables, bootstrap helper
// cells and small vtables:
//
//   mov.l pointer_cell,r1
//   mov.l @r1,r2
//   mov    r2,r3
//   jsr    @r3
//
// The older constant resolver intentionally stopped at memory loads, which left
// these real runtime targets outside the raw-image closure.  Extend the same
// bounded backwards proof with read-only loads from bytes that are present in the
// input ELF/raw image.  No runtime-written memory is guessed here: if the address
// cannot be resolved to file-backed bytes the proof simply fails.
std::optional<std::uint32_t> resolve_register_filebacked_before(
    const Elf32Image& elf,
    const std::vector<sh4::Instruction>& instructions,
    const std::vector<LiteralReference>& literals,
    std::size_t before_index,
    std::uint8_t reg,
    bool stop_at_flow,
    int depth = 0) {
    using sh4::Opcode;
    if (depth > 12) return std::nullopt;

    auto resolve = [&](std::size_t i, std::uint8_t r) {
        return resolve_register_filebacked_before(elf, instructions, literals, i, r, stop_at_flow, depth + 1);
    };
    auto load32 = [&](std::uint32_t address) -> std::optional<std::uint32_t> {
        const auto off = file_offset_for_vaddr(elf, address, 4u);
        if (!off) return std::nullopt;
        return static_cast<std::uint32_t>(elf.bytes[*off]) |
               (static_cast<std::uint32_t>(elf.bytes[*off + 1u]) << 8u) |
               (static_cast<std::uint32_t>(elf.bytes[*off + 2u]) << 16u) |
               (static_cast<std::uint32_t>(elf.bytes[*off + 3u]) << 24u);
    };

    for (std::size_t pos = before_index; pos > 0; --pos) {
        const std::size_t i = pos - 1;
        const auto& insn = instructions[i];
        if (stop_at_flow && is_flow_barrier(insn)) return std::nullopt;
        if (!writes_register(insn, reg)) continue;

        switch (insn.opcode) {
            case Opcode::MovImm:
                return static_cast<std::uint32_t>(insn.immediate);
            case Opcode::MovLPcRel:
            case Opcode::MovWPcRel:
                return literal_value_for_instruction(literals, insn.address);
            case Opcode::Mova:
                return reg == 0u ? std::optional<std::uint32_t>(insn.effective_address) : std::nullopt;
            case Opcode::MovReg:
                return resolve(i, insn.rm);

            case Opcode::MovLLoad:
            case Opcode::MovLPostinc: {
                const auto address = resolve(i, insn.rm);
                return address ? load32(*address) : std::nullopt;
            }
            case Opcode::MovLDispLoad: {
                const auto base = resolve(i, insn.rm);
                if (!base) return std::nullopt;
                return load32(static_cast<std::uint32_t>(*base + static_cast<std::uint32_t>(insn.immediate * 4)));
            }
            case Opcode::MovLIndexedLoad: {
                const auto base = resolve(i, insn.rm);
                const auto index = resolve(i, 0u);
                if (!base || !index) return std::nullopt;
                return load32(*base + *index);
            }

            case Opcode::AddImm: {
                const auto a = resolve(i, reg);
                return a ? std::optional<std::uint32_t>(static_cast<std::uint32_t>(*a + insn.immediate)) : std::nullopt;
            }
            case Opcode::AddReg: {
                const auto a = resolve(i, reg);
                const auto b = resolve(i, insn.rm);
                return a && b ? std::optional<std::uint32_t>(*a + *b) : std::nullopt;
            }
            case Opcode::SubReg: {
                const auto a = resolve(i, reg);
                const auto b = resolve(i, insn.rm);
                return a && b ? std::optional<std::uint32_t>(*a - *b) : std::nullopt;
            }
            case Opcode::Neg: {
                const auto a = resolve(i, insn.rm);
                return a ? std::optional<std::uint32_t>(0u - *a) : std::nullopt;
            }
            case Opcode::AndReg:
            case Opcode::XorReg:
            case Opcode::OrReg: {
                const auto a = resolve(i, reg);
                const auto b = resolve(i, insn.rm);
                if (!a || !b) return std::nullopt;
                if (insn.opcode == Opcode::AndReg) return *a & *b;
                if (insn.opcode == Opcode::XorReg) return *a ^ *b;
                return *a | *b;
            }
            case Opcode::AndImm:
            case Opcode::XorImm:
            case Opcode::OrImm: {
                if (reg != 0u) return std::nullopt;
                const auto a = resolve(i, reg);
                if (!a) return std::nullopt;
                const auto imm = static_cast<std::uint32_t>(insn.immediate) & 0xFFu;
                if (insn.opcode == Opcode::AndImm) return *a & imm;
                if (insn.opcode == Opcode::XorImm) return *a ^ imm;
                return *a | imm;
            }
            case Opcode::Not: {
                const auto a = resolve(i, insn.rm);
                return a ? std::optional<std::uint32_t>(~*a) : std::nullopt;
            }

            case Opcode::Shll:
            case Opcode::Shal:
            case Opcode::Shll2:
            case Opcode::Shll8:
            case Opcode::Shll16:
            case Opcode::Shlr:
            case Opcode::Shlr2:
            case Opcode::Shlr8:
            case Opcode::Shlr16:
            case Opcode::Shar: {
                const auto a = resolve(i, reg);
                if (!a) return std::nullopt;
                unsigned amount = 1u;
                if (insn.opcode == Opcode::Shll2 || insn.opcode == Opcode::Shlr2) amount = 2u;
                else if (insn.opcode == Opcode::Shll8 || insn.opcode == Opcode::Shlr8) amount = 8u;
                else if (insn.opcode == Opcode::Shll16 || insn.opcode == Opcode::Shlr16) amount = 16u;
                if (insn.opcode == Opcode::Shll || insn.opcode == Opcode::Shal ||
                    insn.opcode == Opcode::Shll2 || insn.opcode == Opcode::Shll8 || insn.opcode == Opcode::Shll16)
                    return static_cast<std::uint32_t>(*a << amount);
                if (insn.opcode == Opcode::Shar)
                    return static_cast<std::uint32_t>(static_cast<std::int32_t>(*a) >> amount);
                return *a >> amount;
            }
            case Opcode::ExtuB: {
                const auto a = resolve(i, insn.rm);
                return a ? std::optional<std::uint32_t>(*a & 0xFFu) : std::nullopt;
            }
            case Opcode::ExtuW: {
                const auto a = resolve(i, insn.rm);
                return a ? std::optional<std::uint32_t>(*a & 0xFFFFu) : std::nullopt;
            }
            case Opcode::ExtsB: {
                const auto a = resolve(i, insn.rm);
                if (!a) return std::nullopt;
                return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(*a & 0xFFu)));
            }
            case Opcode::ExtsW: {
                const auto a = resolve(i, insn.rm);
                if (!a) return std::nullopt;
                return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(*a & 0xFFFFu)));
            }
            case Opcode::SwapW: {
                const auto a = resolve(i, insn.rm);
                if (!a) return std::nullopt;
                return static_cast<std::uint32_t>((*a << 16u) | (*a >> 16u));
            }
            case Opcode::SwapB: {
                const auto a = resolve(i, insn.rm);
                if (!a) return std::nullopt;
                return static_cast<std::uint32_t>((*a & 0xFFFF0000u) | ((*a & 0xFFu) << 8u) | ((*a >> 8u) & 0xFFu));
            }
            default:
                return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace

const ElfSymbol* find_symbol_at(const Elf32Image& elf, std::uint32_t address) {
    const ElfSymbol* fallback = nullptr;
    for (const auto& symbol : elf.symbols) {
        if (symbol.value != address || symbol.name.empty() || symbol.name == "<invalid>") {
            continue;
        }
        if (is_callable_code_symbol(elf, symbol)) {
            return &symbol;
        }
        if (!fallback) {
            fallback = &symbol;
        }
    }
    return fallback;
}

const ElfSection* find_section_at(const Elf32Image& elf, std::uint32_t address) {
    for (const auto& section : elf.sections) {
        if (section.size == 0) continue;
        const std::uint64_t begin = section.address;
        const std::uint64_t end = begin + section.size;
        if (address >= begin && address < end) {
            return &section;
        }
    }
    return nullptr;
}

std::optional<std::uint16_t> read_u16_vaddr(const Elf32Image& elf, std::uint32_t address) {
    const auto off = file_offset_for_vaddr(elf, address, 2);
    if (!off) return std::nullopt;
    return static_cast<std::uint16_t>(elf.bytes[*off]) |
           (static_cast<std::uint16_t>(elf.bytes[*off + 1]) << 8);
}

std::optional<std::uint32_t> read_u32_vaddr(const Elf32Image& elf, std::uint32_t address) {
    const auto off = file_offset_for_vaddr(elf, address, 4);
    if (!off) return std::nullopt;
    return static_cast<std::uint32_t>(elf.bytes[*off]) |
           (static_cast<std::uint32_t>(elf.bytes[*off + 1]) << 8) |
           (static_cast<std::uint32_t>(elf.bytes[*off + 2]) << 16) |
           (static_cast<std::uint32_t>(elf.bytes[*off + 3]) << 24);
}

bool has_clean_sh4_prefix(const Elf32Image& elf, std::uint32_t address, std::uint32_t halfwords = 16u) {
    if ((address & 1u) != 0u || halfwords == 0u) return false;
    const auto* section = find_section_at(elf, address);
    if (!section || (section->flags & SHF_EXECINSTR) == 0u) return false;
    const std::uint64_t end = static_cast<std::uint64_t>(address) + halfwords * 2u;
    if (end > static_cast<std::uint64_t>(section->address) + section->size) return false;
    for (std::uint32_t i = 0u; i < halfwords; ++i) {
        const auto pc = address + i * 2u;
        const auto raw = read_u16_vaddr(elf, pc);
        if (!raw || !sh4::is_known(sh4::decode(*raw, pc))) return false;
    }
    return true;
}

bool has_compact_return_thunk(const Elf32Image& elf, std::uint32_t address) {
    if ((address & 1u) != 0u) return false;
    const auto raw = read_u16_vaddr(elf, address);
    const auto slot_raw = read_u16_vaddr(elf, address + 2u);
    if (!raw || !slot_raw) return false;
    const auto first = sh4::decode(*raw, address);
    const auto slot = sh4::decode(*slot_raw, address + 2u);
    return first.opcode == sh4::Opcode::Rts && sh4::is_known(slot);
}

bool has_compact_literal_tail_thunk(const Elf32Image& elf, std::uint32_t address) {
    if ((address & 1u) != 0u) return false;
    const auto raw0 = read_u16_vaddr(elf, address);
    const auto raw1 = read_u16_vaddr(elf, address + 2u);
    const auto raw2 = read_u16_vaddr(elf, address + 4u);
    if (!raw0 || !raw1 || !raw2) return false;
    const auto load = sh4::decode(*raw0, address);
    const auto jump = sh4::decode(*raw1, address + 2u);
    const auto slot = sh4::decode(*raw2, address + 4u);
    // Common SDK veneer: MOV.L @(disp,PC),Rn; JMP @Rn; <delay slot>.
    // The literal may be followed immediately by data, so a 32-byte clean
    // prefix is neither expected nor required.
    return load.opcode == sh4::Opcode::MovLPcRel &&
           jump.opcode == sh4::Opcode::Jmp && jump.rm == load.rn &&
           sh4::is_known(slot);
}

bool has_conditional_literal_tail_thunk(const Elf32Image& elf, std::uint32_t address) {
    if ((address & 1u) != 0u) return false;
    const auto* section = find_section_at(elf, address);
    if (!section || (section->flags & SHF_EXECINSTR) == 0u) return false;

    const auto literal_jump_at = [&](std::uint32_t pc) {
        if (pc < section->address || pc + 6u > section->address + section->size) return false;
        const auto raw0 = read_u16_vaddr(elf, pc);
        const auto raw1 = read_u16_vaddr(elf, pc + 2u);
        const auto raw2 = read_u16_vaddr(elf, pc + 4u);
        if (!raw0 || !raw1 || !raw2) return false;
        const auto load = sh4::decode(*raw0, pc);
        const auto jump = sh4::decode(*raw1, pc + 2u);
        const auto slot = sh4::decode(*raw2, pc + 4u);
        return load.opcode == sh4::Opcode::MovLPcRel &&
               jump.opcode == sh4::Opcode::Jmp && jump.rm == load.rn &&
               sh4::is_known(slot);
    };

    // Compact retail SDK selector veneer:
    //   ...
    //   bt/bf alternate
    //   mov.l target_a, Rn; jmp @Rn; <slot>
    // alternate:
    //   mov.l target_b, Rm; jmp @Rm; <slot>
    // The literal pool can begin immediately after both tails, so a long clean
    // prefix is not available. Requiring *both* branch arms to be complete
    // literal JMP veneers keeps this predicate much narrower than merely
    // accepting a short decodable prefix.
    constexpr std::uint32_t kMaxPreludeHalfwords = 6u;
    for (std::uint32_t i = 0u; i < kMaxPreludeHalfwords; ++i) {
        const std::uint32_t pc = address + i * 2u;
        if (pc + 2u > section->address + section->size) return false;
        const auto raw = read_u16_vaddr(elf, pc);
        if (!raw) return false;
        const auto insn = sh4::decode(*raw, pc);
        if (!sh4::is_known(insn)) return false;

        std::uint32_t fallthrough = 0u;
        switch (insn.opcode) {
            case sh4::Opcode::Bt:
            case sh4::Opcode::Bf:
                fallthrough = pc + 2u;
                break;
            case sh4::Opcode::BtS:
            case sh4::Opcode::BfS:
                fallthrough = pc + 4u;
                break;
            default:
                continue;
        }
        if (literal_jump_at(fallthrough) && literal_jump_at(insn.target)) return true;
    }
    return false;
}

bool has_indexed_literal_tail_dispatch_thunk(const Elf32Image& elf, std::uint32_t address) {
    if ((address & 1u) != 0u) return false;
    std::array<sh4::Instruction, 7> insn{};
    for (std::uint32_t i = 0u; i < insn.size(); ++i) {
        const auto pc = address + i * 2u;
        const auto raw = read_u16_vaddr(elf, pc);
        if (!raw) return false;
        insn[i] = sh4::decode(*raw, pc);
        if (!sh4::is_known(insn[i])) return false;
    }

    // Retail SDK method veneer used by ChuChu Rocket and similar Katana code:
    //   mov.l state, rBase
    //   mov.l table, r0
    //   mov.l @rBase, rIndex
    //   shll2 rIndex
    //   mov.l @(r0,rIndex),rTarget
    //   jmp @rTarget
    //   <delay slot>
    // Its literal pool begins immediately afterwards, so a generic 32-byte
    // clean-prefix test intentionally cannot recognize it.
    return insn[0].opcode == sh4::Opcode::MovLPcRel &&
           insn[1].opcode == sh4::Opcode::MovLPcRel && insn[1].rn == 0u &&
           insn[2].opcode == sh4::Opcode::MovLLoad &&
           insn[3].opcode == sh4::Opcode::Shll2 && insn[3].rn == insn[2].rn &&
           insn[4].opcode == sh4::Opcode::MovLIndexedLoad &&
           insn[4].rn == insn[5].rm &&
           insn[5].opcode == sh4::Opcode::Jmp;
}

// Dense SDK method tables can contain short wrappers that call one helper and
// then tail-call another. Their literal pool may begin well before a 32-byte
// linear prefix is available, so has_clean_sh4_prefix() deliberately rejects
// them even though the entry itself has a complete callable control-flow shape.
// Keep this predicate narrow: it is only used after a two-level row/method table
// has already established strong structural evidence. A JMP counts as a valid
// endpoint only after a real call has been seen, which avoids promoting ordinary
// intra-function branch fragments as standalone methods.
bool has_short_call_then_tail_entry(const Elf32Image& elf, std::uint32_t address) {
    if ((address & 1u) != 0u) return false;
    const auto* section = find_section_at(elf, address);
    if (!section || (section->flags & SHF_EXECINSTR) == 0u) return false;

    bool saw_call = false;
    constexpr std::uint32_t kMaxHalfwords = 24u;
    for (std::uint32_t i = 0u; i < kMaxHalfwords; ++i) {
        const std::uint32_t pc = address + i * 2u;
        if (pc + 2u > section->address + section->size) return false;
        const auto raw = read_u16_vaddr(elf, pc);
        if (!raw) return false;
        const auto insn = sh4::decode(*raw, pc);
        if (!sh4::is_known(insn)) return false;

        if (insn.opcode == sh4::Opcode::Bsr || insn.opcode == sh4::Opcode::Bsrf ||
            insn.opcode == sh4::Opcode::Jsr) {
            saw_call = true;
        }

        if (insn.opcode == sh4::Opcode::Rts || insn.opcode == sh4::Opcode::Rte ||
            insn.opcode == sh4::Opcode::Jmp) {
            const auto slot_raw = read_u16_vaddr(elf, pc + 2u);
            if (!slot_raw || !sh4::is_known(sh4::decode(*slot_raw, pc + 2u))) return false;
            if (insn.opcode == sh4::Opcode::Jmp) return saw_call;
            return true;
        }

        // An unconditional BRA is very common inside normal functions. Do not
        // treat it as evidence that a table entry is a complete callable.
        if (insn.opcode == sh4::Opcode::Bra) return false;
    }
    return false;
}

static FunctionAnalysis analyze_function_with_range(const Elf32Image& elf, const FunctionRange& range, std::string_view function_name, std::uint32_t entry_address = 0u) {
    using sh4::Opcode;

    FunctionAnalysis result;
    if (entry_address == 0u) entry_address = range.start;
    result.name = std::string(function_name);
    result.section_index = range.section_index;
    result.start_address = entry_address;
    result.end_address = range.end;

    std::set<std::uint32_t> pending;
    std::map<std::uint32_t, sh4::Instruction> code;
    std::vector<LiteralSpan> literal_spans;
    std::set<std::pair<std::uint32_t, std::uint32_t>> literal_keys;

    auto add_literal = [&](const sh4::Instruction& insn) {
        if (insn.opcode != Opcode::MovLPcRel && insn.opcode != Opcode::MovWPcRel) {
            return;
        }

        LiteralReference literal;
        literal.instruction_address = insn.address;
        literal.storage_address = insn.effective_address;
        literal.destination_register = insn.rn;

        if (insn.opcode == Opcode::MovLPcRel) {
            literal.kind = LiteralKind::Long32;
            const auto value = read_u32_vaddr(elf, insn.effective_address);
            if (!value) return;
            literal.value = *value;
            literal.inside_function = in_range(range, insn.effective_address, 4);
            if (literal.inside_function) {
                literal_spans.push_back({insn.effective_address, 4});
            }
        } else {
            literal.kind = LiteralKind::Word16;
            const auto value = read_u16_vaddr(elf, insn.effective_address);
            if (!value) return;
            literal.value = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(*value)));
            literal.inside_function = in_range(range, insn.effective_address, 2);
            if (literal.inside_function) {
                literal_spans.push_back({insn.effective_address, 2});
            }
        }

        literal.symbol = symbol_name(elf, literal.value);
        literal.section = section_name(elf, literal.value);

        const auto key = std::make_pair(literal.instruction_address, literal.storage_address);
        if (literal_keys.insert(key).second) {
            result.literals.push_back(std::move(literal));
        }
    };

    auto decode_at = [&](std::uint32_t address) -> std::optional<sh4::Instruction> {
        if (!in_range(range, address, 2) || address_in_literal(literal_spans, address)) {
            return std::nullopt;
        }
        const auto raw = read_u16_vaddr(elf, address);
        if (!raw) return std::nullopt;
        auto insn = sh4::decode(*raw, address);
        add_literal(insn);
        return insn;
    };

    // A SH-4 delay-slot instruction can also be a normal branch target.  In that
    // case it has two distinct execution contexts: as a slot it transfers with
    // the preceding branch, while as a direct entry it executes normally and
    // then falls through to address+2.  Keep track of slots inserted eagerly so
    // later control-flow discovery can expand their independent fallthrough.
    std::set<std::uint32_t> recorded_delay_slots;
    std::set<std::uint32_t> expanded_delay_slot_entries;

    auto record_delay_slot = [&](std::uint32_t address) {
        if (address_in_literal(literal_spans, address)) return;
        recorded_delay_slots.insert(address);
        if (code.contains(address)) return;
        if (const auto slot = decode_at(address)) {
            code.emplace(address, *slot);
        }
    };

    auto enqueue = [&](std::uint32_t address) {
        if (!in_range(range, address, 2) || address_in_literal(literal_spans, address)) return;
        if (code.contains(address)) {
            if (recorded_delay_slots.contains(address) && expanded_delay_slot_entries.insert(address).second) {
                const auto next = address + 2u;
                if (in_range(range, next, 2) && !address_in_literal(literal_spans, next) && !code.contains(next))
                    pending.insert(next);
            }
            return;
        }
        pending.insert(address);
    };

    auto discover_byte_jump_table = [&](const sh4::Instruction& branch) {
        // GCC's common SH-4 switch lowering is:
        //   cmp/hi #N,index ; bt default ; mova table,r0
        //   mov.b @(r0,index),index ; braf index ; <signed byte table>
        // Each table byte is an offset from BRAF's architectural PC+4. MOV.B is signed;
        // GCC sometimes follows it with EXTU.B to deliberately select unsigned offsets.
        // Discovering these targets is essential: returning to the host on an
        // in-function BRAF silently skips epilogues and corrupts callee-saved state.
        const sh4::Instruction* load = nullptr;
        const sh4::Instruction* mova = nullptr;
        std::uint32_t load_address = 0u;
        bool zero_extended_offsets = false;
        // GCC may insert EXTU.B or an unrelated literal load between the table
        // lookup and BRAF, so do not require the three instructions to be adjacent.
        // EXTU.B is semantically important: without it MOV.B produces signed byte
        // offsets, while an explicit EXTU.B makes the branch-table offsets unsigned.
        for (std::uint32_t back = 2u; back <= 10u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::ExtuB && it->second.rn == branch.rn && it->second.rm == branch.rn) {
                zero_extended_offsets = true;
                continue;
            }
            if (it->second.opcode == Opcode::MovBIndexedLoad && it->second.rn == branch.rn && it->second.rm == branch.rn) {
                load = &it->second;
                load_address = it->first;
                break;
            }
            if (writes_register(it->second, branch.rn)) break;
        }
        if (!load) return;
        for (std::uint32_t back = 2u; back <= 12u && load_address >= range.start + back; back += 2u) {
            const auto it = code.find(load_address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::Mova) { mova = &it->second; break; }
            if (writes_register(it->second, 0u)) break;
        }
        if (!mova) return;

        std::optional<std::uint32_t> count;
        // Find a nearby CMP/HI Rbound,Rindex and the MOV #N,Rbound feeding it.
        for (std::uint32_t back = 6u; back <= 512u && branch.address >= range.start + back; back += 2u) {
            const auto cmp_it = code.find(branch.address - back);
            if (cmp_it == code.end() || cmp_it->second.opcode != Opcode::CmpHi || cmp_it->second.rn != branch.rn) continue;
            const auto bound_reg = cmp_it->second.rm;
            for (std::uint32_t back2 = back + 2u; back2 <= back + 32u && branch.address >= range.start + back2; back2 += 2u) {
                const auto def_it = code.find(branch.address - back2);
                if (def_it == code.end()) continue;
                if (def_it->second.opcode == Opcode::MovImm && def_it->second.rn == bound_reg &&
                    def_it->second.immediate >= 0 && def_it->second.immediate < 64) {
                    count = static_cast<std::uint32_t>(def_it->second.immediate) + 1u;
                    break;
                }
                if (writes_register(def_it->second, bound_reg)) break;
            }
            if (count) break;
        }
        if (!count || *count == 0u) return;

        const std::uint32_t table = mova->effective_address;
        const auto table_off = file_offset_for_vaddr(elf, table, *count);
        if (!table_off) return;
        literal_spans.push_back({table, static_cast<std::uint8_t>(*count)});

        DynamicBranchReference ref;
        ref.instruction_address = branch.address;
        for (std::uint32_t n = 0; n < *count; ++n) {
            const std::uint32_t raw_delta = elf.bytes[*table_off + n];
            const std::int32_t delta = zero_extended_offsets
                ? static_cast<std::int32_t>(raw_delta)
                : static_cast<std::int32_t>(static_cast<std::int8_t>(raw_delta));
            const std::uint32_t target = static_cast<std::uint32_t>(branch.address + 4u + delta);
            if ((target & 1u) != 0u || !in_range(range, target, 2)) continue;
            if (std::find(ref.targets.begin(), ref.targets.end(), target) == ref.targets.end()) {
                ref.targets.push_back(target);
                enqueue(target);
            }
        }
        if (!ref.targets.empty()) result.dynamic_branches.push_back(std::move(ref));
    };

    auto discover_absolute_jump_table = [&](const sh4::Instruction& branch) {
        // Katana code uses dense tables of absolute 32-bit callable addresses for
        // both tail dispatch (JMP @Rn) and ordinary dispatch calls (JSR @Rn). A
        // common pattern is:
        //   mov.l @(table_literal,pc),Rn
        //   mov.l @(r0,Rn),Rn
        //   jsr/jmp @Rn
        // with R0 holding a byte offset (0,4,8,...). In-function JMP tables may
        // target basic blocks, while cross-function JSR/JMP tables point at real
        // callable entries. For the latter, strong dense-table evidence plus a
        // clean 32-byte SH-4 prefix is required; no early RTS/branch is required.
        if (branch.opcode != Opcode::Jmp && branch.opcode != Opcode::Jsr) return;

        const sh4::Instruction* indexed = nullptr;
        std::uint32_t indexed_address = 0u;
        for (std::uint32_t back = 2u; back <= 8u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovLIndexedLoad &&
                it->second.rn == branch.rm) {
                indexed = &it->second;
                indexed_address = it->first;
                break;
            }
            if (writes_register(it->second, branch.rm)) break;
        }
        if (!indexed) return;

        std::optional<std::uint32_t> table;
        // MOV.L @(R0,Rm),Rn is commutative at the address-adder: SDK code may
        // keep the table base either in Rm (with R0 as the byte offset) or in
        // the implicit R0 operand (with Rm holding a scaled index). Recognize
        // both forms. The latter is used by retail dispatchers that do
        //   mov.l table, r0; shll2 rN; mov.l @(r0,rN),rN; jsr @rN.
        const std::array<std::uint8_t, 2> candidate_base_regs{indexed->rm, 0u};
        // Some retail Katana dispatchers materialize the table base slightly
        // earlier than the small indexed-load window (18 bytes in one observed
        // SDK sequence). Keep the search bounded and stop at register clobbers.
        for (const auto base_reg : candidate_base_regs) {
            if (table) break;
            for (std::uint32_t back = 2u; back <= 32u && indexed_address >= range.start + back; back += 2u) {
                const auto it = code.find(indexed_address - back);
                if (it == code.end()) continue;
                if (it->second.rn == base_reg && it->second.opcode == Opcode::MovLPcRel) {
                    table = read_u32_vaddr(elf, it->second.effective_address);
                    break;
                }
                if (writes_register(it->second, base_reg)) break;
            }
        }
        if (!table || (*table & 3u) != 0u) return;

        std::vector<std::uint32_t> targets;
        constexpr std::uint32_t kMaxEntries = 256u;
        std::uint32_t table_bytes = 0u;
        for (std::uint32_t n = 0u; n < kMaxEntries; ++n) {
            const std::uint32_t slot = *table + n * 4u;
            const auto value = read_u32_vaddr(elf, slot);
            if (!value || (*value & 1u) != 0u) break;

            const bool local_block = in_range(range, *value, 2);
            const bool clean_callable = has_clean_sh4_prefix(elf, *value) ||
                                        has_compact_return_thunk(elf, *value);
            if (!local_block && !clean_callable) break;
            // A JSR table is a callable-family dispatch even if a target happens
            // to lie inside the current synthetic symbol range. Keep the stronger
            // callable-entry requirement for every JSR target.
            if (branch.opcode == Opcode::Jsr && !clean_callable) break;

            if (std::find(targets.begin(), targets.end(), *value) == targets.end()) targets.push_back(*value);
            table_bytes += 4u;
            if (local_block) enqueue(*value);
        }
        if (targets.size() < 2u) return;

        if (table_bytes <= 255u) literal_spans.push_back({*table, static_cast<std::uint8_t>(table_bytes)});
        DynamicBranchReference ref;
        ref.instruction_address = branch.address;
        ref.targets = std::move(targets);
        result.dynamic_branches.push_back(std::move(ref));
    };

    auto discover_nested_absolute_jump_table = [&](const sh4::Instruction& branch) {
        // Some retail SDK dispatchers use a two-dimensional table:
        //
        //   mov.l top_table,rBase
        //   ... index0 -> R0 ...
        //   mov.l @(r0,rBase),r0       ; select a row/object
        //   ... index1 -> rIndex ...
        //   mov.l @(r0,rIndex),rTarget ; select a method
        //   jsr @rTarget
        //
        // A flat absolute-table scanner cannot follow this because the first
        // table contains pointers to data rows rather than code. Recover it only
        // when at least two consecutive rows each begin with >=3 clean callable
        // pointers. That is substantially stronger evidence than a single
        // address-looking data word and avoids promoting arbitrary pointer graphs.
        if (branch.opcode != Opcode::Jmp && branch.opcode != Opcode::Jsr) return;

        const sh4::Instruction* final_indexed = nullptr;
        std::uint32_t final_address = 0u;
        for (std::uint32_t back = 2u; back <= 8u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovLIndexedLoad && it->second.rn == branch.rm) {
                final_indexed = &it->second;
                final_address = it->first;
                break;
            }
            if (writes_register(it->second, branch.rm)) break;
        }
        if (!final_indexed) return;

        // The row pointer must have been loaded into architectural R0 by an
        // earlier indexed load. Reject paths that overwrite R0 between the row
        // selection and the final method selection.
        const sh4::Instruction* row_indexed = nullptr;
        std::uint32_t row_address = 0u;
        for (std::uint32_t back = 2u; back <= 20u && final_address >= range.start + back; back += 2u) {
            const auto it = code.find(final_address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovLIndexedLoad && it->second.rn == 0u) {
                row_indexed = &it->second;
                row_address = it->first;
                break;
            }
            if (writes_register(it->second, 0u)) break;
        }
        if (!row_indexed || row_indexed->rm == 0u) return;

        // For the row-select load, require the explicit Rm operand to be a
        // PC-relative literal holding the top-level row table. The implicit R0
        // operand is the scaled runtime index in this SDK pattern.
        std::optional<std::uint32_t> top_table;
        for (std::uint32_t back = 2u; back <= 24u && row_address >= range.start + back; back += 2u) {
            const auto it = code.find(row_address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovLPcRel && it->second.rn == row_indexed->rm) {
                top_table = read_u32_vaddr(elf, it->second.effective_address);
                break;
            }
            if (writes_register(it->second, row_indexed->rm)) break;
        }
        if (!top_table || (*top_table & 3u) != 0u) return;

        constexpr std::uint32_t kMaxRows = 64u;
        constexpr std::uint32_t kMaxMethodsPerRow = 16u;
        constexpr std::size_t kMinMethodsPerRow = 3u;
        constexpr std::size_t kMinRows = 2u;
        std::vector<std::uint32_t> targets;
        std::size_t valid_rows = 0u;
        std::uint32_t top_table_bytes = 0u;
        bool stop_after_next_strong_row = false;

        for (std::uint32_t row = 0u; row < kMaxRows; ++row) {
            const bool is_fallback_lookahead = stop_after_next_strong_row;
            const auto row_ptr = read_u32_vaddr(elf, *top_table + row * 4u);
            if (!row_ptr || (*row_ptr & 3u) != 0u) break;

            std::vector<std::uint32_t> row_targets;
            bool row_used_short_tail_fallback = false;
            for (std::uint32_t method = 0u; method < kMaxMethodsPerRow; ++method) {
                const auto value = read_u32_vaddr(elf, *row_ptr + method * 4u);
                if (!value || (*value & 1u) != 0u) break;
                const bool strong_callable = has_clean_sh4_prefix(elf, *value) ||
                                             has_compact_return_thunk(elf, *value) ||
                                             has_compact_literal_tail_thunk(elf, *value) ||
                                             has_conditional_literal_tail_thunk(elf, *value) ||
                                             has_indexed_literal_tail_dispatch_thunk(elf, *value);
                // Only the minimum method prefix may use the short-wrapper
                // fallback. Once a row needs that fallback, accept that row but
                // stop extending the table afterwards. This recovers SDK rows
                // that contain a literal-pool-heavy wrapper without turning a
                // heterogeneous top-level object table into an unbounded code
                // graph.
                const bool fallback_callable = !strong_callable && method < kMinMethodsPerRow &&
                                               has_short_call_then_tail_entry(elf, *value);
                if (!strong_callable && !fallback_callable) break;
                row_used_short_tail_fallback |= fallback_callable;
                if (std::find(row_targets.begin(), row_targets.end(), *value) == row_targets.end())
                    row_targets.push_back(*value);
            }

            if (row_targets.size() < kMinMethodsPerRow) break;
            ++valid_rows;
            top_table_bytes += 4u;
            for (const auto target : row_targets) {
                if (std::find(targets.begin(), targets.end(), target) == targets.end())
                    targets.push_back(target);
            }
            // A short-wrapper fallback marks a confidence boundary. Preserve the
            // old conservative stop, but inspect one immediately adjacent row.
            // If that next row independently has a clean callable prefix, include
            // it and stop. This recovers SDK state rows that sit directly after a
            // literal-pool-heavy row without opening the rest of the object graph.
            if (is_fallback_lookahead) break;
            if (row_used_short_tail_fallback) stop_after_next_strong_row = true;
        }

        // A second retail pattern aliases a later state row into the middle of
        // the immediately preceding method row (for example row N+1 == row N +
        // three function pointers).  This is much stronger evidence than merely
        // continuing through heterogeneous rows: both top-level entries name the
        // same contiguous method array.  Recover only the bounded prefix needed to
        // cover the overlap plus three independently callable methods.
        for (std::uint32_t row = 0u; row + 1u < kMaxRows; ++row) {
            const auto row_ptr = read_u32_vaddr(elf, *top_table + row * 4u);
            const auto next_ptr = read_u32_vaddr(elf, *top_table + (row + 1u) * 4u);
            if (!row_ptr || !next_ptr || (*row_ptr & 3u) != 0u || (*next_ptr & 3u) != 0u) continue;
            if (*next_ptr <= *row_ptr) continue;
            const auto delta = *next_ptr - *row_ptr;
            if ((delta & 3u) != 0u) continue;
            const auto overlap_method = delta / 4u;
            if (overlap_method < kMinMethodsPerRow ||
                overlap_method + kMinMethodsPerRow > kMaxMethodsPerRow) continue;

            std::vector<std::uint32_t> alias_targets;
            const auto required = overlap_method + static_cast<std::uint32_t>(kMinMethodsPerRow);
            for (std::uint32_t method = 0u; method < required; ++method) {
                const auto value = read_u32_vaddr(elf, *row_ptr + method * 4u);
                if (!value || (*value & 1u) != 0u) { alias_targets.clear(); break; }
                const bool callable = has_clean_sh4_prefix(elf, *value) ||
                                      has_compact_return_thunk(elf, *value) ||
                                      has_compact_literal_tail_thunk(elf, *value) ||
                                      has_conditional_literal_tail_thunk(elf, *value) ||
                                      has_indexed_literal_tail_dispatch_thunk(elf, *value) ||
                                      has_short_call_then_tail_entry(elf, *value);
                if (!callable) { alias_targets.clear(); break; }
                alias_targets.push_back(*value);
            }
            if (alias_targets.size() != required) continue;
            for (const auto target : alias_targets) {
                if (std::find(targets.begin(), targets.end(), target) == targets.end())
                    targets.push_back(target);
            }
        }

        // Some SDK state machines intentionally alias two consecutive state
        // indices to the exact same method row. An exact duplicate row pointer is
        // strong structural evidence, but still require a strict callable prefix
        // before promoting anything from it.
        for (std::uint32_t row = 0u; row + 1u < kMaxRows; ++row) {
            const auto row_ptr = read_u32_vaddr(elf, *top_table + row * 4u);
            const auto next_ptr = read_u32_vaddr(elf, *top_table + (row + 1u) * 4u);
            if (!row_ptr || !next_ptr || *row_ptr != *next_ptr || (*row_ptr & 3u) != 0u) continue;

            std::vector<std::uint32_t> duplicate_targets;
            bool duplicate_valid = true;
            for (std::uint32_t method = 0u; method < kMinMethodsPerRow; ++method) {
                const auto value = read_u32_vaddr(elf, *row_ptr + method * 4u);
                if (!value || (*value & 1u) != 0u) { duplicate_valid = false; break; }
                const bool callable = has_clean_sh4_prefix(elf, *value) ||
                                      has_compact_return_thunk(elf, *value) ||
                                      has_compact_literal_tail_thunk(elf, *value) ||
                                      has_conditional_literal_tail_thunk(elf, *value) ||
                                      has_indexed_literal_tail_dispatch_thunk(elf, *value) ||
                                      has_short_call_then_tail_entry(elf, *value);
                if (!callable) { duplicate_valid = false; break; }
                duplicate_targets.push_back(*value);
            }
            if (!duplicate_valid || duplicate_targets.size() != kMinMethodsPerRow) continue;
            for (const auto target : duplicate_targets) {
                if (std::find(targets.begin(), targets.end(), target) == targets.end())
                    targets.push_back(target);
            }
        }

        // If the conservative contiguous scan stops on a short uncertain gap,
        // but a later row in the same top-level state table is already known as
        // executable from independent closure evidence, bridge only that bounded
        // gap. Every missing row must expose a strict three-method callable prefix.
        // After a successful bridge, continue from that independently verified
        // anchor and allow another short bridge. This handles SDK state tables
        // with more than one compact hole while still stopping before any long
        // unverified region or heterogeneous data tail.
        if (valid_rows >= kMinRows) {
            constexpr std::uint32_t kMaxBridgeRows = 4u;
            std::uint32_t gap_begin = static_cast<std::uint32_t>(valid_rows);
            constexpr std::uint32_t kMaxChainedBridges = 2u;
            std::uint32_t chained_bridges = 0u;

            while (gap_begin < kMaxRows && chained_bridges < kMaxChainedBridges) {
                std::optional<std::uint32_t> anchor_row;
                for (std::uint32_t row = gap_begin + 1u;
                     row <= gap_begin + kMaxBridgeRows && row < kMaxRows; ++row) {
                    const auto row_ptr = read_u32_vaddr(elf, *top_table + row * 4u);
                    if (!row_ptr || (*row_ptr & 3u) != 0u) break;
                    bool known_prefix = true;
                    for (std::uint32_t method = 0u; method < kMinMethodsPerRow; ++method) {
                        const auto value = read_u32_vaddr(elf, *row_ptr + method * 4u);
                        if (!value) { known_prefix = false; break; }
                        const auto* symbol = find_symbol_at(elf, *value);
                        if (!symbol || !is_callable_code_symbol(elf, *symbol)) {
                            known_prefix = false;
                            break;
                        }
                    }
                    if (known_prefix) { anchor_row = row; break; }
                }
                if (!anchor_row) break;

                std::vector<std::uint32_t> bridge_targets;
                bool bridge_valid = true;
                for (std::uint32_t row = gap_begin; row < *anchor_row && bridge_valid; ++row) {
                    const auto row_ptr = read_u32_vaddr(elf, *top_table + row * 4u);
                    if (!row_ptr || (*row_ptr & 3u) != 0u) { bridge_valid = false; break; }
                    for (std::uint32_t method = 0u; method < kMinMethodsPerRow; ++method) {
                        const auto value = read_u32_vaddr(elf, *row_ptr + method * 4u);
                        if (!value || (*value & 1u) != 0u) { bridge_valid = false; break; }
                        const bool callable = has_clean_sh4_prefix(elf, *value) ||
                                              has_compact_return_thunk(elf, *value) ||
                                              has_compact_literal_tail_thunk(elf, *value) ||
                                              has_conditional_literal_tail_thunk(elf, *value) ||
                                              has_indexed_literal_tail_dispatch_thunk(elf, *value);
                        if (!callable) { bridge_valid = false; break; }
                        // A one-hop literal JMP veneer inside an otherwise
                        // unverified gap is sufficient to validate the method
                        // slot, but recursively promoting it can immediately
                        // open a different selector family. Leave that veneer
                        // for direct runtime evidence; promote the substantive
                        // methods in the bridged row now.
                        if (!has_compact_literal_tail_thunk(elf, *value))
                            bridge_targets.push_back(*value);
                    }
                }
                if (!bridge_valid) break;

                for (const auto target : bridge_targets) {
                    if (std::find(targets.begin(), targets.end(), target) == targets.end())
                        targets.push_back(target);
                }
                // The anchor itself was independently known before this analysis;
                // continue immediately after it and seek at most one short gap away.
                gap_begin = *anchor_row + 1u;
                ++chained_bridges;
            }
        }

        if (valid_rows < kMinRows || targets.empty()) return;
        if (top_table_bytes <= 255u)
            literal_spans.push_back({*top_table, static_cast<std::uint8_t>(top_table_bytes)});

        DynamicBranchReference ref;
        ref.instruction_address = branch.address;
        ref.targets = std::move(targets);
        result.dynamic_branches.push_back(std::move(ref));
    };

    auto discover_constant_braf = [&](const sh4::Instruction& branch) {
        // GCC/newlib sometimes materializes a constant relative BRAF offset in
        // the immediately preceding instruction instead of using BRA (usually
        // because the destination is outside BRA's displacement range). Treat
        // this as a statically-known edge so the destination is emitted.
        for (std::uint32_t back = 2u; back <= 8u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            std::optional<std::int32_t> delta;
            if (it->second.rn == branch.rn && it->second.opcode == Opcode::MovWPcRel) {
                if (const auto raw = read_u16_vaddr(elf, it->second.effective_address))
                    delta = static_cast<std::int32_t>(static_cast<std::int16_t>(*raw));
            } else if (it->second.rn == branch.rn && it->second.opcode == Opcode::MovLPcRel) {
                if (const auto raw = read_u32_vaddr(elf, it->second.effective_address))
                    delta = static_cast<std::int32_t>(*raw);
            } else if (it->second.rn == branch.rn && it->second.opcode == Opcode::MovImm) {
                delta = it->second.immediate;
            }
            if (delta) {
                const std::uint32_t target = static_cast<std::uint32_t>(branch.address + 4u + *delta);
                if ((target & 1u) == 0u && in_range(range, target, 2)) {
                    DynamicBranchReference ref;
                    ref.instruction_address = branch.address;
                    ref.targets.push_back(target);
                    result.dynamic_branches.push_back(std::move(ref));
                    enqueue(target);
                }
                return;
            }
            if (writes_register(it->second, branch.rn)) return;
        }
    };

    auto discover_word_jump_table = [&](const sh4::Instruction& branch) {
        // GCC/newlib also emits wider SH-4 switch tables of signed 16-bit
        // displacements, commonly as:
        //   mov.l @(table_literal,pc),r0 ; add Rn,Rn
        //   mov.w @(r0,Rn),Rn           ; braf Rn
        // The table entries are signed byte displacements from BRAF PC+4.
        // This pattern is used by newlib's printf format dispatch and is also
        // relevant to symbol-less commercial code, so resolve it statically.
        const sh4::Instruction* load = nullptr;
        std::uint32_t load_address = 0u;
        for (std::uint32_t back = 2u; back <= 10u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovWIndexedLoad &&
                it->second.rn == branch.rn) {
                load = &it->second;
                load_address = it->first;
                break;
            }
            // ADD Rn,Rn is the usual x2 scaling for a 16-bit table and is
            // expected between the table-base load and MOV.W, so allow it.
            if (it->second.opcode == Opcode::AddReg &&
                it->second.rn == branch.rn && it->second.rm == branch.rn) continue;
            if (writes_register(it->second, branch.rn)) break;
        }
        if (!load) return;

        std::optional<std::uint32_t> table;
        for (std::uint32_t back = 2u; back <= 16u && load_address >= range.start + back; back += 2u) {
            const auto it = code.find(load_address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::Mova) {
                table = it->second.effective_address;
                break;
            }
            if (it->second.opcode == Opcode::MovLPcRel && it->second.rn == 0u) {
                table = read_u32_vaddr(elf, it->second.effective_address);
                break;
            }
            if (writes_register(it->second, 0u)) break;
        }
        if (!table) return;

        // MOV.W @(R0,Rm),Rn may use a copied/scaled index register. Katana
        // code commonly does SHLL R0; MOV R0,R1; MOV.L table,R0;
        // MOV.W @(R0,R1),R0; BRAF R0. Recover the pre-copy register so a
        // preceding CMP/HS range check can still provide the table length.
        std::uint8_t index_reg = load->rm;
        for (std::uint32_t back = 2u; back <= 24u && load_address >= range.start + back; back += 2u) {
            const auto it = code.find(load_address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovReg && it->second.rn == index_reg) {
                index_reg = it->second.rm;
                break;
            }
            if (writes_register(it->second, index_reg)) break;
        }

        std::optional<std::uint32_t> count;
        // The range check may be several basic blocks before the dispatch
        // (newlib __svfprintf_r is a real example), so search a wider window.
        // CMP/HI index,bound with BT means valid indices are 0..bound, whereas
        // CMP/HS bound,index with BT means valid indices are 0..bound-1.
        for (std::uint32_t back = 6u; back <= 512u && branch.address >= range.start + back; back += 2u) {
            const auto cmp_it = code.find(branch.address - back);
            if (cmp_it == code.end() || cmp_it->second.rn != index_reg ||
                (cmp_it->second.opcode != Opcode::CmpHi && cmp_it->second.opcode != Opcode::CmpHs)) continue;
            const auto bound_reg = cmp_it->second.rm;
            for (std::uint32_t back2 = back + 2u; back2 <= back + 48u && branch.address >= range.start + back2; back2 += 2u) {
                const auto def_it = code.find(branch.address - back2);
                if (def_it == code.end()) continue;
                if (def_it->second.opcode == Opcode::MovImm && def_it->second.rn == bound_reg &&
                    def_it->second.immediate >= 0 && def_it->second.immediate < 1024) {
                    const auto bound = static_cast<std::uint32_t>(def_it->second.immediate);
                    count = cmp_it->second.opcode == Opcode::CmpHs ? bound : bound + 1u;
                    break;
                }
                if (writes_register(def_it->second, bound_reg)) break;
            }
            if (count) break;
        }
        if (!count || *count == 0u) return;

        const std::size_t table_bytes = static_cast<std::size_t>(*count) * 2u;
        const auto table_off = file_offset_for_vaddr(elf, *table, table_bytes);
        if (!table_off || table_bytes > 255u) return;
        literal_spans.push_back({*table, static_cast<std::uint8_t>(table_bytes)});

        DynamicBranchReference ref;
        ref.instruction_address = branch.address;
        for (std::uint32_t n = 0; n < *count; ++n) {
            const auto lo = static_cast<std::uint16_t>(elf.bytes[*table_off + n * 2u]);
            const auto hi = static_cast<std::uint16_t>(elf.bytes[*table_off + n * 2u + 1u]);
            const auto raw = static_cast<std::uint16_t>(lo | (hi << 8));
            const std::int32_t delta = static_cast<std::int32_t>(static_cast<std::int16_t>(raw));
            const std::uint32_t target = static_cast<std::uint32_t>(branch.address + 4u + delta);
            if ((target & 1u) != 0u || !in_range(range, target, 2)) continue;
            if (std::find(ref.targets.begin(), ref.targets.end(), target) == ref.targets.end()) {
                ref.targets.push_back(target);
                enqueue(target);
            }
        }
        if (!ref.targets.empty()) result.dynamic_branches.push_back(std::move(ref));
    };

    auto discover_masked_braf_range = [&](const sh4::Instruction& branch) {
        // Compiler-generated local dispatch can derive the BRAF displacement
        // directly from a masked selector instead of loading a jump table:
        //
        //   ... value in R0 ...
        //   and   #(2^n-1),r0
        //   shll2 r0
        //   braf  r0
        //
        // The AND proves a finite set of possible register values independent of
        // the incoming value.  Recover that finite set as CFG edges.  Keep the
        // recognizer intentionally narrow: AND-immediate is architecturally R0,
        // the mask must be a contiguous low-bit mask, only fixed left shifts may
        // modify R0 afterwards, and the resulting fan-out is capped.
        if (branch.opcode != Opcode::Braf || branch.rn != 0u) return;

        std::optional<std::uint32_t> mask;
        unsigned shift_bits = 0u;
        for (std::uint32_t back = 2u; back <= 16u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            const auto& d = it->second;
            if (d.opcode == Opcode::Shll && d.rn == 0u) { ++shift_bits; continue; }
            if (d.opcode == Opcode::Shll2 && d.rn == 0u) { shift_bits += 2u; continue; }
            if (d.opcode == Opcode::Shll8 && d.rn == 0u) { shift_bits += 8u; continue; }
            if (d.opcode == Opcode::Shll16 && d.rn == 0u) { shift_bits += 16u; continue; }
            if (d.opcode == Opcode::AndImm) {
                mask = static_cast<std::uint32_t>(d.immediate) & 0xFFu;
                break;
            }
            if (writes_register(d, 0u)) return;
        }
        if (!mask || *mask == 0u || shift_bits > 4u) return;
        // 1,3,7,15,... are exactly the contiguous low-bit masks.
        if (((*mask + 1u) & *mask) != 0u) return;
        const std::uint32_t count = *mask + 1u;
        if (count < 3u || count > 32u) return;

        DynamicBranchReference ref;
        ref.instruction_address = branch.address;
        const std::uint32_t base_pc = branch.address + 4u;
        for (std::uint32_t value = 0u; value < count; ++value) {
            const std::uint64_t target64 = static_cast<std::uint64_t>(base_pc) +
                                           (static_cast<std::uint64_t>(value) << shift_bits);
            if (target64 > 0xFFFFFFFFull) return;
            const auto target = static_cast<std::uint32_t>(target64);
            if ((target & 1u) != 0u || !in_range(range, target, 2u)) return;
            const auto raw = read_u16_vaddr(elf, target);
            if (!raw || !sh4::is_known(sh4::decode(*raw, target))) return;
            ref.targets.push_back(target);
        }
        for (const auto target : ref.targets) enqueue(target);
        result.dynamic_branches.push_back(std::move(ref));
    };

    auto discover_inline_bra_trampoline_table = [&](const sh4::Instruction& branch) {
        // Some retail SH-4 code uses the BRAF register as a small scaled
        // selector into an inline table of four-byte BRA/NOP trampolines:
        //
        //   shll2 Rn
        //   ...                    // instructions preserving Rn
        //   braf  Rn
        //    nop                   // architectural delay slot
        //   nop                    // optional alignment/padding
        //   bra target0 ; nop
        //   bra target1 ; nop
        //   bra target2 ; nop
        //   ...
        //
        // Crazy Taxi 2 uses exactly this shape around 0x8C08138A.  The
        // selector's bound is established by an earlier helper rather than a
        // nearby CMP, so the existing masked/table recognizers cannot prove a
        // finite range.  The trampoline run itself is strong structural
        // evidence: require SHLL2 on the same register, at least three
        // consecutive BRA/NOP pairs, distinct local decoder-clean destinations,
        // and only search a small aligned window immediately after BRAF.
        if (branch.opcode != Opcode::Braf) return;

        bool scaled_by_four = false;
        for (std::uint32_t back = 2u; back <= 16u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            const auto& d = it->second;
            if (d.opcode == Opcode::Shll2 && d.rn == branch.rn) {
                scaled_by_four = true;
                break;
            }
            if (writes_register(d, branch.rn)) break;
        }
        if (!scaled_by_four) return;

        const std::uint32_t base_pc = branch.address + 4u;
        constexpr std::uint32_t kMaxStartOffset = 32u;
        constexpr std::uint32_t kMaxEntries = 16u;
        std::vector<std::uint32_t> best_entries;
        std::vector<std::uint32_t> best_destinations;

        for (std::uint32_t start_off = 0u; start_off <= kMaxStartOffset; start_off += 4u) {
            std::vector<std::uint32_t> entries;
            std::vector<std::uint32_t> destinations;
            for (std::uint32_t i = 0u; i < kMaxEntries; ++i) {
                const std::uint32_t entry = base_pc + start_off + i * 4u;
                if (!in_range(range, entry, 4u)) break;
                const auto bra_raw = read_u16_vaddr(elf, entry);
                const auto slot_raw = read_u16_vaddr(elf, entry + 2u);
                if (!bra_raw || !slot_raw || *slot_raw != 0x0009u) break;
                const auto bra = sh4::decode(*bra_raw, entry);
                if (bra.opcode != Opcode::Bra || (bra.target & 1u) != 0u ||
                    !in_range(range, bra.target, 2u)) break;
                const auto target_raw = read_u16_vaddr(elf, bra.target);
                if (!target_raw || !sh4::is_known(sh4::decode(*target_raw, bra.target))) break;
                if (std::find(destinations.begin(), destinations.end(), bra.target) != destinations.end()) break;
                entries.push_back(entry);
                destinations.push_back(bra.target);
            }
            if (entries.size() >= 3u && entries.size() > best_entries.size()) {
                best_entries = std::move(entries);
                best_destinations = std::move(destinations);
            }
        }

        if (best_entries.size() < 3u) return;

        DynamicBranchReference ref;
        ref.instruction_address = branch.address;
        ref.targets = best_entries; // BRAF lands on the trampoline entries.
        for (const auto entry : best_entries) enqueue(entry);
        // Enqueue destinations as well.  Decoding the BRA entries would discover
        // them eventually, but doing it here keeps the closure explicit and
        // robust if a trampoline entry is already present through another path.
        for (const auto target : best_destinations) enqueue(target);
        result.dynamic_branches.push_back(std::move(ref));
    };

    auto discover_long_jump_table = [&](const sh4::Instruction& branch) {
        // Retail Katana/GCC code also uses compact local BRAF tables whose
        // entries are signed 32-bit byte displacements.  Crazy Taxi 2 exposes
        // one of the smallest/cleanest examples in a memcpy-like helper:
        //
        //   mova   table,r0
        //   ... index preparation in Rm ...
        //   mov.l  @(r0,Rm),Rn
        //   ... instructions that leave Rn intact ...
        //   braf   Rn
        //    <delay slot>
        // table:
        //   .long  target0-(braf_pc+4)
        //   .long  target1-(braf_pc+4)
        //   ...
        //
        // Unlike the byte/word switch forms above, this helper has no nearby
        // CMP bound: the caller constrains the legal copy sizes.  Recover the
        // table conservatively by requiring the exact indexed-load shape, a
        // nearby statically-known table base, and a contiguous run of local,
        // even, decoder-clean destinations.  The first non-matching word ends
        // the table, which keeps adjacent code/data from being promoted.
        if (branch.opcode != Opcode::Braf) return;

        const sh4::Instruction* indexed = nullptr;
        std::uint32_t indexed_address = 0u;
        for (std::uint32_t back = 2u; back <= 16u && branch.address >= range.start + back; back += 2u) {
            const auto it = code.find(branch.address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovLIndexedLoad &&
                it->second.rn == branch.rn) {
                indexed = &it->second;
                indexed_address = it->first;
                break;
            }
            if (writes_register(it->second, branch.rn)) break;
        }
        if (!indexed) return;

        std::optional<std::uint32_t> table;
        // MOV.L @(R0,Rm),Rn can keep the table base in either adder input.
        // MOVA can only define R0; a PC-relative MOV.L may define either input
        // with a literal pointer to the actual table.
        const std::array<std::uint8_t, 2> candidate_base_regs{0u, indexed->rm};
        for (const auto base_reg : candidate_base_regs) {
            if (table) break;
            for (std::uint32_t back = 2u; back <= 32u && indexed_address >= range.start + back; back += 2u) {
                const auto it = code.find(indexed_address - back);
                if (it == code.end()) continue;
                if (base_reg == 0u && it->second.opcode == Opcode::Mova) {
                    table = it->second.effective_address;
                    break;
                }
                if (it->second.opcode == Opcode::MovLPcRel && it->second.rn == base_reg) {
                    table = read_u32_vaddr(elf, it->second.effective_address);
                    break;
                }
                if (writes_register(it->second, base_reg)) break;
            }
        }
        if (!table || ((*table) & 3u) != 0u) return;

        const std::uint32_t branch_base = branch.address + 4u;
        const std::uint64_t distance = *table > branch.address
            ? static_cast<std::uint64_t>(*table - branch.address)
            : static_cast<std::uint64_t>(branch.address - *table);
        // Local compiler BRAF tables live next to the dispatch body.  Keeping
        // this bound tight is intentional: large absolute tables are handled by
        // the dedicated JSR/JMP table recognizers instead.
        if (distance > 0x4000u) return;

        constexpr std::uint32_t kMaxEntries = 63u; // LiteralSpan stores uint8_t bytes.
        std::vector<std::uint32_t> targets;
        std::uint32_t entries = 0u;
        for (; entries < kMaxEntries; ++entries) {
            const std::uint32_t slot = *table + entries * 4u;
            const auto off = file_offset_for_vaddr(elf, slot, 4u);
            if (!off) break;
            const auto raw = static_cast<std::uint32_t>(elf.bytes[*off]) |
                             (static_cast<std::uint32_t>(elf.bytes[*off + 1u]) << 8u) |
                             (static_cast<std::uint32_t>(elf.bytes[*off + 2u]) << 16u) |
                             (static_cast<std::uint32_t>(elf.bytes[*off + 3u]) << 24u);
            const auto delta = static_cast<std::int32_t>(raw);
            const auto target64 = static_cast<std::int64_t>(branch_base) + static_cast<std::int64_t>(delta);
            if (target64 < 0 || target64 > 0xFFFFFFFFll) break;
            const auto target = static_cast<std::uint32_t>(target64);
            if ((target & 1u) != 0u || !in_range(range, target, 2u)) break;
            const auto first_raw = read_u16_vaddr(elf, target);
            if (!first_raw || !sh4::is_known(sh4::decode(*first_raw, target))) break;

            if (std::find(targets.begin(), targets.end(), target) == targets.end())
                targets.push_back(target);
        }

        // One or two coincidental integers are not enough structural evidence.
        // Real compiler tables contain several entries; Crazy Taxi 2 has eight.
        if (entries < 3u || targets.size() < 2u) return;

        literal_spans.push_back({*table, static_cast<std::uint8_t>(entries * 4u)});
        DynamicBranchReference ref;
        ref.instruction_address = branch.address;
        ref.targets = targets;
        for (const auto target : targets) enqueue(target);
        result.dynamic_branches.push_back(std::move(ref));
    };

    enqueue(entry_address);

    while (!pending.empty()) {
        const auto address = *pending.begin();
        pending.erase(pending.begin());
        if (address_in_literal(literal_spans, address)) continue;
        if (code.contains(address)) {
            if (recorded_delay_slots.contains(address) && expanded_delay_slot_entries.insert(address).second) {
                const auto next = address + 2u;
                if (in_range(range, next, 2) && !address_in_literal(literal_spans, next) && !code.contains(next))
                    pending.insert(next);
            }
            continue;
        }

        const auto insn_opt = decode_at(address);
        if (!insn_opt) {
            continue;
        }
        const auto insn = *insn_opt;
        code.emplace(address, insn);

        switch (insn.opcode) {
            case Opcode::Rts:
            case Opcode::Rte:
                record_delay_slot(address + 2);
                break;

            case Opcode::Bra:
                record_delay_slot(address + 2);
                enqueue(insn.target);
                break;

            case Opcode::Bsr:
                record_delay_slot(address + 2);
                enqueue(address + 4); // return site
                break;

            case Opcode::Bt:
            case Opcode::Bf:
                enqueue(insn.target);
                enqueue(address + 2);
                break;

            case Opcode::BtS:
            case Opcode::BfS:
                record_delay_slot(address + 2);
                enqueue(insn.target);
                enqueue(address + 4);
                break;

            case Opcode::Braf:
                record_delay_slot(address + 2);
                discover_constant_braf(insn);
                discover_masked_braf_range(insn);
                discover_inline_bra_trampoline_table(insn);
                discover_byte_jump_table(insn);
                discover_word_jump_table(insn);
                discover_long_jump_table(insn);
                break;

            case Opcode::Bsrf:
                record_delay_slot(address + 2);
                enqueue(address + 4); // return site
                break;

            case Opcode::Jmp:
                record_delay_slot(address + 2);
                discover_absolute_jump_table(insn);
                discover_nested_absolute_jump_table(insn);
                break;

            case Opcode::Jsr:
                record_delay_slot(address + 2);
                discover_absolute_jump_table(insn);
                discover_nested_absolute_jump_table(insn);
                enqueue(address + 4); // return site
                break;

            default:
                enqueue(address + 2);
                break;
        }
    }

    result.instructions.reserve(code.size());
    for (const auto& [address, insn] : code) {
        (void)address;
        result.instructions.push_back(insn);
        if (sh4::is_known(insn)) ++result.known;
        else ++result.unknown;
    }

    // Recover branch-selected literal targets for indirect JSR/JMP calls. Retail
    // Katana code frequently chooses between two callable pointers with a delayed
    // conditional branch, then joins at one indirect call:
    //
    //     mov.l callback_a,r3
    //     bf/s  .join
    //      <delay slot>
    //     mov.l callback_b,r3
    // .join:
    //     jsr   @r3
    //
    // Straight-line constant propagation correctly resolves callback_b but cannot
    // represent the alternate predecessor value callback_a. Keep that alternate as
    // a DynamicBranchReference so raw commercial closure discovery can register
    // both native callees. The pattern is deliberately bounded and requires actual
    // PC-relative literals plus a branch that skips the overriding load.
    auto merge_dynamic_target = [&](std::uint32_t branch_address, std::uint32_t target) {
        for (auto& ref : result.branch_selected_calls) {
            if (ref.instruction_address != branch_address) continue;
            if (std::find(ref.targets.begin(), ref.targets.end(), target) == ref.targets.end())
                ref.targets.push_back(target);
            return;
        }
        DynamicBranchReference ref;
        ref.instruction_address = branch_address;
        ref.targets.push_back(target);
        result.branch_selected_calls.push_back(std::move(ref));
    };

    auto discover_branch_selected_literal_call = [&](const sh4::Instruction& call) {
        if (call.opcode != Opcode::Jsr && call.opcode != Opcode::Jmp) return;
        const auto reg = call.rm;

        // The overriding literal load normally sits only a few instructions before
        // the join call. Stop at the first write to the dispatch register because
        // that is the value reaching the fall-through path.
        const sh4::Instruction* override_load = nullptr;
        std::uint32_t override_address = 0u;
        for (std::uint32_t back = 2u; back <= 16u && call.address >= range.start + back; back += 2u) {
            const auto it = code.find(call.address - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovLPcRel && it->second.rn == reg) {
                override_load = &it->second;
                override_address = it->first;
                break;
            }
            if (writes_register(it->second, reg)) break;
        }
        if (!override_load) return;

        // Recognize either BT/BF immediately before the overriding load or the
        // delayed BT/S,BF/S form with exactly one architectural delay slot.
        const sh4::Instruction* selector = nullptr;
        std::uint32_t selector_address = 0u;
        bool delayed = false;
        if (override_address >= range.start + 2u) {
            const auto it = code.find(override_address - 2u);
            if (it != code.end() && (it->second.opcode == Opcode::Bt || it->second.opcode == Opcode::Bf)) {
                selector = &it->second;
                selector_address = it->first;
            }
        }
        if (!selector && override_address >= range.start + 4u) {
            const auto it = code.find(override_address - 4u);
            const auto slot = code.find(override_address - 2u);
            if (it != code.end() && slot != code.end() &&
                (it->second.opcode == Opcode::BtS || it->second.opcode == Opcode::BfS) &&
                !writes_register(slot->second, reg)) {
                selector = &it->second;
                selector_address = it->first;
                delayed = true;
            }
        }
        if (!selector) return;

        // The taken path must skip the overriding literal and rejoin no later than
        // the indirect call; otherwise callback_a is not proven to reach this call.
        if (selector->target <= override_address || selector->target > call.address) return;

        // Fall-through target (callback_b) is also useful to preserve in the
        // dynamic target set, even though ordinary call resolution normally finds it.
        if (const auto value = read_u32_vaddr(elf, override_load->effective_address))
            merge_dynamic_target(call.address, *value);

        // Search the predecessor path for the earlier literal assignment. Do not
        // cross another write to the dispatch register. For delayed branches begin
        // before the branch itself; the delay slot was already proven not to clobber.
        const auto before_selector = selector_address;
        for (std::uint32_t back = 2u; back <= 32u && before_selector >= range.start + back; back += 2u) {
            const auto it = code.find(before_selector - back);
            if (it == code.end()) continue;
            if (it->second.opcode == Opcode::MovLPcRel && it->second.rn == reg) {
                if (const auto value = read_u32_vaddr(elf, it->second.effective_address))
                    merge_dynamic_target(call.address, *value);
                return;
            }
            if (writes_register(it->second, reg)) return;
        }
        (void)delayed;
    };

    for (const auto& [address, insn] : code) {
        (void)address;
        discover_branch_selected_literal_call(insn);
    }

    // Resolve calls. BSR has a direct target; JSR is resolved conservatively from
    // constants loaded in the same straight-line block. Branch-join alternatives
    // are represented separately in dynamic_branches above.
    for (std::size_t i = 0; i < result.instructions.size(); ++i) {
        const auto& insn = result.instructions[i];
        if (insn.opcode != Opcode::Bsr && insn.opcode != Opcode::Bsrf &&
            insn.opcode != Opcode::Jsr && insn.opcode != Opcode::Jmp) {
            continue;
        }

        CallReference call;
        call.instruction_address = insn.address;
        call.direct = insn.opcode == Opcode::Bsr;

        std::optional<std::uint32_t> target;
        if (insn.opcode == Opcode::Bsr) {
            target = insn.target;
        } else if (insn.opcode == Opcode::Bsrf) {
            // BSRF Rn is a PC-relative call through a register: target = PC+4+Rn.
            // Symbol-less Katana binaries use this form often enough that omitting
            // it from call discovery leaves real commercial functions unregistered.
            auto delta = resolve_register_before(result.instructions, result.literals, i, insn.rn);
            if (!delta) delta = resolve_register_filebacked_before(elf, result.instructions, result.literals, i, insn.rn, true);
            if (!delta) delta = resolve_register_linear_before(result.instructions, result.literals, i, insn.rn);
            if (!delta) delta = resolve_register_filebacked_before(elf, result.instructions, result.literals, i, insn.rn, false);
            if (delta) target = static_cast<std::uint32_t>(insn.address + 4u + *delta);
        } else {
            // JSR is a normal indirect call; JMP is frequently GCC's sibling/tail
            // call form. Both need the target in ProgramAnalysis so native x64 has
            // the callee registered when the dynamic control transfer executes.
            target = resolve_register_before(result.instructions, result.literals, i, insn.rm);
            if (!target) {
                // 0.0.170: resolve file-backed pointer cells/vtable slots before
                // falling back to the older branch-insensitive scan. This recovers
                // Katana bootstrap/ops-table calls without guessing runtime RAM.
                target = resolve_register_filebacked_before(elf, result.instructions, result.literals, i, insn.rm, true);
            }
            if (!target) {
                // Fallback for real GCC code that loads a function pointer once and
                // keeps it live across a loop/back-edge before JSR @Rn (e.g. KOS
                // streaming I/O). This is used only when straight-line proof fails.
                target = resolve_register_linear_before(result.instructions, result.literals, i, insn.rm);
            }
            if (!target) {
                target = resolve_register_filebacked_before(elf, result.instructions, result.literals, i, insn.rm, false);
            }
        }

        if (target) {
            call.resolved = true;
            call.target = *target;
            call.symbol = symbol_name(elf, *target);
            call.section = section_name(elf, *target);
        }
        result.calls.push_back(std::move(call));
    }

    // Prune false fallthrough after calls to well-known noreturn routines.
    // The initial exploration is intentionally conservative so indirect JSR targets
    // can be resolved from straight-line constants first. Once call targets are known,
    // a second reachability pass removes instructions that were reachable only by
    // pretending abort/exit/assert could return. This is important for real GCC/newlib
    // functions that place literal pools or jump tables immediately after such calls.
    std::set<std::uint32_t> noreturn_calls;
    for (const auto& call : result.calls) {
        if (call.resolved && is_known_noreturn_symbol(call.symbol)) {
            noreturn_calls.insert(call.instruction_address);
        }
    }

    if (!noreturn_calls.empty()) {
        std::set<std::uint32_t> reachable;
        std::set<std::uint32_t> work;
        work.insert(range.start);

        auto mark_delay_slot = [&](std::uint32_t address) {
            if (code.contains(address)) reachable.insert(address);
        };
        auto push_code = [&](std::uint32_t address) {
            if (code.contains(address) && !reachable.contains(address)) work.insert(address);
        };

        while (!work.empty()) {
            const auto address = *work.begin();
            work.erase(work.begin());
            const auto it = code.find(address);
            if (it == code.end() || !reachable.insert(address).second) continue;

            const auto& insn = it->second;
            switch (insn.opcode) {
                case Opcode::Rts:
                case Opcode::Rte:
                    mark_delay_slot(address + 2);
                    break;
                case Opcode::Bra:
                    mark_delay_slot(address + 2);
                    push_code(insn.target);
                    break;
                case Opcode::Bsr:
                    mark_delay_slot(address + 2);
                    if (!noreturn_calls.contains(address)) push_code(address + 4);
                    break;
                case Opcode::Bt:
                case Opcode::Bf:
                    push_code(insn.target);
                    push_code(address + 2);
                    break;
                case Opcode::BtS:
                case Opcode::BfS:
                    mark_delay_slot(address + 2);
                    push_code(insn.target);
                    push_code(address + 4);
                    break;
                case Opcode::Braf:
                    mark_delay_slot(address + 2);
                    for (const auto& dynamic : result.dynamic_branches) {
                        if (dynamic.instruction_address == address) {
                            for (const auto target : dynamic.targets) push_code(target);
                            break;
                        }
                    }
                    break;
                case Opcode::Jmp:
                    mark_delay_slot(address + 2);
                    for (const auto& dynamic : result.dynamic_branches) {
                        if (dynamic.instruction_address == address) {
                            for (const auto target : dynamic.targets) push_code(target);
                            break;
                        }
                    }
                    break;
                case Opcode::Bsrf:
                    mark_delay_slot(address + 2);
                    push_code(address + 4);
                    break;
                case Opcode::Jsr:
                    mark_delay_slot(address + 2);
                    if (!noreturn_calls.contains(address)) push_code(address + 4);
                    break;
                default:
                    push_code(address + 2);
                    break;
            }
        }

        for (auto it = code.begin(); it != code.end();) {
            if (!reachable.contains(it->first)) it = code.erase(it);
            else ++it;
        }

        result.instructions.clear();
        result.known = 0;
        result.unknown = 0;
        result.instructions.reserve(code.size());
        for (const auto& [address, insn] : code) {
            (void)address;
            result.instructions.push_back(insn);
            if (sh4::is_known(insn)) ++result.known;
            else ++result.unknown;
        }

        result.calls.erase(std::remove_if(result.calls.begin(), result.calls.end(), [&](const auto& call) {
            return !code.contains(call.instruction_address);
        }), result.calls.end());
    }

    // Any word in an authoritative ELF st_size that is neither reachable code nor
    // a literal is alignment padding/unreachable material. Raw commercial images
    // use a synthetic .raw_boot section whose symbol sizes are deliberately broad
    // analysis envelopes (there is no real st_size metadata), so scanning the whole
    // envelope as padding would be both misleading and O(functions * image-size).
    const bool synthetic_raw_envelope =
        range.section_index < elf.sections.size() && elf.sections[range.section_index].name == ".raw_boot";
    if (!synthetic_raw_envelope) {
        for (std::uint32_t address = range.start; address + 1 < range.end; address += 2) {
            if (code.contains(address) || address_in_literal(literal_spans, address)) {
                continue;
            }
            result.padding_words.push_back(address);
        }
    }

    std::sort(result.literals.begin(), result.literals.end(), [](const auto& a, const auto& b) {
        if (a.storage_address != b.storage_address) return a.storage_address < b.storage_address;
        return a.instruction_address < b.instruction_address;
    });
    std::sort(result.calls.begin(), result.calls.end(), [](const auto& a, const auto& b) {
        return a.instruction_address < b.instruction_address;
    });

    return result;
}

FunctionAnalysis analyze_function(const Elf32Image& elf, std::string_view function_name) {
    const auto range = function_range(elf, function_name);
    if (!range) throw std::runtime_error("No se encontro la funcion ELF: " + std::string(function_name));
    return analyze_function_with_range(elf, *range, function_name);
}

static std::optional<FunctionRange> containing_function_range(const Elf32Image& elf, std::uint32_t address) {
    const ElfSymbol* selected = nullptr;
    std::uint32_t selected_end = 0u;
    for (const auto& symbol : elf.symbols) {
        if (!symbol.is_function() || symbol.value == 0u || symbol.size == 0u || symbol.section_index >= elf.sections.size()) continue;
        const auto& section = elf.sections[symbol.section_index];
        if ((section.flags & 0x4u) == 0u) continue;
        const std::uint64_t begin = symbol.value & ~1u;
        const std::uint64_t end64 = std::min<std::uint64_t>(begin + symbol.size, static_cast<std::uint64_t>(section.address) + section.size);
        if (address < begin || address >= end64) continue;
        if (!selected || symbol.size < selected->size) {
            selected = &symbol;
            selected_end = static_cast<std::uint32_t>(end64);
        }
    }
    if (!selected) return std::nullopt;
    return FunctionRange{selected, selected->section_index, selected->value & ~1u, selected_end & ~1u};
}

FunctionAnalysis analyze_function_at(const Elf32Image& elf, std::uint32_t function_address) {
    const auto range = function_range_at(elf, function_address);
    if (!range) {
        std::ostringstream out;
        out << "No se encontro una funcion ELF en 0x" << std::hex << std::uppercase << function_address;
        throw std::runtime_error(out.str());
    }
    const std::string name = range->symbol && !range->symbol->name.empty() ? range->symbol->name : "sub_" + std::to_string(function_address);
    return analyze_function_with_range(elf, *range, name);
}

FunctionAnalysis analyze_code_fragment_at(const Elf32Image& elf, std::uint32_t entry_address) {
    const auto range = containing_function_range(elf, entry_address);
    if (!range) {
        std::ostringstream out;
        out << "No se encontro una funcion contenedora para el fragmento 0x" << std::hex << std::uppercase << entry_address;
        throw std::runtime_error(out.str());
    }
    std::ostringstream name;
    name << (range->symbol && !range->symbol->name.empty() ? range->symbol->name : "fragment")
         << "+0x" << std::hex << std::uppercase << (entry_address - range->start);
    return analyze_function_with_range(elf, *range, name.str(), entry_address);
}

} // namespace dcrecomp
