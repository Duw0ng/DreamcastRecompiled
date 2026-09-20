#include "dcrecomp/cpp_emitter.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

namespace {

struct Options {
    std::filesystem::path elf_path;
    std::string function_name{"_main"};
    std::filesystem::path output_directory{"generated"};
    std::size_t max_functions{4096};
};

void print_usage() {
    std::cout << "Uso: dc_recomp <archivo.elf> [--function nombre] [--output directorio] [--max-functions N]\n";
}

Options parse_options(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        throw std::runtime_error("Falta <archivo.elf>");
    }

    Options options;
    options.elf_path = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--function") {
            if (++i >= argc) throw std::runtime_error("--function requiere un nombre");
            options.function_name = argv[i];
        } else if (arg == "--output" || arg == "-o") {
            if (++i >= argc) throw std::runtime_error("--output requiere un directorio");
            options.output_directory = argv[i];
        } else if (arg == "--max-functions") {
            if (++i >= argc) throw std::runtime_error("--max-functions requiere un numero");
            options.max_functions = static_cast<std::size_t>(std::stoul(argv[i]));
            if (options.max_functions == 0) throw std::runtime_error("--max-functions debe ser > 0");
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        } else {
            throw std::runtime_error("Opcion desconocida: " + arg);
        }
    }
    return options;
}

std::string hex_address(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
    return out.str();
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse_options(argc, argv);
        const auto elf = dcrecomp::load_elf32(options.elf_path);

        dcrecomp::ProgramAnalysisOptions analysis_options;
        analysis_options.max_functions = options.max_functions;
        // _printf remains a host service in 0.0.96. Other normal STT_FUNC targets
        // are recursively discovered and emitted as native functions.
        analysis_options.native_override_symbols = {"_printf"};
        const auto program = dcrecomp::analyze_reachable_program(elf, options.function_name, analysis_options);

        std::size_t total_blocks = 0;
        std::size_t total_ir_ops = 0;
        std::size_t total_raw_ops = 0;
        for (const auto& unit : program.functions) {
            total_blocks += unit.cfg.blocks.size();
            for (const auto& block : unit.ir.blocks) {
                total_ir_ops += block.instructions.size();
                for (const auto& insn : block.instructions) {
                    if (insn.op == dcrecomp::DCIROp::RawSH4) ++total_raw_ops;
                }
            }
        }

        const auto emitted = dcrecomp::emit_cpp_program(
            elf, program, {options.output_directory, true, true, true});

        std::cout << "DreamcastRecomp Recompiler 0.0.170\n"
                     "=================================\n"
                  << "Input ELF:            " << options.elf_path.string() << "\n"
                  << "Root function:        " << program.root_name << "\n"
                  << "Root address:         " << hex_address(program.root_address) << "\n"
                  << "Reachable functions:  " << program.functions.size() << "\n"
                  << "Call-graph edges:     " << program.edges.size() << "\n"
                  << "External/native calls:" << program.external_calls.size() << "\n"
                  << "CFG blocks:           " << total_blocks << "\n"
                  << "DCIR ops:             " << total_ir_ops << "\n"
                  << "RAW_SH4:              " << total_raw_ops << "\n\n";

        std::cout << "Reachable function graph\n"
                     "========================\n";
        for (const auto& fn : program.functions) {
            std::size_t raw = 0;
            for (const auto& block : fn.ir.blocks) {
                for (const auto& insn : block.instructions) {
                    if (insn.op == dcrecomp::DCIROp::RawSH4) ++raw;
                }
            }
            std::cout << hex_address(fn.analysis.start_address) << "  " << fn.analysis.name
                      << "  blocks=" << fn.cfg.blocks.size() << " raw=" << raw << "\n";
        }
        for (const auto& edge : program.edges) {
            std::cout << "  " << edge.caller_name << " -> " << edge.callee_name
                      << "  (" << hex_address(edge.callee) << ")\n";
        }
        for (const auto& ext : program.external_calls) {
            std::cout << "  " << ext.caller_name << " -> "
                      << (ext.symbol.empty() ? (ext.resolved ? hex_address(ext.target) : std::string("<unresolved>")) : ext.symbol)
                      << "  [" << ext.reason << "]\n";
        }

        std::cout << "\nGenerated C++ program\n"
                     "=====================\n"
                  << emitted.program_header.string() << "\n"
                  << emitted.program_source.string() << "\n"
                  << emitted.runtime_header.string() << "\n"
                  << emitted.runtime_source.string() << "\n"
                  << emitted.image_source.string() << "\n"
                  << emitted.native_source.string() << "\n"
                  << emitted.runner_source.string() << "\n"
                  << emitted.cmake_file.string() << "\n\n"
                  << "Registered recompiled functions: " << emitted.functions.size() << "\n"
                  << "Embedded ELF data:               " << emitted.embedded_bytes << " bytes\n"
                  << "Zero-fill data:                  " << emitted.zero_fill_bytes << " bytes\n"
                  << "_printf native override:         " << hex_address(emitted.printf_address) << "\n";

        if (total_raw_ops != 0) {
            std::cout << "\nADVERTENCIA: algunas funciones alcanzables contienen RAW_SH4.\n"
                         "La salida compila, pero esas rutas fallaran si se ejecutan.\n";
        } else {
            std::cout << "\n[OK] Todas las funciones alcanzables fueron emitidas sin RAW_SH4.\n";
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
