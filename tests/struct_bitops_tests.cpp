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
        if (argc != 2) { std::cerr << "usage: struct_bitops_tests <sh4_structs.elf>\n"; return 2; }
        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main");
        require(p.functions.size() == 2, "expected _main + _process_record");
        const auto* fn = by_name(p, "_process_record");
        require(fn != nullptr, "missing _process_record");
        require(fn->analysis.unknown == 0, "unknown SH-4 opcode in struct test");
        require(has(*fn, dcrecomp::DCIROp::Load8Disp), "missing LOAD8_DISP");
        require(has(*fn, dcrecomp::DCIROp::Load16Disp), "missing LOAD16_DISP");
        require(has(*fn, dcrecomp::DCIROp::Load32Disp), "missing LOAD32_DISP");
        require(has(*fn, dcrecomp::DCIROp::Store8Disp), "missing STORE8_DISP");
        require(has(*fn, dcrecomp::DCIROp::Store16Disp), "missing STORE16_DISP");
        require(has(*fn, dcrecomp::DCIROp::Store32Disp), "missing STORE32_DISP");
        require(has(*fn, dcrecomp::DCIROp::Load8Indexed), "missing LOAD8_INDEXED");
        require(has(*fn, dcrecomp::DCIROp::Store8Indexed), "missing STORE8_INDEXED");
        require(has(*fn, dcrecomp::DCIROp::ExtuB) && has(*fn, dcrecomp::DCIROp::ExtuW), "missing unsigned extension");
        require(has(*fn, dcrecomp::DCIROp::ExtsB), "missing signed extension");
        require(has(*fn, dcrecomp::DCIROp::AndImm) && has(*fn, dcrecomp::DCIROp::XorImm) && has(*fn, dcrecomp::DCIROp::OrImm), "missing immediate logical ops");
        require(has(*fn, dcrecomp::DCIROp::AndReg) && has(*fn, dcrecomp::DCIROp::XorReg) && has(*fn, dcrecomp::DCIROp::OrReg), "missing register logical ops");
        require(has(*fn, dcrecomp::DCIROp::Shlr8) && has(*fn, dcrecomp::DCIROp::Shlr16), "missing fixed shifts");
        require(has(*fn, dcrecomp::DCIROp::Rotl) && has(*fn, dcrecomp::DCIROp::Rotr), "missing rotates");
        for (const auto& unit : p.functions) for (const auto& b : unit.ir.blocks) for (const auto& i : b.instructions)
            require(i.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4 in struct test");
        std::cout << "Struct/bitops/DCIR tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Struct/bitops/DCIR tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
