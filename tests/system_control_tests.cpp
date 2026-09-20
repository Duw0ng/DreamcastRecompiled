#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"
#include "dcrecomp/sh4_decoder.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

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
        if (argc != 2) { std::cerr << "usage: system_control_tests <sh4_system_control.elf>\n"; return 2; }
        using O = dcrecomp::sh4::Opcode;
        dec(0x0048, O::ClrS, "CLRS decode"); dec(0x0058, O::SetS, "SETS decode");
        dec(0x001B, O::Sleep, "SLEEP decode"); dec(0x0038, O::LdTlb, "LDTLB decode");
        dec(0xC312, O::Trapa, "TRAPA decode"); dec(0x0583, O::Pref, "PREF decode");
        dec(0x05C3, O::MovcaL, "MOVCA.L decode"); dec(0x0593, O::Ocbi, "OCBI decode");
        dec(0x05A3, O::Ocbp, "OCBP decode"); dec(0x05B3, O::Ocbwb, "OCBWB decode");
        dec(0x0012, O::StcGbr, "STC GBR decode"); dec(0x0222, O::StcVbr, "STC VBR decode");
        dec(0x411E, O::LdcGbr, "LDC GBR decode"); dec(0x412E, O::LdcVbr, "LDC VBR decode");
        dec(0x4F13, O::StcLGbr, "STC.L GBR decode"); dec(0x4F17, O::LdcLGbr, "LDC.L GBR decode");
        dec(0x4F02, O::StsLMach, "STS.L MACH decode"); dec(0x4F06, O::LdsLMach, "LDS.L MACH decode");
        dec(0x002B, O::Rte, "RTE decode");

        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto main_program = dcrecomp::analyze_reachable_program(elf, "_main");
        require(main_program.functions.size() == 1, "expected only _main in system sample");
        const auto& fn = main_program.functions[0];
        require(fn.analysis.unknown == 0, "unknown opcode in system/control sample");
        require(has(fn, dcrecomp::DCIROp::LdcGbr) && has(fn, dcrecomp::DCIROp::StcGbr), "GBR transfers missing");
        require(has(fn, dcrecomp::DCIROp::StcLGbr) && has(fn, dcrecomp::DCIROp::LdcLGbr), "stack control transfers missing");
        require(has(fn, dcrecomp::DCIROp::StsLMach) && has(fn, dcrecomp::DCIROp::LdsLMach), "stack system-register transfers missing");
        require(has(fn, dcrecomp::DCIROp::Pref) && has(fn, dcrecomp::DCIROp::MovcaL), "cache/store control lowering missing");
        for (const auto& b : fn.ir.blocks) for (const auto& i : b.instructions) require(i.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4");

        const auto rte_program = dcrecomp::analyze_reachable_program(elf, "_rte_probe");
        require(rte_program.functions.size() == 1, "expected one RTE probe function");
        const auto& rte = rte_program.functions[0];
        std::vector<dcrecomp::DCIROp> ops;
        for (const auto& b : rte.ir.blocks) for (const auto& i : b.instructions) ops.push_back(i.op);
        require(ops.size() == 3, "RTE probe should lower to exactly 3 semantic ops");
        require(ops[0] == dcrecomp::DCIROp::PrepareRte, "RTE must restore/snapshot state before slot");
        require(ops[1] == dcrecomp::DCIROp::MovT, "RTE delay slot must execute after SR restore");
        require(ops[2] == dcrecomp::DCIROp::Rte, "RTE transfer must occur after delay slot");

        std::cout << "System/control + RTE ordering tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "System/control + RTE ordering tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
