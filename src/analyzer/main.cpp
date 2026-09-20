#include "dcrecomp/cfg.hpp"
#include "dcrecomp/dcir.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"
#include "dcrecomp/sh4_decoder.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::uint16_t EM_SH = 42;
constexpr std::uint32_t SHF_EXECINSTR = 0x4;

struct Options {
    std::string elf_path;
    bool disassemble{};
    bool stats{};
    bool symbols{};
    bool functions{};
    bool from_entry{};
    std::string function_name;
    bool analyze_function{};
    bool print_cfg{};
    bool emit_ir{};
    std::size_t max_instructions{}; // 0 = unlimited
};

void usage() {
    std::cout
        << "DreamcastRecomp dc_analyzer 0.0.170\n"
        << "Uso: dc_analyzer <archivo.elf> [opciones]\n\n"
        << "Opciones:\n"
        << "  --disassemble            Desensamblar secciones ejecutables\n"
        << "  --from-entry             Empezar el desensamblado en el entrypoint\n"
        << "  --max-instructions <N>   Limitar instrucciones mostradas (0 = todas)\n"
        << "  --stats                  Mostrar cobertura del decoder SH-4\n"
        << "  --symbols                Listar simbolos ELF\n"
        << "  --functions              Listar solo simbolos STT_FUNC\n"
        << "  --function <nombre>      Desensamblar codigo alcanzable de una funcion ELF\n"
        << "  --analyze-function <n>   Analisis detallado: literals, simbolos y llamadas\n"
        << "  --cfg <nombre>           Construir y mostrar Basic Blocks + CFG\n"
        << "  --emit-ir <nombre>       Traducir la funcion a Dreamcast IR (DCIR)\n"
        << "  --help                   Mostrar esta ayuda\n";
}

Options parse_options(int argc, char** argv) {
    if (argc < 2) {
        usage();
        std::exit(1);
    }

    Options out;
    out.elf_path = argv[1];

    for (int i = 2; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--disassemble") {
            out.disassemble = true;
        } else if (arg == "--stats") {
            out.stats = true;
        } else if (arg == "--symbols") {
            out.symbols = true;
        } else if (arg == "--functions") {
            out.functions = true;
        } else if (arg == "--from-entry") {
            out.from_entry = true;
            out.disassemble = true;
        } else if (arg == "--function" || arg == "--analyze-function" ||
                   arg == "--cfg" || arg == "--emit-ir") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Falta el nombre de " + std::string(arg));
            }
            out.function_name = argv[++i];
            out.analyze_function = (arg == "--analyze-function");
            out.print_cfg = (arg == "--cfg");
            out.emit_ir = (arg == "--emit-ir");
            out.disassemble = (arg == "--function");
        } else if (arg == "--max-instructions") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Falta el valor de --max-instructions");
            }
            out.max_instructions = static_cast<std::size_t>(std::stoull(argv[++i]));
        } else if (arg == "--help") {
            usage();
            std::exit(0);
        } else {
            throw std::runtime_error("Opcion desconocida: " + std::string(arg));
        }
    }

    return out;
}

std::uint16_t read_u16_le(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 2 > bytes.size()) {
        throw std::runtime_error("Lectura fuera del ELF");
    }
    return static_cast<std::uint16_t>(bytes[offset]) |
           (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::optional<std::size_t> entry_section_index(const dcrecomp::Elf32Image& elf) {
    for (std::size_t i = 0; i < elf.sections.size(); ++i) {
        const auto& s = elf.sections[i];
        if ((s.flags & SHF_EXECINSTR) == 0 || s.size == 0) {
            continue;
        }
        const std::uint64_t begin = s.address;
        const std::uint64_t end = begin + s.size;
        if (elf.entry >= begin && elf.entry < end) {
            return i;
        }
    }
    return std::nullopt;
}

void print_sections(const dcrecomp::Elf32Image& elf) {
    std::cout << "Secciones: " << elf.sections.size() << "\n\n";
    std::cout << std::left << std::setw(22) << "Nombre"
              << std::setw(12) << "Direccion"
              << std::setw(12) << "Offset"
              << std::setw(12) << "Tamano"
              << "Flags\n";

    for (const auto& section : elf.sections) {
        std::cout << std::left << std::setw(22) << (section.name.empty() ? "<sin nombre>" : section.name)
                  << "0x" << std::right << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << section.address
                  << "  0x" << std::setw(8) << section.offset
                  << "  0x" << std::setw(8) << section.size
                  << "  0x" << std::setw(8) << section.flags
                  << std::setfill(' ') << std::dec << "\n";
    }
}

void print_symbols(const dcrecomp::Elf32Image& elf, bool functions_only) {
    std::vector<const dcrecomp::ElfSymbol*> symbols;
    for (const auto& symbol : elf.symbols) {
        if (functions_only && !symbol.is_function()) {
            continue;
        }
        if (symbol.name.empty() || symbol.name == "<invalid>") {
            continue;
        }
        symbols.push_back(&symbol);
    }

    std::sort(symbols.begin(), symbols.end(), [](const auto* a, const auto* b) {
        if (a->value != b->value) return a->value < b->value;
        return a->name < b->name;
    });

    std::cout << "\n" << (functions_only ? "Funciones" : "Simbolos")
              << " ELF: " << symbols.size() << "\n";
    for (const auto* symbol : symbols) {
        std::cout << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
                  << symbol->value << std::setfill(' ') << std::dec
                  << "  size=" << std::setw(6) << symbol->size
                  << "  " << symbol->name << "\n";
    }
}

struct Coverage {
    std::size_t total{};
    std::size_t known{};
    std::size_t unknown{};
};

const dcrecomp::LiteralReference* literal_for_instruction(
    const dcrecomp::FunctionAnalysis& analysis,
    std::uint32_t instruction_address) {
    for (const auto& literal : analysis.literals) {
        if (literal.instruction_address == instruction_address) {
            return &literal;
        }
    }
    return nullptr;
}

const dcrecomp::CallReference* call_for_instruction(
    const dcrecomp::FunctionAnalysis& analysis,
    std::uint32_t instruction_address) {
    for (const auto& call : analysis.calls) {
        if (call.instruction_address == instruction_address) {
            return &call;
        }
    }
    return nullptr;
}

void print_instruction_line(const dcrecomp::sh4::Instruction& insn) {
    std::cout << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << insn.address
              << "  " << std::setw(4) << insn.raw << "  "
              << dcrecomp::sh4::to_string(insn)
              << (insn.has_delay_slot ? "  ; delay slot" : "")
              << std::setfill(' ') << std::dec << "\n";
}

Coverage process_function(const dcrecomp::Elf32Image& elf,
                          const Options& options,
                          bool print_disassembly) {
    const auto analysis = dcrecomp::analyze_function(elf, options.function_name);
    Coverage coverage{analysis.total(), analysis.known, analysis.unknown};

    if (print_disassembly) {
        const auto& section = elf.sections.at(analysis.section_index);
        std::cout << "\n[" << (section.name.empty() ? "<exec>" : section.name)
                  << "] funcion " << options.function_name << " (codigo alcanzable)\n";
        std::size_t printed = 0;
        for (const auto& insn : analysis.instructions) {
            if (options.max_instructions != 0 && printed >= options.max_instructions) break;
            print_instruction_line(insn);
            ++printed;
        }
    }

    return coverage;
}

Coverage process_code(const dcrecomp::Elf32Image& elf,
                      const Options& options,
                      bool print_disassembly) {
    if (!options.function_name.empty()) {
        return process_function(elf, options, print_disassembly);
    }

    Coverage coverage;
    std::size_t printed = 0;
    const auto entry_index = entry_section_index(elf);

    for (std::size_t section_index = 0; section_index < elf.sections.size(); ++section_index) {
        const auto& section = elf.sections[section_index];
        if ((section.flags & SHF_EXECINSTR) == 0 || section.size == 0) {
            continue;
        }
        if (options.from_entry && (!entry_index || section_index != *entry_index)) {
            continue;
        }
        if (section.offset > elf.bytes.size() || section.size > elf.bytes.size() - section.offset) {
            continue;
        }

        std::size_t start_off = 0;
        if (options.from_entry) {
            start_off = static_cast<std::size_t>(elf.entry - section.address);
            start_off &= ~std::size_t{1};
        }

        if (print_disassembly) {
            std::cout << "\n[" << (section.name.empty() ? "<exec>" : section.name) << "]";
            if (options.from_entry) {
                std::cout << " desde entrypoint";
            }
            std::cout << "\n";
        }

        for (std::size_t off = start_off; off + 1 < section.size; off += 2) {
            if (options.max_instructions != 0 && printed >= options.max_instructions) {
                return coverage;
            }

            const auto raw = read_u16_le(elf.bytes, section.offset + off);
            const auto addr = section.address + static_cast<std::uint32_t>(off);
            const auto insn = dcrecomp::sh4::decode(raw, addr);

            ++coverage.total;
            if (dcrecomp::sh4::is_known(insn)) ++coverage.known;
            else ++coverage.unknown;

            if (print_disassembly) {
                print_instruction_line(insn);
                ++printed;
            } else if (options.max_instructions != 0) {
                ++printed;
            }
        }
    }

    return coverage;
}

void print_function_analysis(const dcrecomp::Elf32Image& elf,
                             const dcrecomp::FunctionAnalysis& analysis) {
    std::cout << "\nDreamcastRecomp Function Analyzer\n"
              << "=================================\n"
              << "Function: " << analysis.name << "\n"
              << "Address:  0x" << std::hex << std::uppercase << analysis.start_address << "\n"
              << "End:      0x" << analysis.end_address << std::dec << "\n"
              << "Size:     " << (analysis.end_address - analysis.start_address) << " bytes\n";

    std::cout << "\nInstructions\n============\n";
    for (const auto& insn : analysis.instructions) {
        print_instruction_line(insn);

        if (const auto* literal = literal_for_instruction(analysis, insn.address)) {
            std::cout << "            literal @0x" << std::hex << std::uppercase
                      << literal->storage_address << " = 0x" << literal->value << std::dec << "\n";
            if (!literal->symbol.empty()) {
                std::cout << "            symbol:  " << literal->symbol << "\n";
            } else if (!literal->section.empty()) {
                std::cout << "            section: " << literal->section << "\n";
            }
        }

        if (const auto* call = call_for_instruction(analysis, insn.address)) {
            if (call->resolved) {
                std::cout << "            call -> 0x" << std::hex << std::uppercase << call->target << std::dec;
                if (!call->symbol.empty()) {
                    std::cout << " (" << call->symbol << ")";
                } else if (!call->section.empty()) {
                    std::cout << " (" << call->section << ")";
                }
                std::cout << "\n";
            } else {
                std::cout << "            call -> <unresolved>\n";
            }
        }
    }

    std::cout << "\nLiteral pool / PC-relative data\n===============================\n";
    if (analysis.literals.empty()) {
        std::cout << "<none>\n";
    } else {
        for (const auto& literal : analysis.literals) {
            std::cout << "0x" << std::hex << std::uppercase << literal.storage_address
                      << "  " << (literal.kind == dcrecomp::LiteralKind::Long32 ? "DATA32" : "DATA16")
                      << "  0x" << literal.value << std::dec;
            if (!literal.symbol.empty()) {
                std::cout << "  -> " << literal.symbol;
            } else if (!literal.section.empty()) {
                std::cout << "  -> " << literal.section;
            }
            if (!literal.inside_function) {
                std::cout << "  [external]";
            }
            std::cout << "\n";
        }
    }

    std::cout << "\nPadding / unreachable words\n===========================\n";
    if (analysis.padding_words.empty()) {
        std::cout << "<none>\n";
    } else {
        for (const auto address : analysis.padding_words) {
            const auto raw = dcrecomp::read_u16_vaddr(elf, address);
            std::cout << "0x" << std::hex << std::uppercase << address;
            if (raw) {
                std::cout << "  0x" << std::setw(4) << std::setfill('0') << *raw << std::setfill(' ');
            }
            std::cout << std::dec << "\n";
        }
    }

    const double percent = analysis.total()
        ? 100.0 * static_cast<double>(analysis.known) / static_cast<double>(analysis.total())
        : 0.0;
    std::cout << "\nCode coverage\n=============\n"
              << "Instructions: " << analysis.total() << "\n"
              << "Known:        " << analysis.known << "\n"
              << "Unknown:      " << analysis.unknown << "\n"
              << "Coverage:     " << std::fixed << std::setprecision(2) << percent << "%\n";

    std::cout << "\nCalls\n=====\n";
    if (analysis.calls.empty()) {
        std::cout << "<none>\n";
    } else {
        for (const auto& call : analysis.calls) {
            std::cout << "0x" << std::hex << std::uppercase << call.instruction_address << std::dec << "  ";
            if (!call.resolved) {
                std::cout << "<unresolved>\n";
                continue;
            }
            std::cout << "0x" << std::hex << std::uppercase << call.target << std::dec;
            if (!call.symbol.empty()) {
                std::cout << "  " << call.symbol;
            } else if (!call.section.empty()) {
                std::cout << "  " << call.section;
            }
            std::cout << "\n";
        }
    }
}

void print_cfg(const dcrecomp::ControlFlowGraph& cfg) {
    std::cout << "\nDreamcastRecomp Control Flow Graph\n"
              << "==================================\n"
              << "Function: " << cfg.function_name << "\n"
              << "Entry:    0x" << std::hex << std::uppercase << cfg.entry << std::dec << "\n"
              << "Blocks:   " << cfg.blocks.size() << "\n";

    for (const auto& block : cfg.blocks) {
        std::cout << "\nBB_" << std::hex << std::uppercase << block.start_address
                  << " [0x" << block.start_address << ", 0x" << block.end_address << ")"
                  << std::dec << "\n";
        for (const auto& insn : block.instructions) {
            std::cout << "  ";
            print_instruction_line(insn);
        }
    }

    std::cout << "\nEdges\n=====\n";
    if (cfg.edges.empty()) {
        std::cout << "<none>\n";
    } else {
        for (const auto& edge : cfg.edges) {
            std::cout << "BB_" << std::hex << std::uppercase << edge.from
                      << " -> BB_" << edge.to << std::dec
                      << "  [" << dcrecomp::to_string(edge.kind) << "]\n";
        }
    }
}

void print_ir(const dcrecomp::DCIRFunction& ir) {
    std::cout << "\nDreamcastRecomp IR (DCIR)\n"
              << "==========================\n"
              << "Function: " << ir.name << "\n"
              << "Entry:    0x" << std::hex << std::uppercase << ir.entry << std::dec << "\n";

    for (const auto& block : ir.blocks) {
        std::cout << "\nBB_" << std::hex << std::uppercase << block.start_address << std::dec << ":\n";
        if (block.instructions.empty()) {
            std::cout << "  <empty>\n";
            continue;
        }
        for (const auto& insn : block.instructions) {
            std::cout << "  0x" << std::hex << std::uppercase << insn.source_address << std::dec
                      << "  " << dcrecomp::format_dcir(insn) << "\n";
        }
    }
}

void print_coverage(const Coverage& coverage) {
    const double percent = coverage.total
        ? (100.0 * static_cast<double>(coverage.known) / static_cast<double>(coverage.total))
        : 0.0;

    std::cout << "\nDecoder SH-4 coverage\n"
              << "---------------------\n"
              << "Total:    " << coverage.total << "\n"
              << "Known:    " << coverage.known << "\n"
              << "Unknown:  " << coverage.unknown << "\n"
              << "Coverage: " << std::fixed << std::setprecision(2) << percent << "%\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse_options(argc, argv);
        const auto elf = dcrecomp::load_elf32(options.elf_path);

        std::cout << "DreamcastRecomp Analyzer 0.0.170\n"
                     "================================\n";
        std::cout << "ELF32 little-endian: si\n";
        std::cout << "Machine: " << elf.machine;
        if (elf.machine == EM_SH) {
            std::cout << " (EM_SH / SuperH)";
        } else {
            std::cout << " [ADVERTENCIA: no es EM_SH]";
        }
        std::cout << "\nEntry point: 0x" << std::hex << std::uppercase << elf.entry << std::dec << "\n";
        std::cout << "Simbolos detectados: " << elf.symbols.size() << "\n";

        if (!options.analyze_function && !options.print_cfg && !options.emit_ir) {
            print_sections(elf);
        }

        if (options.symbols) {
            print_symbols(elf, false);
        }
        if (options.functions) {
            print_symbols(elf, true);
        }

        if (options.analyze_function) {
            const auto analysis = dcrecomp::analyze_function(elf, options.function_name);
            print_function_analysis(elf, analysis);
        } else if (options.print_cfg || options.emit_ir) {
            const auto analysis = dcrecomp::analyze_function(elf, options.function_name);
            const auto cfg = dcrecomp::build_cfg(analysis);
            if (options.print_cfg) {
                print_cfg(cfg);
            } else {
                const auto ir = dcrecomp::lower_to_dcir(elf, analysis, cfg);
                print_ir(ir);
            }
        } else if (options.disassemble) {
            std::cout << "\nCodigo ejecutable:\n";
            const auto coverage = process_code(elf, options, true);
            if (options.stats) {
                print_coverage(coverage);
            }
        } else if (options.stats) {
            const auto coverage = process_code(elf, options, false);
            print_coverage(coverage);
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
