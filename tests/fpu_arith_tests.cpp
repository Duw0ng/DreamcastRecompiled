#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"
#include "dcrecomp/cpp_emitter.hpp"
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
    std::ostringstream out; out << file.rdbuf(); return out.str();
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) { std::cerr << "usage: fpu_arith_tests <sh4_fpu_arith.elf>\n"; return 2; }
        using O = dcrecomp::sh4::Opcode;
        dec(0xF210, O::Fadd, "FADD decode");
        dec(0xF211, O::Fsub, "FSUB decode");
        dec(0xF212, O::Fmul, "FMUL decode");
        dec(0xF303, O::Fdiv, "FDIV decode");
        dec(0xF214, O::FcmpEq, "FCMP/EQ decode");
        dec(0xF215, O::FcmpGt, "FCMP/GT decode");
        dec(0xF21E, O::Fmac, "FMAC decode");

        const auto elf = dcrecomp::load_elf32(argv[1]);
        const auto p = dcrecomp::analyze_reachable_program(elf, "_main");
        require(p.functions.size() == 1, "expected only _main");
        const auto& fn = p.functions[0];
        require(fn.analysis.unknown == 0, "unknown opcode in FPU arithmetic sample");
        require(has(fn, dcrecomp::DCIROp::Fadd), "FADD lowering missing");
        require(has(fn, dcrecomp::DCIROp::Fsub), "FSUB lowering missing");
        require(has(fn, dcrecomp::DCIROp::Fmul), "FMUL lowering missing");
        require(has(fn, dcrecomp::DCIROp::Fdiv), "FDIV lowering missing");
        require(has(fn, dcrecomp::DCIROp::FcmpEq), "FCMP/EQ lowering missing");
        require(has(fn, dcrecomp::DCIROp::FcmpGt), "FCMP/GT lowering missing");
        require(has(fn, dcrecomp::DCIROp::Fmac), "FMAC lowering missing");
        for (const auto& b : fn.ir.blocks) for (const auto& i : b.instructions)
            require(i.op != dcrecomp::DCIROp::RawSH4, "unexpected RAW_SH4");

        // 0.0.155 keeps the proven 0.0.153 region cache as the A/B baseline.
        // Functions with enough hot regions additionally receive the static
        // superblock path; this small arithmetic sample only needs region mode.
        const auto emit_dir = std::filesystem::temp_directory_path() / "dcr_fpu_cache_emit_0155";
        std::filesystem::remove_all(emit_dir);
        const auto emitted = dcrecomp::emit_cpp_program(elf, p, {emit_dir, true, true, false});
        const auto generated = read_all(emitted.program_source);
        require(generated.find("rfc_fr2") != std::string::npos &&
                generated.find("rfc_fr_bank") != std::string::npos,
                "0.0.155 retains the proven per-region FPU cache baseline");
        require(generated.find("runtime.fpu_block_cache_hits") != std::string::npos &&
                generated.find("++runtime.fpu_block_cache_fallbacks") != std::string::npos,
                "0.0.155 keeps cache coverage telemetry and exact fallback");
        require(generated.find("fc_fr_valid") == std::string::npos &&
                generated.find("fc_fr_dirty") == std::string::npos,
                "0.0.155 removes 0.0.154 per-lane dynamic cache masks from generated code");
        require(generated.find("dc_fpu_double_precision(ctx)") != std::string::npos,
                "0.0.155 keeps the pre-cache FPU path as runtime fallback");
        require(generated.find("std::uint32_t rfc_fr") != std::string::npos,
                "0.0.170 Region+ restores the proven uint32 FR local cache");
        require(generated.find("dc_host_fma(std::bit_cast<float>(rfc_fr0)") != std::string::npos,
                "0.0.170 Region+ keeps host FMA3 on top of the proven cache");
        require(generated.find("runtime.fpu_aot_mode != 3u || runtime.fpu_region_metrics") != std::string::npos,
                "0.0.170 Region+ gates expensive per-region telemetry in production");
        const auto runtime_hpp = read_all(emitted.runtime_header);
        require(runtime_hpp.find("dc_host_fma") != std::string::npos &&
                runtime_hpp.find("DCR_HOST_AVX2") != std::string::npos,
                "0.0.170 runtime exposes portable/AVX2 single-rounding FMAC helper");
        const auto generated_cmake = read_all(emitted.cmake_file);
        require(generated_cmake.find("/arch:AVX2") != std::string::npos &&
                generated_cmake.find("DCR_HOST_AVX2") != std::string::npos,
                "0.0.170 Win64 generated builds expose the host-tuned AVX2/FMA option");
        std::filesystem::remove_all(emit_dir);

        std::cout << "FPU arithmetic/DCIR tests: PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FPU arithmetic/DCIR tests: FAIL: " << e.what() << "\n";
        return 1;
    }
}
