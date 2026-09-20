#include "dcrecomp/program_analysis.hpp"

#include <algorithm>
#include <deque>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace dcrecomp {
namespace {

constexpr std::uint32_t SHF_EXECINSTR = 0x4u;

bool is_callable_code_symbol(const Elf32Image& elf, const ElfSymbol& symbol) {
    if (symbol.value == 0 || symbol.section_index >= elf.sections.size()) return false;
    const auto& section = elf.sections[symbol.section_index];
    const bool executable = (section.flags & SHF_EXECINSTR) != 0u &&
                            symbol.value >= section.address && symbol.value < section.address + section.size;
    if (!executable) return false;
    return symbol.is_function() || (symbol.type() == 0u && symbol.binding() != 0u);
}

const ElfSymbol* function_symbol_at(const Elf32Image& elf, std::uint32_t address) {
    // Exact symbol addresses win. If the call is through SH-4 P2 (uncached)
    // while the ELF/raw image is represented through P1, accept the callable
    // symbol with the same 29-bit physical address. Katana startup/cache code
    // deliberately uses this aliasing pattern.
    for (const auto& symbol : elf.symbols) {
        if (symbol.value == address && is_callable_code_symbol(elf, symbol) &&
            !symbol.name.empty() && symbol.name != "<invalid>") {
            return &symbol;
        }
    }
    const std::uint32_t physical = address & 0x1FFFFFFFu;
    for (const auto& symbol : elf.symbols) {
        if ((symbol.value & 0x1FFFFFFFu) == physical && is_callable_code_symbol(elf, symbol) &&
            !symbol.name.empty() && symbol.name != "<invalid>") {
            return &symbol;
        }
    }
    return nullptr;
}

bool is_native_override(const ProgramAnalysisOptions& options, const std::string& symbol) {
    return std::find(options.native_override_symbols.begin(), options.native_override_symbols.end(), symbol) !=
           options.native_override_symbols.end();
}

bool has_containing_function(const Elf32Image& elf, std::uint32_t address) {
    for (const auto& symbol : elf.symbols) {
        if (!symbol.is_function() || symbol.value == 0u || symbol.size == 0u || symbol.section_index >= elf.sections.size()) continue;
        const auto& section = elf.sections[symbol.section_index];
        if ((section.flags & SHF_EXECINSTR) == 0u) continue;
        const std::uint64_t begin = symbol.value & ~1u;
        const std::uint64_t end = std::min<std::uint64_t>(begin + symbol.size, static_cast<std::uint64_t>(section.address) + section.size);
        if (address >= begin && address < end) return true;
    }
    return false;
}

bool is_direct_branch(sh4::Opcode opcode) {
    using sh4::Opcode;
    return opcode == Opcode::Bra || opcode == Opcode::Bt || opcode == Opcode::Bf ||
           opcode == Opcode::BtS || opcode == Opcode::BfS;
}

} // namespace

const ProgramFunction* ProgramAnalysis::find_function(std::uint32_t address) const {
    for (const auto& fn : functions) {
        if (fn.analysis.start_address == address) return &fn;
    }
    return nullptr;
}

ProgramAnalysis analyze_reachable_program(const Elf32Image& elf,
                                          std::string_view root_function,
                                          const ProgramAnalysisOptions& options) {
    if (options.max_functions == 0) {
        throw std::runtime_error("max_functions debe ser mayor que cero");
    }

    ProgramAnalysis program;
    program.root_name = std::string(root_function);

    std::deque<std::uint32_t> pending;
    std::unordered_set<std::uint32_t> visited;
    std::set<std::pair<std::uint32_t, std::uint32_t>> edge_keys;
    const auto root_analysis = analyze_function(elf, program.root_name);
    pending.push_back(root_analysis.start_address);
    for (const auto requested : options.seed_addresses) {
        if (const auto* symbol = function_symbol_at(elf, requested)) {
            const std::uint32_t canonical = symbol->value & ~1u;
            if (canonical != root_analysis.start_address) pending.push_back(canonical);
        }
    }

    while (!pending.empty()) {
        const std::uint32_t pending_address = pending.front();
        pending.pop_front();

        const auto analysis = (pending_address == root_analysis.start_address)
            ? root_analysis
            : (function_symbol_at(elf, pending_address)
                ? analyze_function_at(elf, pending_address)
                : analyze_code_fragment_at(elf, pending_address));
        if (visited.contains(analysis.start_address)) continue;
        if (program.functions.size() >= options.max_functions) {
            throw std::runtime_error("Se alcanzo el limite de funciones alcanzables (--max-functions)");
        }
        visited.insert(analysis.start_address);
        if (program.functions.empty()) program.root_address = analysis.start_address;

        ProgramFunction unit;
        unit.analysis = analysis;
        unit.cfg = build_cfg(unit.analysis);
        unit.ir = lower_to_dcir(elf, unit.analysis, unit.cfg);

        for (const auto& call : unit.analysis.calls) {
            if (!call.resolved || call.target == 0) {
                program.external_calls.push_back({call.instruction_address,
                                                  unit.analysis.start_address,
                                                  0,
                                                  false,
                                                  unit.analysis.name,
                                                  {},
                                                  "unresolved call target"});
                continue;
            }

            const auto* symbol = function_symbol_at(elf, call.target);
            if (!symbol) {
                program.external_calls.push_back({call.instruction_address,
                                                  unit.analysis.start_address,
                                                  call.target,
                                                  true,
                                                  unit.analysis.name,
                                                  call.symbol,
                                                  "target has no callable code symbol"});
                continue;
            }

            if (is_native_override(options, symbol->name)) {
                program.external_calls.push_back({call.instruction_address,
                                                  unit.analysis.start_address,
                                                  call.target,
                                                  true,
                                                  unit.analysis.name,
                                                  symbol->name,
                                                  "native override"});
                continue;
            }

            const std::uint32_t callee_address = symbol->value & ~1u;
            if (edge_keys.insert({unit.analysis.start_address, callee_address}).second) {
                program.edges.push_back({call.instruction_address,
                                         unit.analysis.start_address,
                                         callee_address,
                                         unit.analysis.name,
                                         symbol->name});
            }
            if (!visited.contains(callee_address)) pending.push_back(callee_address);
        }


        // Function pointers frequently live in literal pools and are later invoked
        // through JSR/JMP @Rn after being stored in FILE/device/callback structures.
        // A purely direct-call reachability walk misses those address-taken targets
        // (newlib ___sread, KOS assert handlers, callbacks, etc.). If a 32-bit literal
        // exactly names a callable ELF function, emit it as an indirect target too.
        for (const auto& literal : unit.analysis.literals) {
            const auto* symbol = function_symbol_at(elf, literal.value);
            // Address-taken closure is intentionally stricter than direct-call
            // resolution: zero-sized global NOTYPE labels in .text are often
            // linker markers/data anchors, not callable entry points.
            if (!symbol || !symbol->is_function() || is_native_override(options, symbol->name)) continue;
            const std::uint32_t callee_address = symbol->value & ~1u;
            if (!visited.contains(callee_address)) pending.push_back(callee_address);
        }


        // GCC/libgcc may share implementation tails between adjacent ELF function
        // symbols. A direct branch can therefore legally leave the current st_size
        // and enter the middle of another function (for example sdiv -> udiv).
        // Treat those addresses as executable fragments instead of silently returning
        // to the host with an unbalanced guest stack.
        for (const auto& insn : unit.analysis.instructions) {
            if (!is_direct_branch(insn.opcode) || insn.target == 0u) continue;
            if (insn.target >= unit.analysis.start_address && insn.target < unit.analysis.end_address) continue;
            if (!has_containing_function(elf, insn.target)) continue;
            if (!visited.contains(insn.target)) pending.push_back(insn.target);
        }

        program.functions.push_back(std::move(unit));
    }

    std::sort(program.functions.begin(), program.functions.end(), [](const auto& a, const auto& b) {
        return a.analysis.start_address < b.analysis.start_address;
    });
    std::sort(program.edges.begin(), program.edges.end(), [](const auto& a, const auto& b) {
        if (a.caller != b.caller) return a.caller < b.caller;
        if (a.instruction_address != b.instruction_address) return a.instruction_address < b.instruction_address;
        return a.callee < b.callee;
    });
    return program;
}

} // namespace dcrecomp
