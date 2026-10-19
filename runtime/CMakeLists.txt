cmake_minimum_required(VERSION 3.20)
project(DreamcastRecompiledRuntime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DCR_HOST_AVX2 "Tune runtime for AVX2/FMA hosts" ON)
set(DCR_RUNTIME_OUTPUT_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "Directory shared with the generated game runner")

add_library(DreamcastRuntime SHARED
    dc_runtime.cpp
    dc_ct2_wait.cpp
    dc_sh4_interp.cpp
    dc_arm7.cpp
    dc_native_overrides.cpp
    dcr_plugin_host.cpp
)
target_include_directories(DreamcastRuntime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
# Stop Windows.h from pulling legacy winsock.h ahead of winsock2.h.
target_compile_definitions(DreamcastRuntime PRIVATE WIN32_LEAN_AND_MEAN)
set_target_properties(DreamcastRuntime PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    INTERPROCEDURAL_OPTIMIZATION FALSE
    RUNTIME_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/Release"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/Release"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/Release"
)

if(MSVC)
    target_compile_options(DreamcastRuntime PRIVATE
        $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi> $<$<CONFIG:Release>:/Ot>)
    if(DCR_HOST_AVX2)
        target_compile_options(DreamcastRuntime PRIVATE $<$<CONFIG:Release>:/arch:AVX2>)
        target_compile_definitions(DreamcastRuntime PRIVATE DCR_HOST_AVX2=1)
    endif()
else()
    target_compile_options(DreamcastRuntime PRIVATE $<$<CONFIG:Release>:-O3> -pipe)
endif()

if(WIN32)
    target_link_libraries(DreamcastRuntime PRIVATE user32 gdi32 winmm d3d11 d3dcompiler dxgi ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(DreamcastRuntime PRIVATE dl pthread)
endif()

# First fully external device implementation. The same ABI is reserved for
# PVR/AICA/GD-ROM/Maple; they can migrate one-by-one without touching guest code.
add_library(dcr_network SHARED plugins/dcr_network.cpp)
target_compile_definitions(dcr_network PRIVATE DCR_PLUGIN_BUILD=1 NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(dcr_network PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(dcr_network PRIVATE DreamcastRuntime)
set_target_properties(dcr_network PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    INTERPROCEDURAL_OPTIMIZATION FALSE
    RUNTIME_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
)
if(MSVC)
    target_compile_options(dcr_network PRIVATE $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>)
else()
    target_compile_options(dcr_network PRIVATE $<$<CONFIG:Release>:-O3>)
endif()
if(WIN32)
    target_link_libraries(dcr_network PRIVATE ws2_32)
endif()

# cpp35b: Maple owns controller, VMU, DMA/MMIO and persistence state privately.
# DreamcastRuntime keeps the frozen ABI1 layout only as a fallback compatibility shell;
# no DCRuntime pointer crosses the ABI2 Maple boundary.
add_library(dcr_maple SHARED plugins/dcr_maple.cpp)
target_compile_definitions(dcr_maple PRIVATE DCR_PLUGIN_BUILD=1 NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(dcr_maple PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(dcr_maple PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    INTERPROCEDURAL_OPTIMIZATION FALSE
    RUNTIME_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
)
if(MSVC)
    target_compile_options(dcr_maple PRIVATE $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>)
else()
    target_compile_options(dcr_maple PRIVATE $<$<CONFIG:Release>:-O3>)
endif()
if(WIN32)
    target_link_libraries(dcr_maple PRIVATE user32 winmm setupapi hid)
endif()

# cpp37c: physical media + command/DMA + complete native CDDA transport/decoder
# live in the native GD-ROM plugin. DreamcastRuntime keeps a compatibility
# mirror and the CDDA audio lifecycle until cpp37c.
add_library(dcr_gdrom SHARED plugins/dcr_gdrom.cpp)
target_compile_definitions(dcr_gdrom PRIVATE DCR_PLUGIN_BUILD=1 NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(dcr_gdrom PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(dcr_gdrom PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    INTERPROCEDURAL_OPTIMIZATION FALSE
    RUNTIME_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${DCR_RUNTIME_OUTPUT_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${DCR_RUNTIME_OUTPUT_DIR}/plugins/Release"
)
if(MSVC)
    target_compile_options(dcr_gdrom PRIVATE $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>)
else()
    target_compile_options(dcr_gdrom PRIVATE $<$<CONFIG:Release>:-O3>)
endif()

# Regression for combined portable retail BOOTSTRAP.BIN staging: preserve IP.BIN
# while scrambling only the executable. Build/test independently of the guest.
include(CTest)
if(BUILD_TESTING)
    # DCR v1.2 Rev 1: shared ARM7 RAM/MMIO mirrors and firmware fetch baseline.
    # This runs on Linux and Windows; native ARM7 recompilation must preserve
    # these exact bus semantics.
    add_executable(dcr_aica_arm_bus_tests tests_aica_arm_bus.cpp)
    target_include_directories(dcr_aica_arm_bus_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(dcr_aica_arm_bus_tests PRIVATE DreamcastRuntime)
    add_test(NAME dcr_aica_arm_bus_tests COMMAND dcr_aica_arm_bus_tests)

    # Rev 2: prevent finished one-shot voices from restarting when an unrelated
    # AICA slot executes the global KYONEX key strobe.
    add_executable(dcr_aica_one_shot_keyon_tests tests_aica_one_shot_keyon.cpp)
    target_include_directories(dcr_aica_one_shot_keyon_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(dcr_aica_one_shot_keyon_tests PRIVATE DreamcastRuntime)
    add_test(NAME dcr_aica_one_shot_keyon_tests COMMAND dcr_aica_one_shot_keyon_tests)

    # DCR V1.2 CT2 fast-path: disabled/enabled callbacks and changed-code guard.
    add_executable(dcr_sh4_trivial_ret_tests tests_sh4_trivial_ret.cpp)
    target_include_directories(dcr_sh4_trivial_ret_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(dcr_sh4_trivial_ret_tests PRIVATE DreamcastRuntime)
    add_test(NAME dcr_sh4_trivial_ret_off COMMAND dcr_sh4_trivial_ret_tests)
    set_tests_properties(dcr_sh4_trivial_ret_off PROPERTIES ENVIRONMENT "DCR_SH4_RETFAST=0")
    add_test(NAME dcr_sh4_trivial_ret_on COMMAND dcr_sh4_trivial_ret_tests)
    set_tests_properties(dcr_sh4_trivial_ret_on PROPERTIES ENVIRONMENT "DCR_SH4_RETFAST=1")

    # CT2 loop coalescing must stay within the existing 256-cycle scheduler quantum.
    add_executable(dcr_ct2_wait_tests tests_ct2_wait.cpp)
    target_include_directories(dcr_ct2_wait_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(dcr_ct2_wait_tests PRIVATE DreamcastRuntime)
    add_test(NAME dcr_ct2_wait_off COMMAND dcr_ct2_wait_tests)
    set_tests_properties(dcr_ct2_wait_off PROPERTIES ENVIRONMENT "DCR_CT2_WAIT_COALESCE=0;DCR_SH4_RETFAST=0")
    add_test(NAME dcr_ct2_wait_on COMMAND dcr_ct2_wait_tests)
    set_tests_properties(dcr_ct2_wait_on PROPERTIES ENVIRONMENT "DCR_CT2_WAIT_COALESCE=1;DCR_SH4_RETFAST=0")

    add_executable(dcr_portable_ipbin_stage_tests tests_portable_ipbin_stage.cpp)
    target_include_directories(dcr_portable_ipbin_stage_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(dcr_portable_ipbin_stage_tests PRIVATE DreamcastRuntime)
    add_test(NAME dcr_portable_ipbin_stage_tests COMMAND dcr_portable_ipbin_stage_tests)

    # interp-fallback-v1: SH-4 interpreter differential/semantic tests. Only
    # built for a standalone runtime build (cmake -S runtime), never inside a
    # per-game guest build that add_subdirectory()s this folder.
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../tests/interp/interp_fixtures.inc")
        add_executable(dcr_sh4_interp_tests tests_sh4_interp.cpp)
        target_include_directories(dcr_sh4_interp_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(dcr_sh4_interp_tests PRIVATE DreamcastRuntime)
        # The host reference build of the corpus must round like the SH-4 build.
        if(MSVC)
            target_compile_options(dcr_sh4_interp_tests PRIVATE /fp:precise /bigobj)
        else()
            target_compile_options(dcr_sh4_interp_tests PRIVATE -ffp-contract=off)
        endif()
        add_test(NAME dcr_sh4_interp_tests COMMAND dcr_sh4_interp_tests)
    endif()
endif()
