cmake_minimum_required(VERSION 3.20)
project(DreamcastRecompiledPortableLauncher LANGUAGES CXX)

add_executable(DreamcastRecompiled WIN32
    DreamcastRecompiledLauncher.cpp
)

target_compile_features(DreamcastRecompiled PRIVATE cxx_std_20)
target_compile_definitions(DreamcastRecompiled PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(DreamcastRecompiled PRIVATE
    comdlg32
    shell32
    bcrypt
)
