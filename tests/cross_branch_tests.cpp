#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"
#include <cstdint>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto elf = dcrecomp::load_elf32(argv[1]);
    dcrecomp::ProgramAnalysisOptions opts; opts.max_functions = 16;
    const auto p = dcrecomp::analyze_reachable_program(elf, "_main", opts);
    constexpr std::uint32_t kSigned = 0x8C010080u;
    constexpr std::uint32_t kSharedTail = 0x8C010048u;
    if (!p.find_function(kSigned)) { std::cerr << "missing signed\n"; return 3; }
    const auto* frag = p.find_function(kSharedTail);
    if (!frag) { std::cerr << "missing shared-tail fragment\n"; return 4; }
    if (frag->analysis.unknown != 0 || frag->ir.blocks.empty()) return 5;
    bool has_pop_r4 = false;
    for (const auto& b : frag->ir.blocks)
        for (const auto& i : b.instructions)
            if (i.op == dcrecomp::DCIROp::Load32PostInc && i.dst == 4u && i.src == 15u) has_pop_r4 = true;
    if (!has_pop_r4) { std::cerr << "shared epilogue was not lowered\n"; return 6; }
    std::cout << "cross-symbol shared tail discovered at 0x" << std::hex << kSharedTail << "\n";
    return 0;
}
