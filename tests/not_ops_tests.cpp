#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"
#include "dcrecomp/sh4_decoder.hpp"

#include <iostream>
#include <stdexcept>

namespace {
void require(bool c, const char* msg) { if (!c) throw std::runtime_error(msg); }
bool has(const dcrecomp::ProgramFunction& fn, dcrecomp::DCIROp op) {
    for (const auto& b : fn.ir.blocks) for (const auto& i : b.instructions) if (i.op == op) return true;
    return false;
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: not_ops_tests <sh4_not.elf>\n"; return 2; }
        const auto decoded = dcrecomp::sh4::decode(0x6317, 0x8C010000);
        require(decoded.opcode == dcrecomp::sh4::Opcode::Not, "0x6317 must decode as NOT");
        require(decoded.rn == 3 && decoded.rm == 1, "NOT register fields wrong");
        require(dcrecomp::sh4::to_string(decoded) == "not r1,r3", "NOT disassembly wrong");

        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main");
        require(p.functions.size() == 1, "expected only _main");
        require(p.functions[0].analysis.unknown == 0, "unknown opcode in NOT sample");
        require(has(p.functions[0], dcrecomp::DCIROp::NotReg), "NOT not lowered to DCIR");
        for (const auto& b : p.functions[0].ir.blocks) for (const auto& i : b.instructions)
            require(i.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4 in NOT sample");
        std::cout << "NOT/DCIR tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "NOT/DCIR tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
