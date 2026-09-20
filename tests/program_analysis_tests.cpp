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

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: program_analysis_tests <sh4_multifunc.elf>\n";
            return 2;
        }
        const auto elf = dcrecomp::load_elf32(argv[1]);
        dcrecomp::ProgramAnalysisOptions opts;
        opts.native_override_symbols = {"_printf"};
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main", opts);

        require(p.root_address == 0x8C010000u, "wrong root address");
        require(p.functions.size() == 3, "expected three reachable functions");
        require(p.edges.size() == 2, "expected two call-graph edges");
        require(p.external_calls.empty(), "synthetic graph should have no external calls");
        require(by_name(p, "_main") != nullptr, "missing _main");
        require(by_name(p, "_helper") != nullptr, "missing _helper");
        require(by_name(p, "_leaf") != nullptr, "missing _leaf");

        bool main_helper = false;
        bool helper_leaf = false;
        for (const auto& e : p.edges) {
            if (e.caller_name == "_main" && e.callee_name == "_helper") main_helper = true;
            if (e.caller_name == "_helper" && e.callee_name == "_leaf") helper_leaf = true;
        }
        require(main_helper, "missing _main -> _helper edge");
        require(helper_leaf, "missing _helper -> _leaf edge");

        for (const auto& fn : p.functions) {
            for (const auto& block : fn.ir.blocks) {
                for (const auto& insn : block.instructions) {
                    require(insn.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4 in synthetic multi-function ELF");
                }
            }
        }

        std::cout << "Program analysis tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Program analysis tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
