cmake_minimum_required(VERSION 3.20)
project(DreamcastRecompGenerated LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DCR_GENERATED_PROGRAM_SOURCES
    generated_program.cpp
)

add_library(dreamcast_generated STATIC
    dc_runtime.cpp
    dc_arm7.cpp
    dc_image.cpp
    dc_native_overrides.cpp
    ${DCR_GENERATED_PROGRAM_SOURCES}
)
target_include_directories(dreamcast_generated PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET dreamcast_generated PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
# 0.0.112: retail SH-4 code stays sharded for incremental rebuild granularity.
# 0.0.149: Windows commercial builds remain intentionally serial/low-memory; keep /MP disabled.
option(DCR_HOST_AVX2 "Tune commercial generated code for AVX2/FMA hosts" ON)
if(MSVC)
    target_compile_options(dreamcast_generated PRIVATE
        $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/Ot> $<$<CONFIG:Release>:/Gy>)
    if(DCR_HOST_AVX2)
        target_compile_options(dreamcast_generated PRIVATE $<$<CONFIG:Release>:/arch:AVX2>)
        target_compile_definitions(dreamcast_generated PRIVATE DCR_HOST_AVX2=1)
    endif()
else()
    target_compile_options(dreamcast_generated PRIVATE $<$<CONFIG:Release>:-O3> -pipe)
endif()
if(WIN32)
    target_link_libraries(dreamcast_generated PUBLIC user32 gdi32 winmm d3d11 d3dcompiler dxgi)
endif()

add_executable(generated_compile_test generated_compile_test.cpp)
target_link_libraries(generated_compile_test PRIVATE dreamcast_generated)

add_executable(dreamcast_program generated_runner.cpp)
target_link_libraries(dreamcast_program PRIVATE dreamcast_generated)
if(MSVC)
    target_link_options(dreamcast_program PRIVATE $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()
# Keep the runner non-LTO as well; enable host compiler optimisations normally.
