#include "loader.hpp"
#include <bit>
#include <cstdio>
#include <cstring>
#include <elf.h>
#include <memory>
#include <stdexcept>
#include <unistd.h>

void ELFLoader::init_ehdr() {
  if (ehdr)
    return;

  ehdr = std::make_unique<Elf32_Ehdr>();
  std::fseek(file, 0, SEEK_SET);
  if (std::fread(ehdr.get(), sizeof(Elf32_Ehdr), 1, file) != 1)
    throw std::runtime_error("Failed to read ehdr");

  if (std::memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    throw std::runtime_error("Invalid ELF magic");

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
    throw std::runtime_error("Invalid ELF class");

  if (ehdr->e_ident[EI_DATA] !=
      (std::endian::native == std::endian::little ? ELFDATA2LSB : ELFDATA2MSB))
    throw std::runtime_error("Invalid ELF data encoding");

  if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    throw std::runtime_error("Invalid ELF version");

  // dont care about OS ABI
}

void ELFLoader::init_phdrs() {
  if (phdrs)
    return;

  init_ehdr();

  if (ehdr->e_phnum == 0) {
    phdrs = std::make_unique<Elf32_Phdr[]>(0);
    return;
  }

  if (ehdr->e_phentsize != sizeof(Elf32_Phdr))
    throw std::runtime_error("Invalid ELF program header size");

  phdrs = std::make_unique<Elf32_Phdr[]>(ehdr->e_phnum);
  if (std::fseek(file, ehdr->e_phoff, SEEK_SET) != 0 ||
      std::fread(phdrs.get(), sizeof(Elf32_Phdr), ehdr->e_phnum, file) !=
          ehdr->e_phnum)
    throw std::runtime_error("Failed to read program headers");

  // sanity check
  for (auto phdr = phdrs.get(); phdr < phdrs.get() + ehdr->e_phnum; phdr++)
    switch (phdr->p_type) {
    case PT_NULL:
    case PT_LOAD:
    // case PT_DYNAMIC:
    // case PT_INTERP:
    case PT_NOTE:
    case PT_PHDR:
      break;
    default:
      throw std::runtime_error("Invalid program header type");
    }
}

void ELFLoader::init_notes() {
  if (notes)
    return;

  init_phdrs();

  notes = std::make_unique<std::forward_list<std::unique_ptr<ELFNotes>>>();

  for (auto phdr = phdrs.get(); phdr < phdrs.get() + ehdr->e_phnum; phdr++) {
    if (phdr->p_type != PT_NOTE)
      continue;
    if (phdr->p_filesz != phdr->p_memsz)
      throw std::runtime_error("Mismatched note size");

    if (phdr->p_filesz == 0)
      continue;

    if (std::fseek(file, phdr->p_offset, SEEK_SET) != 0)
      throw std::runtime_error("Failed to seek to note header");

    auto remaining_size = phdr->p_filesz;

    while (remaining_size > 0) {
      if (remaining_size < sizeof(Elf32_Nhdr))
        throw std::runtime_error("Invalid note header size");

      Elf32_Nhdr header;
      if (std::fread(&header, sizeof(Elf32_Nhdr), 1, file) != 1)
        throw std::runtime_error("Failed to read note header");

      remaining_size -= sizeof(Elf32_Nhdr);

      const auto actual_name_size =
          header.n_namesz +
          (sizeof(Elf32_Word) - header.n_namesz % sizeof(Elf32_Word)) %
              sizeof(Elf32_Word);
      const auto actual_desc_size =
          header.n_descsz +
          (sizeof(Elf32_Word) - header.n_descsz % sizeof(Elf32_Word)) %
              sizeof(Elf32_Word);
      if (remaining_size < actual_name_size + actual_desc_size)
        throw std::runtime_error("Invalid note content size");

      auto name = std::make_unique<char[]>(header.n_namesz);
      auto desc = std::make_unique<std::uint8_t[]>(header.n_descsz);

      if (std::fread(name.get(), header.n_namesz, 1, file) != 1 ||
          std::fseek(file, actual_name_size - header.n_namesz, SEEK_CUR) != 0)
        throw std::runtime_error("Failed to read note name");

      if (std::fread(desc.get(), header.n_descsz, 1, file) != 1 ||
          std::fseek(file, actual_desc_size - header.n_descsz, SEEK_CUR) != 0)
        throw std::runtime_error("Failed to read note desc");

      remaining_size -= actual_name_size + actual_desc_size;

      notes->emplace_front(std::make_unique<ELFNotes>(
          std::unique_ptr<const char[]>(
              reinterpret_cast<const char *>(name.release())),
          std::unique_ptr<const std::uint8_t[]>(
              reinterpret_cast<const std::uint8_t *>(desc.release())),
          header.n_namesz, header.n_descsz, header.n_type));
    }
  }
}
