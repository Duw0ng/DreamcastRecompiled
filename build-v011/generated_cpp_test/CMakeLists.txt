cmake_minimum_required(VERSION 3.20)
project(DreamcastRecompGenerated LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dreamcast_generated STATIC
    dc_runtime.cpp
    dc_arm7.cpp
    dc_image.cpp
    dc_native_overrides.cpp
    generated_main.cpp
)
target_include_directories(dreamcast_generated PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Keep full-program LTO disabled for huge retail closures, but make the
# runtime/rasterizer TU aggressively optimize and inline hot helpers.
option(DCR_HOST_AVX2 "Tune commercial generated code for AVX2/FMA hosts" ON)
if(MSVC)
    target_compile_options(dreamcast_generated PRIVATE
        $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/Ot>)
    if(DCR_HOST_AVX2)
        target_compile_options(dreamcast_generated PRIVATE $<$<CONFIG:Release>:/arch:AVX2>)
        target_compile_definitions(dreamcast_generated PRIVATE DCR_HOST_AVX2=1)
    endif()
else()
    target_compile_options(dreamcast_generated PRIVATE $<$<CONFIG:Release>:-O3>)
endif()
# IPO/LTO is intentionally disabled by default for large commercial closures.
if(WIN32)
    target_link_libraries(dreamcast_generated PUBLIC user32 gdi32 winmm d3d11 d3dcompiler dxgi)
endif()

add_executable(generated_compile_test generated_compile_test.cpp)
target_link_libraries(generated_compile_test PRIVATE dreamcast_generated)

add_executable(dreamcast_hello generated_runner.cpp)
target_link_libraries(dreamcast_hello PRIVATE dreamcast_generated)
# Keep the runner non-LTO as well; enable host compiler optimisations normally.
