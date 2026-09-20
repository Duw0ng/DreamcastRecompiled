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
void dec(std::uint16_t raw, dcrecomp::sh4::Opcode expected, const char* msg) {
    require(dcrecomp::sh4::decode(raw, 0x8C010000).opcode == expected, msg);
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: cpu_batch_tests <sh4_cpu_batch.elf>\n"; return 2; }
        using O = dcrecomp::sh4::Opcode;
        dec(0x0008, O::ClrT, "CLRT decode"); dec(0x0018, O::SetT, "SETT decode"); dec(0x0028, O::ClrMac, "CLRMAC decode");
        dec(0x0127, O::MulL, "MUL.L decode"); dec(0x212E, O::MuluW, "MULU.W decode"); dec(0x212F, O::MulsW, "MULS.W decode");
        dec(0x3125, O::DmuluL, "DMULU.L decode"); dec(0x312D, O::DmulsL, "DMULS.L decode");
        dec(0x6418, O::SwapB, "SWAP.B decode"); dec(0x6519, O::SwapW, "SWAP.W decode"); dec(0x261D, O::Xtrct, "XTRCT decode");
        dec(0x403D, O::Shld, "SHLD decode"); dec(0x443C, O::Shad, "SHAD decode"); dec(0x4A1B, O::TasB, "TAS.B decode");
        dec(0x001A, O::StsMacl, "STS MACL decode"); dec(0x005A, O::StsFpul, "STS FPUL decode"); dec(0x096A, O::StsFpscr, "STS FPSCR decode");
        dec(0x410A, O::LdsMach, "LDS MACH decode"); dec(0x405A, O::LdsFpul, "LDS FPUL decode"); dec(0x436A, O::LdsFpscr, "LDS FPSCR decode");

        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main");
        require(p.functions.size() == 1, "expected only _main");
        const auto& fn = p.functions[0];
        require(fn.analysis.unknown == 0, "unknown opcode in CPU batch sample");
        require(has(fn, dcrecomp::DCIROp::MulL) && has(fn, dcrecomp::DCIROp::DmuluL) && has(fn, dcrecomp::DCIROp::DmulsL), "multiply lowering missing");
        require(has(fn, dcrecomp::DCIROp::SwapB) && has(fn, dcrecomp::DCIROp::SwapW) && has(fn, dcrecomp::DCIROp::Xtrct), "swap/xtrct lowering missing");
        require(has(fn, dcrecomp::DCIROp::Shld) && has(fn, dcrecomp::DCIROp::Shad), "dynamic shift lowering missing");
        require(has(fn, dcrecomp::DCIROp::TasB), "TAS.B lowering missing");
        require(has(fn, dcrecomp::DCIROp::StsMacl) && has(fn, dcrecomp::DCIROp::StsFpul) && has(fn, dcrecomp::DCIROp::LdsFpscr), "special-register lowering missing");
        for (const auto& b : fn.ir.blocks) for (const auto& i : b.instructions) require(i.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4");
        std::cout << "CPU batch/DCIR tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "CPU batch/DCIR tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
