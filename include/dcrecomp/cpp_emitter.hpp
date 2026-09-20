#pragma once

#include "dcrecomp/dcir.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dcrecomp {

struct CppEmitOptions {
    std::filesystem::path output_directory;
    bool emit_runtime{true};
    bool emit_build_files{true};
    bool emit_native_runner{true};
};

struct CppEmitResult {
    std::filesystem::path function_header;
    std::filesystem::path function_source;
    std::filesystem::path runtime_header;
    std::filesystem::path runtime_source;
    std::filesystem::path arm7_header;
    std::filesystem::path arm7_source;
    std::filesystem::path image_header;
    std::filesystem::path image_source;
    std::filesystem::path native_header;
    std::filesystem::path native_source;
    std::filesystem::path runner_source;
    std::filesystem::path cmake_file;
    std::filesystem::path compile_test_source;
    std::string cpp_function_name;
    std::string registration_function_name;
    std::string native_runner_target{"dreamcast_hello"};
    std::size_t embedded_bytes{};
    std::size_t zero_fill_bytes{};
    std::uint32_t printf_address{};
};

// 0.0.5/0.0.6-compatible single-function emitter. Kept for regression tests.
CppEmitResult emit_cpp(const Elf32Image& elf,
                       const DCIRFunction& function,
                       const CppEmitOptions& options);

struct EmittedProgramFunction {
    std::string name;
    std::uint32_t address{};
    std::string cpp_function_name;
    std::size_t raw_ops{};
};

struct CppProgramEmitResult {
    std::filesystem::path program_header;
    std::filesystem::path program_source;
    // 0.0.109: large retail closures are emitted as multiple translation units
    // so host compilers can optimize them in parallel. program_source remains
    // the registration/compatibility TU; program_sources contains all TUs.
    std::vector<std::filesystem::path> program_sources;
    std::filesystem::path runtime_header;
    std::filesystem::path runtime_source;
    std::filesystem::path arm7_header;
    std::filesystem::path arm7_source;
    std::filesystem::path image_header;
    std::filesystem::path image_source;
    std::filesystem::path native_header;
    std::filesystem::path native_source;
    std::filesystem::path runner_source;
    std::filesystem::path cmake_file;
    std::filesystem::path compile_test_source;
    std::string registration_function_name{"register_recompiled_program"};
    std::string native_runner_target{"dreamcast_program"};
    std::vector<EmittedProgramFunction> functions;
    std::size_t embedded_bytes{};
    std::size_t zero_fill_bytes{};
    std::uint32_t printf_address{};
};

// 0.0.15 multi-function backend: emits every statically reachable function
// discovered by analyze_reachable_program() into one native C++ program.
CppProgramEmitResult emit_cpp_program(const Elf32Image& elf,
                                      const ProgramAnalysis& program,
                                      const CppEmitOptions& options);

} // namespace dcrecomp
