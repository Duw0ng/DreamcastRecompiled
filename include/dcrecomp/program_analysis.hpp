#pragma once

#include "dcrecomp/cfg.hpp"
#include "dcrecomp/dcir.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dcrecomp {

struct ProgramFunction {
    FunctionAnalysis analysis;
    ControlFlowGraph cfg;
    DCIRFunction ir;
};

struct ProgramCallEdge {
    std::uint32_t instruction_address{};
    std::uint32_t caller{};
    std::uint32_t callee{};
    std::string caller_name;
    std::string callee_name;
};

struct ProgramExternalCall {
    std::uint32_t instruction_address{};
    std::uint32_t caller{};
    std::uint32_t target{};
    bool resolved{};
    std::string caller_name;
    std::string symbol;
    std::string reason;
};

struct ProgramAnalysisOptions {
    std::size_t max_functions{4096};
    // Additional callable entry points that should be emitted even when the
    // static root call graph cannot prove the path (e.g. function pointers copied
    // into runtime SDK tables before an indirect JMP/JSR).
    std::vector<std::uint32_t> seed_addresses;
    // Symbols implemented by the host runtime instead of recursively recompiled.
    std::vector<std::string> native_override_symbols{"_printf"};
};

struct ProgramAnalysis {
    std::string root_name;
    std::uint32_t root_address{};
    std::vector<ProgramFunction> functions;
    std::vector<ProgramCallEdge> edges;
    std::vector<ProgramExternalCall> external_calls;

    const ProgramFunction* find_function(std::uint32_t address) const;
};

ProgramAnalysis analyze_reachable_program(const Elf32Image& elf,
                                          std::string_view root_function,
                                          const ProgramAnalysisOptions& options = {});

} // namespace dcrecomp
