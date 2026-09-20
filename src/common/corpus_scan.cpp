#include "dcrecomp/corpus_scan.hpp"

#include "dcrecomp/dcir.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"
#include "dcrecomp/program_analysis.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>

namespace dcrecomp {
namespace {

bool has_elf_extension(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext == ".elf";
}

std::vector<const ElfSymbol*> unique_function_symbols(const Elf32Image& elf) {
    // Preserve the corpus baseline definition: "all functions" means ELF STT_FUNC
    // symbols. GLOBAL/WEAK STT_NOTYPE assembly entry points are still followed by
    // ProgramAnalysis when they are actually called, but treating every NOTYPE
    // symbol in .text as a standalone function would misclassify literal pools and
    // startup data labels as executable code.
    std::map<std::uint32_t, const ElfSymbol*> by_address;
    for (const auto& symbol : elf.symbols) {
        if (!symbol.is_function() || symbol.name.empty() || symbol.name == "<invalid>") continue;
        auto [it, inserted] = by_address.emplace(symbol.value & ~1u, &symbol);
        if (!inserted) {
            // Prefer a symbol with a non-zero size because it gives the analyzer a stronger range.
            if (it->second->size == 0 && symbol.size != 0) it->second = &symbol;
        }
    }

    std::vector<const ElfSymbol*> out;
    out.reserve(by_address.size());
    for (const auto& [_, sym] : by_address) out.push_back(sym);
    return out;
}

std::size_t count_raw_dcir(const ProgramAnalysis& program) {
    std::size_t raw = 0;
    for (const auto& fn : program.functions) {
        for (const auto& block : fn.ir.blocks) {
            for (const auto& insn : block.instructions) {
                if (insn.op == DCIROp::RawSH4) ++raw;
            }
        }
    }
    return raw;
}

} // namespace

std::vector<std::filesystem::path> find_elf_files_recursive(const std::filesystem::path& input) {
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    if (std::filesystem::is_regular_file(input, ec)) {
        if (has_elf_extension(input)) files.push_back(input);
        return files;
    }
    if (!std::filesystem::is_directory(input, ec)) {
        throw std::runtime_error("La ruta de corpus no existe o no es accesible: " + input.string());
    }

    for (std::filesystem::recursive_directory_iterator it(input, std::filesystem::directory_options::skip_permission_denied, ec), end;
         it != end;
         it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (it->is_regular_file(ec) && has_elf_extension(it->path())) files.push_back(it->path());
    }
    std::sort(files.begin(), files.end());
    return files;
}

CorpusElfReport scan_elf_compatibility(const std::filesystem::path& path,
                                       const CorpusScanOptions& options) {
    CorpusElfReport report;
    report.path = path;

    try {
        const auto elf = load_elf32(path);
        report.loaded = true;

        auto functions = unique_function_symbols(elf);
        report.symbol_functions = functions.size();
        if (functions.size() > options.max_functions_per_elf) {
            functions.resize(options.max_functions_per_elf);
        }

        for (const auto* symbol : functions) {
            try {
                const auto analysis = analyze_function(elf, symbol->name);
                ++report.scanned_functions;
                report.instructions += analysis.total();
                report.known += analysis.known;
                report.unknown += analysis.unknown;
                report.calls += analysis.calls.size();
                for (const auto& call : analysis.calls) {
                    if (!call.resolved) ++report.unresolved_calls;
                }
                if (analysis.unknown != 0) ++report.functions_with_unknown;

                if (report.unknown_samples.size() < options.max_unknown_samples_per_elf) {
                    for (const auto& insn : analysis.instructions) {
                        if (insn.opcode != sh4::Opcode::Unknown) continue;
                        report.unknown_samples.push_back({insn.raw, insn.address, analysis.name});
                        if (report.unknown_samples.size() >= options.max_unknown_samples_per_elf) break;
                    }
                }
            } catch (const std::exception&) {
                ++report.analysis_failures;
            }
        }

        if (options.scan_main_reachable) {
            std::string main_name;
            for (const auto& symbol : elf.symbols) {
                if (!symbol.is_function()) continue;
                if (symbol.name == "_main") {
                    main_name = "_main";
                    break;
                }
                if (main_name.empty() && symbol.name == "main") main_name = "main";
            }
            report.has_main = !main_name.empty();
            if (report.has_main) {
                try {
                    ProgramAnalysisOptions po;
                    po.max_functions = options.main_max_functions;
                    po.native_override_symbols = {"_printf"};
                    const auto program = analyze_reachable_program(elf, main_name, po);
                    report.main_reachable_functions = program.functions.size();
                    report.main_raw_dcir = count_raw_dcir(program);
                    report.main_external_calls = program.external_calls.size();
                    for (const auto& fn : program.functions) {
                        for (const auto& insn : fn.analysis.instructions) {
                            if (insn.opcode != sh4::Opcode::Unknown) continue;
                            report.main_unknown_samples.push_back({insn.raw, insn.address, fn.analysis.name});
                            if (report.main_unknown_samples.size() >= options.max_unknown_samples_per_elf) break;
                        }
                        if (report.main_unknown_samples.size() >= options.max_unknown_samples_per_elf) break;
                    }
                } catch (const std::exception& e) {
                    report.main_error = e.what();
                }
            }
        }
    } catch (const std::exception& e) {
        report.error = e.what();
    }

    return report;
}

} // namespace dcrecomp
