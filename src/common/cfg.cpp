#include "dcrecomp/cfg.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace dcrecomp {
namespace {

bool is_control(const sh4::Instruction& i) {
    using sh4::Opcode;
    switch (i.opcode) {
        case Opcode::Rts:
        case Opcode::Rte:
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
            return true;
        default:
            return false;
    }
}

const sh4::Instruction* terminator(const BasicBlock& block) {
    // The actual control transfer precedes its delay slot, so scan backwards.
    for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
        if (is_control(*it)) return &*it;
    }
    return nullptr;
}

} // namespace

const char* to_string(CFGEdgeKind kind) {
    switch (kind) {
        case CFGEdgeKind::Fallthrough: return "fallthrough";
        case CFGEdgeKind::CallReturn: return "call-return";
        case CFGEdgeKind::Branch: return "branch";
        case CFGEdgeKind::ConditionalTaken: return "taken";
        case CFGEdgeKind::ConditionalNotTaken: return "not-taken";
    }
    return "unknown";
}

ControlFlowGraph build_cfg(const FunctionAnalysis& analysis) {
    using sh4::Opcode;

    ControlFlowGraph cfg;
    cfg.function_name = analysis.name;
    cfg.entry = analysis.start_address;
    if (analysis.instructions.empty()) return cfg;

    std::map<std::uint32_t, sh4::Instruction> code;
    for (const auto& i : analysis.instructions) code.emplace(i.address, i);

    auto exists = [&](std::uint32_t address) { return code.contains(address); };
    std::set<std::uint32_t> leaders{analysis.start_address};
    auto dynamic_targets = [&](std::uint32_t instruction_address) -> const std::vector<std::uint32_t>* {
        for (const auto& dynamic : analysis.dynamic_branches)
            if (dynamic.instruction_address == instruction_address) return &dynamic.targets;
        return nullptr;
    };

    for (const auto& [address, i] : code) {
        (void)address;
        switch (i.opcode) {
            case Opcode::Jsr:
            case Opcode::Bsr:
            case Opcode::Bsrf:
                if (exists(i.address + 4)) leaders.insert(i.address + 4);
                break;
            case Opcode::Bt:
            case Opcode::Bf:
                if (exists(i.target)) leaders.insert(i.target);
                if (exists(i.address + 2)) leaders.insert(i.address + 2);
                break;
            case Opcode::BtS:
            case Opcode::BfS:
                if (exists(i.target)) leaders.insert(i.target);
                if (exists(i.address + 4)) leaders.insert(i.address + 4);
                break;
            case Opcode::Bra:
                if (exists(i.target)) leaders.insert(i.target);
                break;
            case Opcode::Braf:
            case Opcode::Jmp:
                if (const auto* targets = dynamic_targets(i.address))
                    for (const auto target : *targets) if (exists(target)) leaders.insert(target);
                break;
            default:
                break;
        }
    }

    BasicBlock current;
    for (const auto& [address, i] : code) {
        if (!current.instructions.empty() && leaders.contains(address)) {
            current.end_address = current.instructions.back().address + 2;
            cfg.blocks.push_back(std::move(current));
            current = BasicBlock{};
        }
        if (current.instructions.empty()) current.start_address = address;
        current.instructions.push_back(i);
    }
    if (!current.instructions.empty()) {
        current.end_address = current.instructions.back().address + 2;
        cfg.blocks.push_back(std::move(current));
    }

    std::map<std::uint32_t, std::size_t> block_by_start;
    for (std::size_t i = 0; i < cfg.blocks.size(); ++i) {
        block_by_start.emplace(cfg.blocks[i].start_address, i);
    }

    auto add_edge = [&](std::uint32_t from, std::uint32_t to, CFGEdgeKind kind) {
        if (!block_by_start.contains(to)) return;
        const auto duplicate = std::any_of(cfg.edges.begin(), cfg.edges.end(), [&](const CFGEdge& e) {
            return e.from == from && e.to == to && e.kind == kind;
        });
        if (!duplicate) cfg.edges.push_back({from, to, kind});
    };

    for (std::size_t bi = 0; bi < cfg.blocks.size(); ++bi) {
        const auto& block = cfg.blocks[bi];
        const auto* term = terminator(block);
        const auto next_block = [&]() -> std::uint32_t {
            return bi + 1 < cfg.blocks.size() ? cfg.blocks[bi + 1].start_address : 0;
        };

        if (!term) {
            if (const auto next = next_block()) add_edge(block.start_address, next, CFGEdgeKind::Fallthrough);
            continue;
        }

        switch (term->opcode) {
            case Opcode::Jsr:
            case Opcode::Bsr:
            case Opcode::Bsrf:
                if (exists(term->address + 4)) {
                    add_edge(block.start_address, term->address + 4, CFGEdgeKind::CallReturn);
                }
                break;
            case Opcode::Bra:
                add_edge(block.start_address, term->target, CFGEdgeKind::Branch);
                break;
            case Opcode::Bt:
            case Opcode::Bf:
                add_edge(block.start_address, term->target, CFGEdgeKind::ConditionalTaken);
                add_edge(block.start_address, term->address + 2, CFGEdgeKind::ConditionalNotTaken);
                break;
            case Opcode::BtS:
            case Opcode::BfS:
                add_edge(block.start_address, term->target, CFGEdgeKind::ConditionalTaken);
                add_edge(block.start_address, term->address + 4, CFGEdgeKind::ConditionalNotTaken);
                break;
            case Opcode::Braf:
            case Opcode::Jmp:
                if (const auto* targets = dynamic_targets(term->address))
                    for (const auto target : *targets) add_edge(block.start_address, target, CFGEdgeKind::Branch);
                break;
            case Opcode::Rts:
            case Opcode::Rte:
                break;
            default:
                if (const auto next = next_block()) add_edge(block.start_address, next, CFGEdgeKind::Fallthrough);
                break;
        }
    }

    return cfg;
}

} // namespace dcrecomp
