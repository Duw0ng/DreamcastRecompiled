#include "dcrecomp/elf32.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace dcrecomp {
namespace {

constexpr std::size_t EI_CLASS = 4;
constexpr std::size_t EI_DATA = 5;
constexpr std::uint8_t ELFCLASS32 = 1;
constexpr std::uint8_t ELFDATA2LSB = 1;
constexpr std::uint32_t SHT_SYMTAB = 2;
constexpr std::uint32_t SHT_DYNSYM = 11;

#pragma pack(push, 1)
struct Elf32Ehdr {
    std::uint8_t ident[16];
    std::uint16_t type;
    std::uint16_t machine;
    std::uint32_t version;
    std::uint32_t entry;
    std::uint32_t phoff;
    std::uint32_t shoff;
    std::uint32_t flags;
    std::uint16_t ehsize;
    std::uint16_t phentsize;
    std::uint16_t phnum;
    std::uint16_t shentsize;
    std::uint16_t shnum;
    std::uint16_t shstrndx;
};

struct Elf32Shdr {
    std::uint32_t name;
    std::uint32_t type;
    std::uint32_t flags;
    std::uint32_t addr;
    std::uint32_t offset;
    std::uint32_t size;
    std::uint32_t link;
    std::uint32_t info;
    std::uint32_t addralign;
    std::uint32_t entsize;
};

struct Elf32Sym {
    std::uint32_t name;
    std::uint32_t value;
    std::uint32_t size;
    std::uint8_t info;
    std::uint8_t other;
    std::uint16_t shndx;
};
#pragma pack(pop)

template <typename T>
T read_struct(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) {
        throw std::runtime_error("ELF truncado al leer una estructura");
    }
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}

std::string read_c_string(const std::vector<std::uint8_t>& bytes,
                          std::size_t base,
                          std::size_t limit,
                          std::uint32_t offset) {
    if (offset >= limit || base > bytes.size() || limit > bytes.size() - base) {
        return "<invalid>";
    }

    const auto begin = base + static_cast<std::size_t>(offset);
    const auto end_limit = base + limit;
    std::size_t end = begin;
    while (end < end_limit && bytes[end] != 0) {
        ++end;
    }
    return std::string(reinterpret_cast<const char*>(bytes.data() + begin), end - begin);
}

bool range_valid(const std::vector<std::uint8_t>& bytes, std::uint32_t offset, std::uint32_t size) {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

} // namespace

Elf32Image load_elf32(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("No se pudo abrir el archivo: " + path.string());
    }

    const auto end = file.tellg();
    if (end < static_cast<std::streamoff>(sizeof(Elf32Ehdr))) {
        throw std::runtime_error("El archivo es demasiado pequeño para ser un ELF32");
    }

    Elf32Image image;
    image.bytes.resize(static_cast<std::size_t>(end));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(image.bytes.data()), static_cast<std::streamsize>(image.bytes.size()));
    if (!file) {
        throw std::runtime_error("Error leyendo el archivo ELF");
    }

    const auto header = read_struct<Elf32Ehdr>(image.bytes, 0);
    if (!(header.ident[0] == 0x7F && header.ident[1] == 'E' &&
          header.ident[2] == 'L' && header.ident[3] == 'F')) {
        throw std::runtime_error("El archivo no tiene la firma ELF");
    }
    if (header.ident[EI_CLASS] != ELFCLASS32) {
        throw std::runtime_error("Por ahora DreamcastRecomp solo acepta ELF de 32 bits");
    }
    if (header.ident[EI_DATA] != ELFDATA2LSB) {
        throw std::runtime_error("Por ahora solo se acepta ELF little-endian");
    }
    if (header.shentsize != sizeof(Elf32Shdr) && header.shnum != 0) {
        throw std::runtime_error("Tamaño inesperado de section header ELF32");
    }

    image.type = header.type;
    image.machine = header.machine;
    image.entry = header.entry;

    std::vector<Elf32Shdr> raw_sections;
    raw_sections.reserve(header.shnum);
    for (std::uint16_t i = 0; i < header.shnum; ++i) {
        const auto offset = static_cast<std::size_t>(header.shoff) +
                            static_cast<std::size_t>(i) * header.shentsize;
        raw_sections.push_back(read_struct<Elf32Shdr>(image.bytes, offset));
    }

    std::size_t names_base = 0;
    std::size_t names_size = 0;
    if (header.shstrndx < raw_sections.size()) {
        const auto& names = raw_sections[header.shstrndx];
        if (range_valid(image.bytes, names.offset, names.size)) {
            names_base = names.offset;
            names_size = names.size;
        }
    }

    image.sections.reserve(raw_sections.size());
    for (const auto& section : raw_sections) {
        ElfSection out;
        out.name = names_size ? read_c_string(image.bytes, names_base, names_size, section.name) : "";
        out.type = section.type;
        out.flags = section.flags;
        out.address = section.addr;
        out.offset = section.offset;
        out.size = section.size;
        out.link = section.link;
        out.info = section.info;
        out.entsize = section.entsize;
        image.sections.push_back(std::move(out));
    }

    // Parse ELF symbol tables when present. KallistiOS development ELFs normally retain
    // useful symbols, which makes them excellent inputs for early recompilation work.
    for (std::size_t section_index = 0; section_index < raw_sections.size(); ++section_index) {
        const auto& symtab = raw_sections[section_index];
        if (symtab.type != SHT_SYMTAB && symtab.type != SHT_DYNSYM) {
            continue;
        }
        if (!range_valid(image.bytes, symtab.offset, symtab.size)) {
            continue;
        }
        if (symtab.link >= raw_sections.size()) {
            continue;
        }

        const auto& strtab = raw_sections[symtab.link];
        if (!range_valid(image.bytes, strtab.offset, strtab.size)) {
            continue;
        }

        const std::uint32_t entsize = symtab.entsize ? symtab.entsize : sizeof(Elf32Sym);
        if (entsize < sizeof(Elf32Sym)) {
            continue;
        }

        for (std::uint32_t off = 0; off + sizeof(Elf32Sym) <= symtab.size; off += entsize) {
            const auto raw = read_struct<Elf32Sym>(image.bytes,
                                                  static_cast<std::size_t>(symtab.offset) + off);
            ElfSymbol symbol;
            symbol.name = read_c_string(image.bytes, strtab.offset, strtab.size, raw.name);
            symbol.value = raw.value;
            symbol.size = raw.size;
            symbol.info = raw.info;
            symbol.other = raw.other;
            symbol.section_index = raw.shndx;
            image.symbols.push_back(std::move(symbol));
        }
    }

    return image;
}

} // namespace dcrecomp
