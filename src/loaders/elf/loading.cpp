#include "loader.hpp"
#include <cstdio>
#include <cstring>
#include <elf.h>
#include <memory>
#include <sdk/calc/calc.h>
#include <stdexcept>
#include <unistd.h>

void ELFLoader::load() {
  init_phdrs();

  for (auto phdr = phdrs.get(); phdr < phdrs.get() + ehdr->e_phnum; phdr++) {
    if (phdr->p_type != PT_LOAD)
      continue;

    if (phdr->p_memsz < phdr->p_filesz)
      throw std::runtime_error("Illegal sizes");

    if (phdr->p_flags & ~(PF_X | PF_W | PF_R))
      throw std::runtime_error("Unknown flags");

    if (!(phdr->p_flags & (PF_X | PF_W | PF_R)))
      continue; // not accessible -> can skip

    if (phdr->p_filesz) {
      if (std::fseek(file, phdr->p_offset, SEEK_SET) != 0 ||
          std::fread(reinterpret_cast<void *>(phdr->p_vaddr), phdr->p_filesz, 1,
                     file) != 1)
        throw std::runtime_error("Failed to read contents");
    }

    std::memset(reinterpret_cast<void *>(phdr->p_vaddr + phdr->p_filesz), 0,
                phdr->p_memsz - phdr->p_filesz);

    if (phdr->p_vaddr <
            reinterpret_cast<std::uintptr_t>(vram + width * height * 2) &&
        phdr->p_vaddr + phdr->p_memsz >=
            reinterpret_cast<std::uintptr_t>(vram + width * height))
      in_vbak = true;
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

  const auto pathlen = std::char_traits<char>::length(path.get());
  auto argv0 = new char[pathlen + 1];
  std::copy_n(path.get(), pathlen, argv0);
  argv0[pathlen] = '\0';
  char *argv[] = {argv0};

  if (!in_vbak)
    calcExit();
  auto ret = reinterpret_cast<int (*)(int argc, char **argv, char **envp)>(
      ehdr->e_entry)(sizeof(argv) / sizeof(*argv), argv, environ);
  if (!in_vbak)
    calcInit();

  return ret;
}
