#include "loader.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <elf.h>
#include <memory>
#include <stdexcept>
#include <unistd.h>

void ELFLoader::load() {
  const auto shared = file.acquire();
  const auto file = shared.get();

  init_phdrs();

  for (const auto &phdr : std::span(phdrs.get(), ehdr->e_phnum)) {
    if (phdr.p_type != PT_LOAD)
      continue;

    if (phdr.p_memsz < phdr.p_filesz)
      throw std::runtime_error("Illegal sizes");

    if (phdr.p_flags & ~(PF_X | PF_W | PF_R))
      throw std::runtime_error("Unknown flags");

    if (!(phdr.p_flags & (PF_X | PF_W | PF_R)))
      continue; // not accessible -> can skip

    if (phdr.p_filesz) {
      if (std::fseek(file, phdr.p_offset, SEEK_SET) != 0 ||
          std::fread(reinterpret_cast<void *>(phdr.p_vaddr), phdr.p_filesz, 1,
                     file) != 1)
        throw std::runtime_error("Failed to read contents");
    }

    std::memset(reinterpret_cast<void *>(phdr.p_vaddr + phdr.p_filesz), 0,
                phdr.p_memsz - phdr.p_filesz);

    for (std::size_t i = 0; i < phdr.p_memsz; i += 32)
      __asm__ volatile("ocbwb @%0" : : "r"(phdr.p_vaddr + i));
    __asm__ volatile(
        "ocbwb @%0"
        :
        : "r"(reinterpret_cast<void *>(phdr.p_vaddr + phdr.p_memsz - 1)));

    if (phdr.p_flags & PF_X) {
      for (std::size_t i = 0; i < phdr.p_memsz; i += 32)
        __asm__ volatile("icbi @%0" : : "r"(phdr.p_vaddr + i));
      __asm__ volatile(
          "icbi @%0"
          :
          : "r"(reinterpret_cast<void *>(phdr.p_vaddr + phdr.p_memsz - 1)));
    }
  }
}

void ELFLoader::unload() {}

int ELFLoader::execute() {
  init_ehdr();

  if (!ehdr->e_entry)
    throw std::runtime_error("No execution address set");

  switch (ehdr->e_type) {
  case ET_EXEC:
  case ET_CORE:
    // case ET_DYN:
    break;
  default:
    throw std::runtime_error("Invalid type for execution");
  }

  if (ehdr->e_machine != EM_SH)
    throw std::runtime_error("Invalid processor");

  const auto argv0 = getPath();
  char *argv[] = {argv0.get(), nullptr};

  const auto ret = reinterpret_cast<int (*)(int argc, char **argv, char **envp)>(
      ehdr->e_entry)(std::size(argv) - 1, argv, environ);

  return ret;
}
