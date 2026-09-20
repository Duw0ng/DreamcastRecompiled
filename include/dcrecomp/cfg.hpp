#pragma once

#include "dcrecomp/function_analysis.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace dcrecomp {

enum class CFGEdgeKind {
    Fallthrough,
    CallReturn,
    Branch,
    ConditionalTaken,
    ConditionalNotTaken,
};

struct CFGEdge {
    std::uint32_t from{};
    std::uint32_t to{};
    CFGEdgeKind kind{CFGEdgeKind::Fallthrough};
};

struct BasicBlock {
    std::uint32_t start_address{};
    std::uint32_t end_address{}; // exclusive; includes a delay-slot instruction when present
    std::vector<sh4::Instruction> instructions;
};

struct ControlFlowGraph {
    std::string function_name;
    std::uint32_t entry{};
    std::vector<BasicBlock> blocks;
    std::vector<CFGEdge> edges;
};

ControlFlowGraph build_cfg(const FunctionAnalysis& analysis);
const char* to_string(CFGEdgeKind kind);

} // namespace dcrecomp
