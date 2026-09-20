#include "dcrecomp/elf32.hpp"
#include "dcrecomp/cpp_emitter.hpp"
#include "dcrecomp/program_analysis.hpp"
#include "dcrecomp/sh4_decoder.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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
std::string read_all(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: fpu_unary_tests <sh4_fpu_unary.elf>\n"; return 2; }
        using O=dcrecomp::sh4::Opcode;
        dec(0xF00D,O::Fsts,"FSTS"); dec(0xF11D,O::Flds,"FLDS"); dec(0xF22D,O::Float,"FLOAT");
        dec(0xF33D,O::Ftrc,"FTRC"); dec(0xF44D,O::Fneg,"FNEG"); dec(0xF55D,O::Fabs,"FABS");
        dec(0xF66D,O::Fsqrt,"FSQRT"); dec(0xF77D,O::Fsrra,"FSRRA"); dec(0xF88D,O::Fldi0,"FLDI0");
        dec(0xF99D,O::Fldi1,"FLDI1"); dec(0xF4AD,O::Fcnvsd,"FCNVSD"); dec(0xF4BD,O::Fcnvds,"FCNVDS");
        dec(0xF4ED,O::Fipr,"FIPR"); dec(0xF1FD,O::Ftrv,"FTRV"); dec(0xF8FD,O::Fsca,"FSCA");
        dec(0xFBFD,O::Frchg,"FRCHG"); dec(0xF3FD,O::Fschg,"FSCHG");

        const auto elf=dcrecomp::load_elf32(argv[1]);
        const auto program=dcrecomp::analyze_reachable_program(elf,"_main");
        std::size_t raw_instruction_count = 0;
        for (const auto& f : program.functions) {
            for (const auto& b : f.ir.blocks) {
                for (const auto& i : b.instructions) {
                    if (i.op == dcrecomp::DCIROp::RawSH4) ++raw_instruction_count;
                }
            }
        }
        require(raw_instruction_count==0,"sample must have RAW_SH4=0");
        require(program.functions.size()==1,"sample should have one function");
        const auto& fn=program.functions.front();
        require(has(fn,dcrecomp::DCIROp::Float),"FLOAT IR");
        require(has(fn,dcrecomp::DCIROp::Ftrc),"FTRC IR");
        require(has(fn,dcrecomp::DCIROp::Fipr),"FIPR IR");
        require(has(fn,dcrecomp::DCIROp::Ftrv),"FTRV IR");
        require(has(fn,dcrecomp::DCIROp::Fsca),"FSCA IR");
        require(has(fn,dcrecomp::DCIROp::Frchg),"FRCHG IR");
        require(has(fn,dcrecomp::DCIROp::Fschg),"FSCHG IR");

        // 0.0.81 regression: FTRV must read the architectural XF bank. When
        // FPSCR.FR=1 the logical XF registers live in the physical FR array;
        // directly reading ctx.xf_bits silently uses the wrong matrix and is
        // catastrophic for retail 3D while screen-space 2D can remain intact.
        const auto emit_dir = std::filesystem::temp_directory_path() / "dcr_fpu_emit_081";
        std::filesystem::remove_all(emit_dir);
        const auto emitted = dcrecomp::emit_cpp_program(
            elf, program, {emit_dir, true, false, false});
        const auto generated = read_all(emitted.program_source);
        require(generated.find("const auto& xfb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.fr_bits : ctx.xf_bits") != std::string::npos,
                "FTRV must resolve the architectural XF bank under FPSCR.FR");
        require(generated.find("auto& frb = ((ctx.fpscr & (1u << 21)) != 0u) ? ctx.xf_bits : ctx.fr_bits") != std::string::npos,
                "FTRV must resolve the architectural FR destination bank under FPSCR.FR");
        require(generated.find("runtime.fpu_ftrv_fr1_ops") != std::string::npos,
                "FTRV FR=1 diagnostic emitted");
        require(generated.find("const double o0 =") != std::string::npos &&
                generated.find("const double o3 =") != std::string::npos,
                "0.0.151 FTRV emits direct double-accumulated dot products");
        require(generated.find("float v[4]") == std::string::npos &&
                generated.find("float m[16]") == std::string::npos,
                "0.0.151 FTRV removes per-instruction temporary vector/matrix arrays");
        require(generated.find("const double sum =") != std::string::npos,
                "0.0.151 FIPR accumulates in double before final float writeback");
        require(generated.find("bool sfc_active = false") != std::string::npos &&
                generated.find("auto sfc_activate") != std::string::npos &&
                generated.find("auto sfc_tick") != std::string::npos,
                "0.0.155 emits a static cross-block superblock cache for vector-heavy FPU code");
        require(generated.find("runtime.fpu_aot_mode == 2u") != std::string::npos &&
                generated.find("runtime.fpu_super_cross_block_keeps") != std::string::npos,
                "0.0.155 emits runtime-selectable region/superblock A/B telemetry");
        require(generated.find("sfc_fr_valid") == std::string::npos &&
                generated.find("sfc_fr_dirty") == std::string::npos,
                "0.0.155 static superblock has no per-lane valid/dirty masks");
        require(generated.find("std::uint32_t rfc_fr") != std::string::npos &&
                generated.find("std::uint32_t rfc_xf0") != std::string::npos,
                "0.0.170 Region+ restores local XF matrix lanes alongside FR locals");
        require(generated.find("std::bit_cast<float>(rfc_xf0)") != std::string::npos,
                "0.0.170 FTRV consumes cached XF locals instead of repeated context reads");
        const auto runtime_hpp = read_all(emitted.runtime_header);
        require(runtime_hpp.find("std::uint32_t fpu_aot_mode{3u}") != std::string::npos &&
                runtime_hpp.find("bool fpu_region_metrics") != std::string::npos,
                "0.0.170 makes Region+ default and exposes optional FPU metrics");
        std::filesystem::remove_all(emit_dir);

        std::cout << "FPU unary/vector tests: PASS\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
