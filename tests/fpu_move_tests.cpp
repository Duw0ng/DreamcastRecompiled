#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"
#include "dcrecomp/sh4_decoder.hpp"

#include <iostream>
#include <stdexcept>

namespace {
void require(bool c, const char* msg) { if (!c) throw std::runtime_error(msg); }
void dec(std::uint16_t raw, dcrecomp::sh4::Opcode expected, const char* msg) {
    require(dcrecomp::sh4::decode(raw, 0x8C010000).opcode == expected, msg);
}
bool has(const dcrecomp::ProgramFunction& fn, dcrecomp::DCIROp op) {
    for (const auto& b : fn.ir.blocks) for (const auto& i : b.instructions) if (i.op == op) return true;
    return false;
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: fpu_move_tests <sh4_fpu_moves.elf>\n"; return 2; }
        using O = dcrecomp::sh4::Opcode;
        dec(0xF148, O::FmovLoad, "FMOV.S @Rm,FRn decode");
        dec(0xF359, O::FmovLoadPostInc, "FMOV.S @Rm+,FRn decode");
        dec(0xF62A, O::FmovStore, "FMOV.S FRm,@Rn decode");
        dec(0xF73B, O::FmovStorePreDec, "FMOV.S FRm,@-Rn decode");
        dec(0xF21C, O::FmovReg, "FMOV FRm,FRn decode");
        dec(0xF446, O::FmovIndexedLoad, "FMOV.S @(R0,Rm),FRn decode");
        dec(0xF847, O::FmovIndexedStore, "FMOV.S FRm,@(R0,Rn) decode");

        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main");
        require(p.functions.size() == 1, "expected only _main");
        const auto& fn = p.functions[0];
        require(fn.analysis.unknown == 0, "unknown opcode in FPU move sample");
        require(has(fn, dcrecomp::DCIROp::FmovLoad), "FMOV load lowering missing");
        require(has(fn, dcrecomp::DCIROp::FmovLoadPostInc), "FMOV postinc lowering missing");
        require(has(fn, dcrecomp::DCIROp::FmovStore), "FMOV store lowering missing");
        require(has(fn, dcrecomp::DCIROp::FmovStorePreDec), "FMOV predec lowering missing");
        require(has(fn, dcrecomp::DCIROp::FmovReg), "FMOV reg lowering missing");
        require(has(fn, dcrecomp::DCIROp::FmovIndexedLoad), "FMOV indexed load lowering missing");
        require(has(fn, dcrecomp::DCIROp::FmovIndexedStore), "FMOV indexed store lowering missing");
        for (const auto& b : fn.ir.blocks) for (const auto& i : b.instructions)
            require(i.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4");

        std::cout << "FPU move/DCIR tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FPU move/DCIR tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
