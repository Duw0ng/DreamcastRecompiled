#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool c, const char* msg) { if (!c) throw std::runtime_error(msg); }
const dcrecomp::ProgramFunction* by_name(const dcrecomp::ProgramAnalysis& p, const std::string& name) {
    for (const auto& fn : p.functions) if (fn.analysis.name == name) return &fn;
    return nullptr;
}
bool has(const dcrecomp::ProgramFunction& fn, dcrecomp::DCIROp op) {
    for (const auto& b : fn.ir.blocks) for (const auto& i : b.instructions) if (i.op == op) return true;
    return false;
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: address_bus_tests <sh4_addressbus.elf>\n"; return 2; }
        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main");
        require(p.functions.size() == 2, "expected _main + _memTestAddressBus");
        const auto* fn = by_name(p, "_memTestAddressBus");
        require(fn != nullptr, "missing _memTestAddressBus");
        require(fn->analysis.unknown == 0, "unknown SH-4 opcode in address-bus test");
        require(fn->cfg.blocks.size() >= 10, "address-bus CFG should contain multiple/nested-loop blocks");
        require(has(*fn, dcrecomp::DCIROp::Load32Indexed), "missing LOAD32_INDEXED");
        require(has(*fn, dcrecomp::DCIROp::Store32Indexed), "missing STORE32_INDEXED");
        require(has(*fn, dcrecomp::DCIROp::Store32), "missing STORE32");
        require(has(*fn, dcrecomp::DCIROp::TstReg), "missing TST_REG mask test");
        require(has(*fn, dcrecomp::DCIROp::CmpEq), "missing CMP_EQ");
        require(has(*fn, dcrecomp::DCIROp::Shll), "missing SHLL");
        require(has(*fn, dcrecomp::DCIROp::Shll2), "missing SHLL2 index scaling");
        require(has(*fn, dcrecomp::DCIROp::Shlr2), "missing SHLR2 nBytes scaling");
        require(has(*fn, dcrecomp::DCIROp::BranchIfFalse), "missing BF lowering");
        require(has(*fn, dcrecomp::DCIROp::BranchIfTrue), "missing BT lowering");
        for (const auto& unit : p.functions) for (const auto& b : unit.ir.blocks) for (const auto& i : b.instructions)
            require(i.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4 in address-bus test");
        std::cout << "Address-bus/CFG/DCIR tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Address-bus/CFG/DCIR tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
