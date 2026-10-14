// Native seed-only compilation: deliberately does not compute retail closure.
// One or more previously unseen interpreter seeds become an independent DLL.
#include "dcrecomp/cpp_emitter.hpp"
#include "dcrecomp/function_analysis.hpp"
#include "dcrecomp/cfg.hpp"
#include "dcrecomp/dcir.hpp"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t u32(const std::string& s) {
    std::size_t used = 0;
    const auto v = std::stoull(s, &used, 0);
    if (used != s.size() || v > 0xffffffffull) throw std::runtime_error("invalid address: " + s);
    return static_cast<std::uint32_t>(v);
}
std::vector<std::uint8_t> read(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot read bootstrap " + p.string());
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
std::string fn_name(std::uint32_t n) {
    std::ostringstream out;
    out << "raw_" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << n;
    return out.str();
}
}

int main(int argc, char** argv) {
    try {
        std::filesystem::path input, output;
        std::uint32_t base = 0x8c008000u;
        std::set<std::uint32_t> seeds;
        std::size_t max_instructions = 4096u;
        for (int i=1; i<argc; ++i) {
            const std::string a = argv[i];
            if (a.rfind("--image=", 0)==0) input=a.substr(8);
            else if (a.rfind("--output=", 0)==0) output=a.substr(9);
            else if (a.rfind("--base=", 0)==0) base=u32(a.substr(7));
            else if (a.rfind("--seed=", 0)==0) seeds.insert(u32(a.substr(7)));
            else if (a.rfind("--max-instructions=",0)==0) max_instructions=std::stoul(a.substr(19));
            else throw std::runtime_error("unknown option: " + a);
        }
        if (input.empty() || output.empty() || seeds.empty() || !max_instructions)
            throw std::runtime_error("required: --image=BOOTSTRAP.BIN --base=0x... --seed=0x... --output=DIR");
        const auto bytes=read(input);
        dcrecomp::Elf32Image elf;
        elf.type=2u;
        elf.machine=42u;
        elf.entry=*seeds.begin();
        elf.bytes=bytes;
        dcrecomp::ElfSection section;
        section.name=".raw_boot";
        section.type=1u;
        section.flags=0x6u; // ALLOC | EXEC
        section.address=base;
        section.offset=0u;
        section.size=static_cast<std::uint32_t>(bytes.size());
        elf.sections.push_back(section);
        // Do not add every known retail symbol. Native calls to code outside
        // this batch must go through call_recompiled() and its pre-existing AOT.
        for (const auto seed:seeds) {
            if (seed & 1u || seed < base || static_cast<std::uint64_t>(seed)-base+32u > bytes.size())
                throw std::runtime_error("unaligned/out-of-image seed " + fn_name(seed));
            dcrecomp::ElfSymbol sym;
            sym.name=fn_name(seed);
            sym.value=seed;
            sym.size=static_cast<std::uint32_t>(bytes.size()-(seed-base));
            sym.info=0x12u; // STB_GLOBAL/STT_FUNC
            sym.section_index=0;
            elf.symbols.push_back(sym);
        }
        dcrecomp::ProgramAnalysis p;
        p.root_address=*seeds.begin();
        p.root_name=fn_name(p.root_address);
        std::size_t total=0;
        for(const auto seed:seeds) {
            dcrecomp::ProgramFunction unit;
            unit.analysis=dcrecomp::analyze_function_at(elf,seed);
            total+=unit.analysis.instructions.size();
            if (unit.analysis.instructions.empty() || unit.analysis.instructions.size()>max_instructions)
                throw std::runtime_error("seed analysis exceeded safety limit: " + fn_name(seed));
            unit.cfg=dcrecomp::build_cfg(unit.analysis);
            unit.ir=dcrecomp::lower_to_dcir(elf,unit.analysis,unit.cfg);
            // Unknown SH-4 opcodes must not be silently compiled to wrong semantics.
            for(const auto& b:unit.ir.blocks) for(const auto& op:b.instructions)
                if(op.op==dcrecomp::DCIROp::RawSH4)
                    throw std::runtime_error("RAW_SH4 in seed " + fn_name(seed) + ": keeping interpreter fallback");
            p.functions.push_back(std::move(unit));
        }
        std::filesystem::create_directories(output);
        const auto result=dcrecomp::emit_cpp_program(elf,p,{output,false,false,false,false});
        std::ofstream manifest(output / "seed_entries.txt");
        for(const auto seed:seeds) manifest << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << seed << '\n';
        std::cout << "[INCREMENTAL SH4] native-only seeds=" << p.functions.size()
                  << " instructions=" << total << " sources=" << result.program_sources.size()
                  << " output=" << output.string() << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[INCREMENTAL SH4] " << e.what() << '\n';
        return 1;
    }
}
