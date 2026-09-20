#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

const dcrecomp::ProgramFunction* by_name(const dcrecomp::ProgramAnalysis& p, const std::string& name) {
    for (const auto& fn : p.functions) if (fn.analysis.name == name) return &fn;
    return nullptr;
}

bool has_op(const dcrecomp::ProgramFunction& fn, dcrecomp::DCIROp op) {
    for (const auto& block : fn.ir.blocks) {
        for (const auto& insn : block.instructions) {
            if (insn.op == op) return true;
        }
    }
    return false;
}

std::size_t edge_count(const dcrecomp::ProgramFunction& fn, dcrecomp::CFGEdgeKind kind) {
    std::size_t count = 0;
    for (const auto& edge : fn.cfg.edges) if (edge.kind == kind) ++count;
    return count;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: branch_control_tests <sh4_branches.elf>\n";
            return 2;
        }

        const auto elf = dcrecomp::load_elf32(argv[1]);
        dcrecomp::ProgramAnalysisOptions opts;
        opts.native_override_symbols = {"_printf"};
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main", opts);

        require(p.functions.size() == 2, "expected _main and _branch_loop");
        require(p.edges.size() == 1, "expected one call-graph edge");
        require(p.external_calls.empty(), "branch test should have no external calls");

        const auto* main_fn = by_name(p, "_main");
        const auto* loop_fn = by_name(p, "_branch_loop");
        require(main_fn != nullptr, "missing _main");
        require(loop_fn != nullptr, "missing _branch_loop");

        require(loop_fn->analysis.unknown == 0, "branch loop has unknown SH-4 opcodes");
        require(loop_fn->cfg.blocks.size() == 6, "expected six basic blocks in branch loop");
        require(edge_count(*loop_fn, dcrecomp::CFGEdgeKind::ConditionalTaken) >= 2,
                "expected conditional taken edges");
        require(edge_count(*loop_fn, dcrecomp::CFGEdgeKind::ConditionalNotTaken) >= 2,
                "expected conditional not-taken edges");
        require(edge_count(*loop_fn, dcrecomp::CFGEdgeKind::Branch) >= 1,
                "expected unconditional branch edge");

        require(has_op(*loop_fn, dcrecomp::DCIROp::Dt), "DT was not lowered to DCIR");
        require(has_op(*loop_fn, dcrecomp::DCIROp::CmpEqImm), "CMP/EQ #imm was not lowered to DCIR");
        require(has_op(*loop_fn, dcrecomp::DCIROp::BranchIfFalse), "BF was not lowered to DCIR");
        require(has_op(*loop_fn, dcrecomp::DCIROp::BranchIfTrue), "BT was not lowered to DCIR");
        require(has_op(*loop_fn, dcrecomp::DCIROp::Branch), "BRA was not lowered to DCIR");

        for (const auto& fn : p.functions) {
            for (const auto& block : fn.ir.blocks) {
                for (const auto& insn : block.instructions) {
                    require(insn.op != dcrecomp::DCIROp::RawSH4,
                            "unexpected RAW_SH4 in control-flow synthetic ELF");
                }
            }
        }

        std::cout << "Branch/control-flow tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Branch/control-flow tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
