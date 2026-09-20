#pragma once

#include "dcrecomp/elf32.hpp"
#include "dcrecomp/sh4_decoder.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcrecomp {

enum class LiteralKind {
    Word16,
    Long32,
};

struct LiteralReference {
    std::uint32_t instruction_address{};
    std::uint32_t storage_address{};
    std::uint32_t value{};
    std::uint8_t destination_register{};
    LiteralKind kind{LiteralKind::Long32};
    bool inside_function{};
    std::string symbol;
    std::string section;
};

struct CallReference {
    std::uint32_t instruction_address{};
    bool direct{};
    bool resolved{};
    std::uint32_t target{};
    std::string symbol;
    std::string section;
};

struct DynamicBranchReference {
    std::uint32_t instruction_address{};
    std::vector<std::uint32_t> targets;
};

struct FunctionAnalysis {
    std::string name;
    std::size_t section_index{};
    std::uint32_t start_address{};
    std::uint32_t end_address{}; // exclusive

    // Only instructions reachable from the function entry are included here.
    // Literal-pool words and unreachable alignment padding are deliberately excluded.
    std::vector<sh4::Instruction> instructions;
    std::vector<LiteralReference> literals;
    std::vector<CallReference> calls;
    std::vector<DynamicBranchReference> dynamic_branches;
    // Multiple literal callback values that can reach one indirect JSR/JMP
    // through a local conditional join. Kept separate from jump-table edges so
    // raw commercial closure can apply callable-entry validation explicitly.
    std::vector<DynamicBranchReference> branch_selected_calls;
    std::vector<std::uint32_t> padding_words;

    std::size_t known{};
    std::size_t unknown{};

    std::size_t total() const { return instructions.size(); }
};

FunctionAnalysis analyze_function(const Elf32Image& elf, std::string_view function_name);
FunctionAnalysis analyze_function_at(const Elf32Image& elf, std::uint32_t function_address);
// Analyze an executable entry point that may live inside another ELF function symbol.
// Used for shared libgcc tails / alternate entries reached by cross-symbol branches.
FunctionAnalysis analyze_code_fragment_at(const Elf32Image& elf, std::uint32_t entry_address);

// Helpers used by the analyzer UI and later by DCIR generation.
const ElfSymbol* find_symbol_at(const Elf32Image& elf, std::uint32_t address);
const ElfSection* find_section_at(const Elf32Image& elf, std::uint32_t address);
std::optional<std::uint16_t> read_u16_vaddr(const Elf32Image& elf, std::uint32_t address);
std::optional<std::uint32_t> read_u32_vaddr(const Elf32Image& elf, std::uint32_t address);

} // namespace dcrecomp
