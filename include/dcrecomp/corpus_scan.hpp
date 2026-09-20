#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dcrecomp {

struct UnknownOpcodeSample {
    std::uint16_t raw{};
    std::uint32_t address{};
    std::string function;
};

struct CorpusElfReport {
    std::filesystem::path path;
    bool loaded{};
    std::string error;
    std::size_t symbol_functions{};
    std::size_t scanned_functions{};
    std::size_t functions_with_unknown{};
    std::size_t analysis_failures{};
    std::size_t instructions{};
    std::size_t known{};
    std::size_t unknown{};
    std::size_t calls{};
    std::size_t unresolved_calls{};
    bool has_main{};
    std::size_t main_reachable_functions{};
    std::size_t main_raw_dcir{};
    std::size_t main_external_calls{};
    std::string main_error;
    std::vector<UnknownOpcodeSample> unknown_samples;
    std::vector<UnknownOpcodeSample> main_unknown_samples;
};

struct CorpusScanOptions {
    std::size_t max_functions_per_elf{10000};
    std::size_t max_unknown_samples_per_elf{100000};
    std::size_t main_max_functions{2048};
    bool scan_main_reachable{true};
};

CorpusElfReport scan_elf_compatibility(const std::filesystem::path& path,
                                       const CorpusScanOptions& options = {});

std::vector<std::filesystem::path> find_elf_files_recursive(const std::filesystem::path& input);

} // namespace dcrecomp
