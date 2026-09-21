cmake_minimum_required(VERSION 3.20)
project(DreamcastRecomp VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dcrecomp_core
    src/common/elf32.cpp
    src/common/function_analysis.cpp
    src/common/cfg.cpp
    src/common/program_analysis.cpp
    src/common/corpus_scan.cpp
    src/common/raw_data.cpp
    src/ir/dcir.cpp
    src/codegen/cpp_emitter.cpp
    src/sh4/decoder.cpp
    src/aica/arm7.cpp
)

target_include_directories(dcrecomp_core PUBLIC include)

if(MSVC)
    target_compile_options(dcrecomp_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(dcrecomp_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(dc_analyzer src/analyzer/main.cpp)
target_link_libraries(dc_analyzer PRIVATE dcrecomp_core)

add_executable(dc_recomp src/recompiler/main.cpp)
target_link_libraries(dc_recomp PRIVATE dcrecomp_core)

add_executable(dc_corpus_scan src/corpus/main.cpp)
target_link_libraries(dc_corpus_scan PRIVATE dcrecomp_core)

enable_testing()

add_executable(arm7_tests tests/arm7_tests.cpp)
target_link_libraries(arm7_tests PRIVATE dcrecomp_core)
add_test(NAME arm7_tests COMMAND arm7_tests)
add_executable(sh4_decoder_tests tests/sh4_decoder_tests.cpp)
target_link_libraries(sh4_decoder_tests PRIVATE dcrecomp_core)
add_test(NAME sh4_decoder_tests COMMAND sh4_decoder_tests)

add_executable(raw_data_tests tests/raw_data_tests.cpp)
target_link_libraries(raw_data_tests PRIVATE dcrecomp_core)
add_test(NAME raw_data_tests COMMAND raw_data_tests)

add_test(
    NAME dc_analyzer_smoke
    COMMAND dc_analyzer
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_smoke.elf
        --disassemble --from-entry --max-instructions 13 --stats --functions
)

add_executable(function_analysis_tests tests/function_analysis_tests.cpp)
target_link_libraries(function_analysis_tests PRIVATE dcrecomp_core)
add_test(
    NAME function_analysis_tests
    COMMAND function_analysis_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_literal_pool.elf
)

add_test(
    NAME dc_analyzer_literal_pool
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_literal_pool.elf --analyze-function _main
)
set_tests_properties(dc_analyzer_literal_pool PROPERTIES
    PASS_REGULAR_EXPRESSION "Coverage:     100.00%"
)

add_executable(cfg_dcir_tests tests/cfg_dcir_tests.cpp)
target_link_libraries(cfg_dcir_tests PRIVATE dcrecomp_core)
add_test(
    NAME cfg_dcir_tests
    COMMAND cfg_dcir_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_literal_pool.elf
)

add_test(
    NAME dc_analyzer_dcir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_literal_pool.elf --emit-ir _main
)
set_tests_properties(dc_analyzer_dcir PROPERTIES
    PASS_REGULAR_EXPRESSION "DYNAMIC_CALL"
)


add_executable(cpp_emitter_tests tests/cpp_emitter_tests.cpp)
target_link_libraries(cpp_emitter_tests PRIVATE dcrecomp_core)
set(GENERATED_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp_test)
add_test(
    NAME cpp_emitter_tests
    COMMAND cpp_emitter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_literal_pool.elf
        ${GENERATED_TEST_DIR}
)

add_test(
    NAME dc_recomp_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_literal_pool.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_output
)
set_tests_properties(dc_recomp_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "Todas las funciones alcanzables fueron emitidas sin RAW_SH4"
)

add_executable(program_analysis_tests tests/program_analysis_tests.cpp)
target_link_libraries(program_analysis_tests PRIVATE dcrecomp_core)
add_test(
    NAME program_analysis_tests
    COMMAND program_analysis_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_multifunc.elf
)

add_test(
    NAME dc_recomp_multifunc_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_multifunc.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_multifunc_output
)
set_tests_properties(dc_recomp_multifunc_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "Reachable functions:  3"
)


add_executable(branch_control_tests tests/branch_control_tests.cpp)
target_link_libraries(branch_control_tests PRIVATE dcrecomp_core)
add_test(
    NAME branch_control_tests
    COMMAND branch_control_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_branches.elf
)

add_test(
    NAME dc_recomp_branches_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_branches.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_branches_output
)
set_tests_properties(dc_recomp_branches_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "Reachable functions:  2"
)


add_executable(memory_ops_tests tests/memory_ops_tests.cpp)
target_link_libraries(memory_ops_tests PRIVATE dcrecomp_core)
add_test(
    NAME memory_ops_tests
    COMMAND memory_ops_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_memory.elf
)

add_test(
    NAME dc_analyzer_memory_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_memory.elf --emit-ir _sum_array
)
set_tests_properties(dc_analyzer_memory_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "LOAD32_POSTINC"
)

add_test(
    NAME dc_recomp_memory_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_memory.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_memory_output
)
set_tests_properties(dc_recomp_memory_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "Reachable functions:  2"
)

add_executable(struct_bitops_tests tests/struct_bitops_tests.cpp)
target_link_libraries(struct_bitops_tests PRIVATE dcrecomp_core)
add_test(
    NAME struct_bitops_tests
    COMMAND struct_bitops_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_structs.elf
)

add_test(
    NAME dc_analyzer_struct_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_structs.elf --emit-ir _process_record
)
set_tests_properties(dc_analyzer_struct_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "LOAD8_DISP"
)

add_test(
    NAME dc_recomp_struct_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_structs.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_struct_output
)
set_tests_properties(dc_recomp_struct_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "Reachable functions:  2"
)

add_executable(address_bus_tests tests/address_bus_tests.cpp)
target_link_libraries(address_bus_tests PRIVATE dcrecomp_core)
add_test(
    NAME address_bus_tests
    COMMAND address_bus_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_addressbus.elf
)

add_test(
    NAME dc_analyzer_addressbus_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_addressbus.elf --emit-ir _memTestAddressBus
)
set_tests_properties(dc_analyzer_addressbus_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "STORE32_INDEXED"
)

add_test(
    NAME dc_recomp_addressbus_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_addressbus.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_addressbus_output
)
set_tests_properties(dc_recomp_addressbus_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "Reachable functions:  2"
)

add_executable(not_ops_tests tests/not_ops_tests.cpp)
target_link_libraries(not_ops_tests PRIVATE dcrecomp_core)
add_test(
    NAME not_ops_tests
    COMMAND not_ops_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_not.elf
)

add_test(
    NAME dc_analyzer_not_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_not.elf --emit-ir _main
)
set_tests_properties(dc_analyzer_not_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "NOT_REG"
)

add_test(
    NAME dc_recomp_not_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_not.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_not_output
)
set_tests_properties(dc_recomp_not_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "RAW_SH4:              0"
)

# Keep the corpus scanner regression isolated from user/homebrew ELFs copied
# into samples/. The public sample directory is intentionally extensible, so
# testing it directly made this test fail as soon as a user added 2ndMix,
# Raytris, pvrmark, or any other valid ELF.
set(DC_CORPUS_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/corpus_scan_samples_input")
file(REMOVE_RECURSE "${DC_CORPUS_TEST_DIR}")
file(MAKE_DIRECTORY "${DC_CORPUS_TEST_DIR}")
set(DC_CORPUS_TEST_SAMPLES
    sh4_addressbus.elf
    sh4_branches.elf
    sh4_cpu_batch.elf
    sh4_cpu_control.elf
    sh4_cross_branch.elf
    sh4_fpu_arith.elf
    sh4_fpu_moves.elf
    sh4_fpu_unary.elf
    sh4_literal_pool.elf
    sh4_memory.elf
    sh4_multifunc.elf
    sh4_noreturn.elf
    sh4_not.elf
    sh4_smoke.elf
    sh4_structs.elf
    sh4_system_control.elf
)
foreach(sample IN LISTS DC_CORPUS_TEST_SAMPLES)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/samples/${sample}"
        "${DC_CORPUS_TEST_DIR}/${sample}"
        COPYONLY
    )
endforeach()

add_test(
    NAME dc_corpus_scan_samples
    COMMAND dc_corpus_scan
        ${DC_CORPUS_TEST_DIR}
        --no-main
        --top 10
        --text ${CMAKE_CURRENT_BINARY_DIR}/corpus_samples.txt
        --csv ${CMAKE_CURRENT_BINARY_DIR}/corpus_samples.csv
)
set_tests_properties(dc_corpus_scan_samples PROPERTIES
    PASS_REGULAR_EXPRESSION "ELF ISA-clean.*16"
)

add_executable(cpu_batch_tests tests/cpu_batch_tests.cpp)
target_link_libraries(cpu_batch_tests PRIVATE dcrecomp_core)
add_test(
    NAME cpu_batch_tests
    COMMAND cpu_batch_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cpu_batch.elf
)

add_test(
    NAME dc_analyzer_cpu_batch_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cpu_batch.elf --emit-ir _main
)
set_tests_properties(dc_analyzer_cpu_batch_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "SHLD"
)

add_test(
    NAME dc_recomp_cpu_batch_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cpu_batch.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_cpu_batch_output
)
set_tests_properties(dc_recomp_cpu_batch_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "RAW_SH4:              0"
)

add_executable(cpu_control_tests tests/cpu_control_tests.cpp)
target_link_libraries(cpu_control_tests PRIVATE dcrecomp_core)
add_test(
    NAME cpu_control_tests
    COMMAND cpu_control_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cpu_control.elf
)

add_test(
    NAME dc_analyzer_cpu_control_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cpu_control.elf --emit-ir _helper
)
set_tests_properties(dc_analyzer_cpu_control_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "DIV1"
)

add_test(
    NAME dc_recomp_cpu_control_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cpu_control.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_cpu_control_output
)
set_tests_properties(dc_recomp_cpu_control_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "RAW_SH4:              0"
)


add_executable(fpu_move_tests tests/fpu_move_tests.cpp)
target_link_libraries(fpu_move_tests PRIVATE dcrecomp_core)
add_test(
    NAME fpu_move_tests
    COMMAND fpu_move_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_moves.elf
)

add_test(
    NAME dc_analyzer_fpu_moves_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_moves.elf --emit-ir _main
)
set_tests_properties(dc_analyzer_fpu_moves_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "FMOV_INDEXED_STORE"
)

add_test(
    NAME dc_recomp_fpu_moves_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_moves.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_fpu_moves_output
)
set_tests_properties(dc_recomp_fpu_moves_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "RAW_SH4:              0"
)

add_executable(fpu_arith_tests tests/fpu_arith_tests.cpp)
target_link_libraries(fpu_arith_tests PRIVATE dcrecomp_core)
add_test(
    NAME fpu_arith_tests
    COMMAND fpu_arith_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_arith.elf
)

add_test(
    NAME dc_analyzer_fpu_arith_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_arith.elf --emit-ir _main
)
set_tests_properties(dc_analyzer_fpu_arith_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "FMAC"
)

add_test(
    NAME dc_recomp_fpu_arith_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_arith.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_fpu_arith_output
)
set_tests_properties(dc_recomp_fpu_arith_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "RAW_SH4:              0"
)

add_executable(fpu_unary_tests tests/fpu_unary_tests.cpp)
target_link_libraries(fpu_unary_tests PRIVATE dcrecomp_core)
add_test(
    NAME fpu_unary_tests
    COMMAND fpu_unary_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_unary.elf
)

add_test(
    NAME dc_analyzer_fpu_unary_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_unary.elf --emit-ir _main
)
set_tests_properties(dc_analyzer_fpu_unary_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "FTRV"
)

add_test(
    NAME dc_recomp_fpu_unary_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_fpu_unary.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_fpu_unary_output
)
set_tests_properties(dc_recomp_fpu_unary_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "RAW_SH4:              0"
)

add_executable(system_control_tests tests/system_control_tests.cpp)
target_link_libraries(system_control_tests PRIVATE dcrecomp_core)
add_test(
    NAME system_control_tests
    COMMAND system_control_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_system_control.elf
)

add_test(
    NAME dc_analyzer_system_control_ir
    COMMAND dc_analyzer ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_system_control.elf --emit-ir _main
)
set_tests_properties(dc_analyzer_system_control_ir PROPERTIES
    PASS_REGULAR_EXPRESSION "MOVCA_L"
)

add_test(
    NAME dc_recomp_system_control_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_system_control.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_system_control_output
)
set_tests_properties(dc_recomp_system_control_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "RAW_SH4:              0"
)

add_executable(noreturn_flow_tests tests/noreturn_flow_tests.cpp)
target_link_libraries(noreturn_flow_tests PRIVATE dcrecomp_core)
add_test(
    NAME noreturn_flow_tests
    COMMAND noreturn_flow_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_noreturn.elf
)

add_executable(cross_branch_tests tests/cross_branch_tests.cpp)
target_link_libraries(cross_branch_tests PRIVATE dcrecomp_core)
add_test(
    NAME cross_branch_tests
    COMMAND cross_branch_tests ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cross_branch.elf
)
add_test(
    NAME dc_recomp_cross_branch_codegen
    COMMAND dc_recomp
        ${CMAKE_CURRENT_SOURCE_DIR}/samples/sh4_cross_branch.elf
        --function _main
        --output ${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_cross_branch_output
)
set_tests_properties(dc_recomp_cross_branch_codegen PROPERTIES
    PASS_REGULAR_EXPRESSION "0x8C010048"
)

add_executable(dc_disc_probe
    src/disc/main.cpp
)

add_executable(dc_raw_boot_probe
    src/disc/raw_boot_probe.cpp
)
target_link_libraries(dc_raw_boot_probe PRIVATE dcrecomp_core)

add_executable(dc_boot_prepare
    src/disc/boot_prepare.cpp
)

add_executable(dc_raw_recomp
    src/disc/raw_recompiler.cpp
)
target_link_libraries(dc_raw_recomp PRIVATE dcrecomp_core)

# 0.0.136: functional + synthetic timing comparison for the indexed deferred
# PVR frame representation. Timing is informational; only equivalence is tested.
add_executable(pvr_indexed_tests tests/pvr_indexed_tests.cpp)
add_test(NAME pvr_indexed_tests COMMAND pvr_indexed_tests)

# 0.0.136: Flycast-style per-surface specialized TA vertex decoder equivalence
# and informational microbenchmark (performance is not an assertion).
add_executable(pvr_vertex_decoder_tests tests/pvr_vertex_decoder_tests.cpp)
add_test(NAME pvr_vertex_decoder_tests COMMAND pvr_vertex_decoder_tests)

# 0.0.141: exact native-PREF TA guard must remain mathematically equivalent
# to the generic QACR target + TA FIFO range classifier.
add_executable(pvr_pref_guard_tests tests/pvr_pref_guard_tests.cpp)
add_test(NAME pvr_pref_guard_tests COMMAND pvr_pref_guard_tests)

# Guard both emitter paths with actual host compilation and linking.
function(dcr_add_generated_build_test name producer source_dir)
    set_tests_properties(${producer} PROPERTIES FIXTURES_SETUP ${name}_source)
    add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_DIR=${source_dir}"
        "-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}"
        "-DGENERATOR=${CMAKE_GENERATOR}"
        "-DPLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DTOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCONFIG=$<IF:$<BOOL:$<CONFIG>>,$<CONFIG>,Release>"
        "-DEXE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/build_generated.cmake")
    set_tests_properties(${name} PROPERTIES
        FIXTURES_REQUIRED ${name}_source RUN_SERIAL TRUE TIMEOUT 900)
endfunction()
dcr_add_generated_build_test(generated_single_build cpp_emitter_tests "${GENERATED_TEST_DIR}")
dcr_add_generated_build_test(generated_program_build dc_recomp_multifunc_codegen
    "${CMAKE_CURRENT_BINARY_DIR}/dc_recomp_multifunc_output")

# v0.1: release-level validation for the Windows visual controller tester.
add_test(
    NAME controller_visual_tester_package
    COMMAND ${CMAKE_COMMAND} -DDCR_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/controller_visual_tester_check.cmake
)
