#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"

#include <iostream>
#include <stdexcept>

namespace {
void require(bool c, const char* msg) { if (!c) throw std::runtime_error(msg); }
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: noreturn_flow_tests <sh4_noreturn.elf>\n"; return 2; }
        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto fn = dcrecomp::analyze_function(elf, "_main");
        require(fn.unknown == 0, "post-abort data must not count as unknown SH-4");
        require(fn.instructions.size() == 2, "_main should contain only BSR + delay slot after noreturn pruning");
        require(fn.calls.size() == 1, "expected one call");
        require(fn.calls[0].resolved && fn.calls[0].symbol == "_abort", "_abort target must resolve");
        require(fn.padding_words.size() >= 4, "post-abort words should be classified as unreachable/data padding");
        std::cout << "Noreturn CFG/data classification: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Noreturn CFG/data classification: FAIL: " << e.what() << "\n";
        return 1;
    }
}
