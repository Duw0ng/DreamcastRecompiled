#include "dcrecomp/corpus_scan.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
    std::filesystem::path input;
    std::filesystem::path csv_output{"corpus_report.csv"};
    std::filesystem::path text_output{"corpus_report.txt"};
    std::size_t top{40};
    std::size_t max_functions_per_elf{10000};
    std::size_t main_max_functions{2048};
    bool scan_main{true};
};

struct UnknownAggregate {
    std::size_t count{};
    std::set<std::string> elfs;
    std::string example_elf;
    std::string example_function;
    std::uint32_t example_address{};
};

struct UnknownFamilyAggregate : UnknownAggregate {
    std::uint16_t example_raw{};
};

std::string unknown_family(std::uint16_t raw) {
    // High-value SH-4 families not implemented yet. The mask describes which
    // nibbles are operands rather than opcode bits. This lets the scanner rank
    // instruction families instead of treating every register combination as a
    // different opcode.
    if ((raw & 0xF00F) == 0x3004) return "DIV1 Rm,Rn [3nm4]";
    if ((raw & 0xF0FF) == 0x0023) return "BRAF Rn [0n23]";
    if ((raw & 0xF0FF) == 0x0003) return "BSRF Rn [0n03]";
    if ((raw & 0xF00F) == 0x000F) return "MAC.L @Rm+,@Rn+ [0nmF]";
    if ((raw & 0xF00F) == 0x400F) return "MAC.W @Rm+,@Rn+ [4nmF]";
    if ((raw & 0xF00F) == 0x200C) return "CMP/STR Rm,Rn [2nmC]";
    if ((raw & 0xF00F) == 0x2007) return "DIV0S Rm,Rn [2nm7]";
    if (raw == 0x0019) return "DIV0U [0019]";

    // SH-4 FPU register-register families. These dominate real KallistiOS once
    // the integer/system baseline is covered.
    if ((raw & 0xF00F) == 0xF000) return "FADD FRm,FRn [Fnm0]";
    if ((raw & 0xF00F) == 0xF001) return "FSUB FRm,FRn [Fnm1]";
    if ((raw & 0xF00F) == 0xF002) return "FMUL FRm,FRn [Fnm2]";
    if ((raw & 0xF00F) == 0xF003) return "FDIV FRm,FRn [Fnm3]";
    if ((raw & 0xF00F) == 0xF004) return "FCMP/EQ FRm,FRn [Fnm4]";
    if ((raw & 0xF00F) == 0xF005) return "FCMP/GT FRm,FRn [Fnm5]";
    if ((raw & 0xF00F) == 0xF006) return "FMOV.S @(R0,Rm),FRn [Fnm6]";
    if ((raw & 0xF00F) == 0xF007) return "FMOV.S FRm,@(R0,Rn) [Fnm7]";
    if ((raw & 0xF00F) == 0xF008) return "FMOV.S @Rm,FRn [Fnm8]";
    if ((raw & 0xF00F) == 0xF009) return "FMOV.S @Rm+,FRn [Fnm9]";
    if ((raw & 0xF00F) == 0xF00A) return "FMOV.S FRm,@Rn [FnmA]";
    if ((raw & 0xF00F) == 0xF00B) return "FMOV.S FRm,@-Rn [FnmB]";
    if ((raw & 0xF00F) == 0xF00C) return "FMOV FRm,FRn [FnmC]";
    if ((raw & 0xF00F) == 0xF00D) return "FPU unary/conversion [FnmD]";
    if ((raw & 0xF00F) == 0xF00E) return "FMAC FR0,FRm,FRn [FnmE]";

    // Useful broad buckets for the remaining CPU work.
    if ((raw & 0xF000) == 0xF000) return "FPU/SH-4 0xF*** other";
    if ((raw & 0xF000) == 0x4000) return "SH-4 0x4*** special/shift";
    if ((raw & 0xF000) == 0x0000) return "SH-4 0x0*** system/control";
    if ((raw & 0xF000) == 0x3000) return "SH-4 0x3*** arithmetic";
    if ((raw & 0xF000) == 0x2000) return "SH-4 0x2*** logical/multiply";

    std::ostringstream out;
    out << "Other top-nibble 0x" << std::hex << std::uppercase << ((raw >> 12) & 0xF) << "***";
    return out.str();
}

std::string hex16(std::uint16_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << value;
    return out.str();
}

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
    return out.str();
}

std::string csv_escape(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

void usage() {
    std::cout
        << "DreamcastRecomp dc_corpus_scan 0.0.58\n"
        << "Uso: dc_corpus_scan <directorio-o-elf> [opciones]\n\n"
        << "  --csv <archivo>             reporte por ELF (default corpus_report.csv)\n"
        << "  --text <archivo>            resumen/ranking (default corpus_report.txt)\n"
        << "  --top <N>                   opcodes desconocidos a mostrar (default 40)\n"
        << "  --max-functions <N>         maximo de funciones escaneadas por ELF\n"
        << "  --main-max-functions <N>    limite del call graph desde _main\n"
        << "  --no-main                   no analiza call graph alcanzable desde _main\n";
}

Options parse(int argc, char** argv) {
    if (argc < 2) {
        usage();
        throw std::runtime_error("Falta el directorio/ELF de entrada");
    }
    Options o;
    o.input = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error(arg + " requiere un valor");
            return argv[i];
        };
        if (arg == "--csv") o.csv_output = next();
        else if (arg == "--text") o.text_output = next();
        else if (arg == "--top") o.top = std::stoull(next());
        else if (arg == "--max-functions") o.max_functions_per_elf = std::stoull(next());
        else if (arg == "--main-max-functions") o.main_max_functions = std::stoull(next());
        else if (arg == "--no-main") o.scan_main = false;
        else if (arg == "--help" || arg == "-h") { usage(); std::exit(0); }
        else throw std::runtime_error("Opcion desconocida: " + arg);
    }
    return o;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse(argc, argv);
        const auto files = dcrecomp::find_elf_files_recursive(options.input);
        if (files.empty()) throw std::runtime_error("No se encontraron archivos .elf");

        dcrecomp::CorpusScanOptions scan_options;
        scan_options.max_functions_per_elf = options.max_functions_per_elf;
        scan_options.main_max_functions = options.main_max_functions;
        scan_options.scan_main_reachable = options.scan_main;

        std::vector<dcrecomp::CorpusElfReport> reports;
        reports.reserve(files.size());
        std::map<std::uint16_t, UnknownAggregate> unknowns;
        std::map<std::uint16_t, UnknownAggregate> main_unknowns;
        std::map<std::string, UnknownFamilyAggregate> unknown_families;
        std::map<std::string, UnknownFamilyAggregate> main_unknown_families;
        std::size_t loaded = 0, clean = 0, all_unknown = 0, all_known = 0, scanned_functions = 0;
        std::size_t main_clean = 0, main_scanned = 0;

        const auto started = std::chrono::steady_clock::now();
        std::cout << "DreamcastRecomp Corpus Scanner 0.0.58\n"
                     "===================================\n"
                  << "ELF encontrados: " << files.size() << "\n\n";

        for (std::size_t i = 0; i < files.size(); ++i) {
            auto r = dcrecomp::scan_elf_compatibility(files[i], scan_options);
            if (r.loaded) {
                ++loaded;
                scanned_functions += r.scanned_functions;
                all_unknown += r.unknown;
                all_known += r.known;
                if (r.unknown == 0 && r.analysis_failures == 0) ++clean;
                if (r.has_main && r.main_error.empty()) {
                    ++main_scanned;
                    if (r.main_raw_dcir == 0) ++main_clean;
                }
                for (const auto& sample : r.unknown_samples) {
                    auto& a = unknowns[sample.raw];
                    ++a.count;
                    a.elfs.insert(files[i].string());
                    if (a.example_elf.empty()) {
                        a.example_elf = files[i].string();
                        a.example_function = sample.function;
                        a.example_address = sample.address;
                    }
                    auto& f = unknown_families[unknown_family(sample.raw)];
                    ++f.count;
                    f.elfs.insert(files[i].string());
                    if (f.example_elf.empty()) {
                        f.example_raw = sample.raw;
                        f.example_elf = files[i].string();
                        f.example_function = sample.function;
                        f.example_address = sample.address;
                    }
                }
                for (const auto& sample : r.main_unknown_samples) {
                    auto& a = main_unknowns[sample.raw];
                    ++a.count;
                    a.elfs.insert(files[i].string());
                    if (a.example_elf.empty()) {
                        a.example_elf = files[i].string();
                        a.example_function = sample.function;
                        a.example_address = sample.address;
                    }
                    auto& f = main_unknown_families[unknown_family(sample.raw)];
                    ++f.count;
                    f.elfs.insert(files[i].string());
                    if (f.example_elf.empty()) {
                        f.example_raw = sample.raw;
                        f.example_elf = files[i].string();
                        f.example_function = sample.function;
                        f.example_address = sample.address;
                    }
                }
            }
            std::cout << "[" << (i + 1) << "/" << files.size() << "] " << files[i].filename().string();
            if (!r.loaded) std::cout << "  ERROR";
            else std::cout << "  funcs=" << r.scanned_functions << " unknown=" << r.unknown;
            if (r.has_main && r.main_error.empty()) std::cout << " main_raw=" << r.main_raw_dcir;
            std::cout << "\n";
            reports.push_back(std::move(r));
        }

        auto rank_map = [](const auto& source) {
            std::vector<std::pair<std::uint16_t, UnknownAggregate>> ranked(source.begin(), source.end());
            std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
                if (a.second.count != b.second.count) return a.second.count > b.second.count;
                return a.first < b.first;
            });
            return ranked;
        };
        const auto ranked = rank_map(unknowns);
        const auto ranked_main = rank_map(main_unknowns);
        auto rank_families = [](const auto& source) {
            std::vector<std::pair<std::string, UnknownFamilyAggregate>> ranked(source.begin(), source.end());
            std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
                if (a.second.count != b.second.count) return a.second.count > b.second.count;
                return a.first < b.first;
            });
            return ranked;
        };
        const auto ranked_families = rank_families(unknown_families);
        const auto ranked_main_families = rank_families(main_unknown_families);

        std::ofstream csv(options.csv_output);
        if (!csv) throw std::runtime_error("No se pudo crear CSV: " + options.csv_output.string());
        csv << "path,loaded,symbol_functions,scanned_functions,functions_with_unknown,analysis_failures,instructions,known,unknown,calls,unresolved_calls,has_main,main_reachable_functions,main_raw_dcir,main_external_calls,error,main_error\n";
        for (const auto& r : reports) {
            csv << csv_escape(r.path.string()) << ',' << (r.loaded ? 1 : 0) << ','
                << r.symbol_functions << ',' << r.scanned_functions << ',' << r.functions_with_unknown << ','
                << r.analysis_failures << ',' << r.instructions << ',' << r.known << ',' << r.unknown << ','
                << r.calls << ',' << r.unresolved_calls << ',' << (r.has_main ? 1 : 0) << ','
                << r.main_reachable_functions << ',' << r.main_raw_dcir << ',' << r.main_external_calls << ','
                << csv_escape(r.error) << ',' << csv_escape(r.main_error) << "\n";
        }

        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::ofstream text(options.text_output);
        if (!text) throw std::runtime_error("No se pudo crear reporte: " + options.text_output.string());
        auto write_summary = [&](std::ostream& out) {
            out << "DreamcastRecomp 0.0.58 - KallistiOS corpus compatibility\n"
                   "=========================================================\n"
                << "ELF encontrados:                " << files.size() << "\n"
                << "ELF cargados:                   " << loaded << "\n"
                << "ELF ISA-clean (all functions):  " << clean << "\n"
                << "Funciones escaneadas:           " << scanned_functions << "\n"
                << "Instrucciones conocidas:        " << all_known << "\n"
                << "Instrucciones desconocidas:     " << all_unknown << "\n"
                << "_main call graphs escaneados:   " << main_scanned << "\n"
                << "_main con RAW_SH4=0:             " << main_clean << "\n"
                << "Tiempo:                          " << std::fixed << std::setprecision(2) << elapsed << " s\n\n"
                << "NOTA: ISA-clean significa que el decoder/DCIR no encontro opcodes desconocidos en las funciones simbolizadas.\n"
                   "No implica que el programa completo ya pueda ejecutarse: pueden faltar servicios KOS/MMIO/PVR/AICA/etc.\n\n"
                   "Top familias desconocidas alcanzables desde _main\n"
                   "==================================================\n";
            const auto main_family_limit = std::min(options.top, ranked_main_families.size());
            for (std::size_t i = 0; i < main_family_limit; ++i) {
                const auto& [name, a] = ranked_main_families[i];
                out << std::setw(3) << (i + 1) << ". " << name
                    << "  count=" << a.count << "  elfs=" << a.elfs.size()
                    << "  sample=" << hex16(a.example_raw)
                    << "  " << a.example_function << "@" << hex32(a.example_address)
                    << "\n";
            }
            out << "\nTop raw words desconocidos alcanzables desde _main\n"
                   "====================================================\n";
            const auto main_limit = std::min(options.top, ranked_main.size());
            for (std::size_t i = 0; i < main_limit; ++i) {
                const auto& [raw, a] = ranked_main[i];
                out << std::setw(3) << (i + 1) << ". " << hex16(raw)
                    << "  count=" << a.count << "  elfs=" << a.elfs.size()
                    << "  example=" << a.example_function << "@" << hex32(a.example_address)
                    << "  " << a.example_elf << "\n";
            }
            out << "\nTop familias desconocidas en todas las funciones simbolizadas\n"
                   "==========================================================\n";
            const auto family_limit = std::min(options.top, ranked_families.size());
            for (std::size_t i = 0; i < family_limit; ++i) {
                const auto& [name, a] = ranked_families[i];
                out << std::setw(3) << (i + 1) << ". " << name
                    << "  count=" << a.count << "  elfs=" << a.elfs.size()
                    << "  sample=" << hex16(a.example_raw)
                    << "  " << a.example_function << "@" << hex32(a.example_address)
                    << "\n";
            }
            out << "\nTop raw words SH-4 desconocidos en todas las funciones simbolizadas\n"
                   "=================================================================\n";
            const auto limit = std::min(options.top, ranked.size());
            for (std::size_t i = 0; i < limit; ++i) {
                const auto& [raw, a] = ranked[i];
                out << std::setw(3) << (i + 1) << ". " << hex16(raw)
                    << "  count=" << a.count << "  elfs=" << a.elfs.size()
                    << "  example=" << a.example_function << "@" << hex32(a.example_address)
                    << "  " << a.example_elf << "\n";
            }
        };
        write_summary(text);
        write_summary(std::cout);

        std::cout << "\nReportes:\n  " << options.text_output.string() << "\n  " << options.csv_output.string() << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
