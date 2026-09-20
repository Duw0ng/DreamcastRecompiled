#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"
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

bool has_literal_symbol(const dcrecomp::ProgramFunction& fn, const std::string& symbol) {
    for (const auto& literal : fn.analysis.literals) {
        if (literal.symbol == symbol) return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: memory_ops_tests <sh4_memory.elf>\n";
            return 2;
        }

        const auto elf = dcrecomp::load_elf32(argv[1]);
        dcrecomp::ProgramAnalysisOptions opts;
        opts.native_override_symbols = {"_printf"};
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main", opts);

        require(p.functions.size() == 2, "expected _main and _sum_array");
        require(p.edges.size() == 1, "expected one call-graph edge");
        require(p.external_calls.empty(), "memory test should have no external calls");

        const auto* main_fn = by_name(p, "_main");
        const auto* sum_fn = by_name(p, "_sum_array");
        require(main_fn != nullptr, "missing _main");
        require(sum_fn != nullptr, "missing _sum_array");

        require(main_fn->analysis.unknown == 0, "_main has unknown SH-4 opcodes");
        require(sum_fn->analysis.unknown == 0, "_sum_array has unknown SH-4 opcodes");

        require(has_literal_symbol(*main_fn, "_values"), "_values literal was not resolved");
        require(has_literal_symbol(*main_fn, "_scratch"), "_scratch literal was not resolved");
        require(has_op(*main_fn, dcrecomp::DCIROp::Store32), "MOV.L store was not lowered");
        require(has_op(*main_fn, dcrecomp::DCIROp::Load32), "MOV.L load was not lowered");
        require(has_op(*sum_fn, dcrecomp::DCIROp::Load32PostInc), "MOV.L post-increment was not lowered");
        require(has_op(*sum_fn, dcrecomp::DCIROp::AddReg), "ADD was not lowered");
        require(has_op(*sum_fn, dcrecomp::DCIROp::Dt), "DT was not lowered");
        require(has_op(*sum_fn, dcrecomp::DCIROp::BranchIfFalse), "BF was not lowered");

        for (const auto& fn : p.functions) {
            for (const auto& block : fn.ir.blocks) {
                for (const auto& insn : block.instructions) {
                    require(insn.op != dcrecomp::DCIROp::RawSH4,
                            "unexpected RAW_SH4 in memory synthetic ELF");
                }
            }
        }

        const auto* values = dcrecomp::find_symbol_at(elf, 0x8C020000u);
        const auto* scratch = dcrecomp::find_symbol_at(elf, 0x8C030000u);
        require(values && values->name == "_values", "missing _values object symbol");
        require(scratch && scratch->name == "_scratch", "missing _scratch object symbol");

        std::cout << "Memory/DCIR tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Memory/DCIR tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
