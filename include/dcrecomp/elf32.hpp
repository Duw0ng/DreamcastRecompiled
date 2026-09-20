#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dcrecomp {

struct ElfSection {
    std::string name;
    std::uint32_t type{};
    std::uint32_t flags{};
    std::uint32_t address{};
    std::uint32_t offset{};
    std::uint32_t size{};
    std::uint32_t link{};
    std::uint32_t info{};
    std::uint32_t entsize{};
};

struct ElfSymbol {
    std::string name;
    std::uint32_t value{};
    std::uint32_t size{};
    std::uint8_t info{};
    std::uint8_t other{};
    std::uint16_t section_index{};

    std::uint8_t type() const { return static_cast<std::uint8_t>(info & 0x0F); }
    std::uint8_t binding() const { return static_cast<std::uint8_t>(info >> 4); }
    bool is_function() const { return type() == 2; } // STT_FUNC
};

struct Elf32Image {
    std::uint16_t type{};
    std::uint16_t machine{};
    std::uint32_t entry{};
    std::vector<std::uint8_t> bytes;
    std::vector<ElfSection> sections;
    std::vector<ElfSymbol> symbols;
};

// Loads a 32-bit little-endian ELF. Dreamcast SH-4 ELF files use EM_SH (42).
Elf32Image load_elf32(const std::filesystem::path& path);

} // namespace dcrecomp
