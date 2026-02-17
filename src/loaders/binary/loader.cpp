#include "loader.hpp"
#include <cstdio>

void BinaryLoader::load() {
  const auto shared = file.acquire();
  const auto file = shared.get();

  std::fseek(file, 0, SEEK_END);
  const size_t size = std::ftell(file);

  // execution_address = load_address in this format
  execution_address = nullptr;

  if (size >= 0x10) {
    if (std::fseek(file, 0x0C, SEEK_SET) != 0 &&
        std::fread(&execution_address, sizeof(execution_address), 1, file) != 1)
      throw std::runtime_error("Failed to read execution address");
  }

  if (reinterpret_cast<std::uintptr_t>(execution_address) >> (3 * 8) != 0x8c) {
    execution_address = reinterpret_cast<void *>(0x8cff0000);
  }

  if (std::fseek(file, 0, SEEK_SET) != 0 &&
      std::fread(execution_address, size, 1, file) != 1)
    throw std::runtime_error("Failed to read executable");
  for (std::size_t i = 0; i < size; i += 32) {
    __asm__ volatile("ocbwb @%0"
                     :
                     : "r"(reinterpret_cast<uintptr_t>(execution_address) + i));
    __asm__ volatile("icbi @%0"
                     :
                     : "r"(reinterpret_cast<uintptr_t>(execution_address) + i));
  }
  __asm__ volatile(
      "ocbwb @%0"
      :
      : "r"(reinterpret_cast<uintptr_t>(execution_address) + size - 1));
  __asm__ volatile(
      "icbi @%0"
      :
      : "r"(reinterpret_cast<uintptr_t>(execution_address) + size - 1));
}

int BinaryLoader::execute() {
  /*const auto pathlen = std::char_traits<char>::length(path.get());
  auto argv0 = new char[pathlen + 1];
  std::copy_n(path.get(), pathlen, argv0);
  argv0[pathlen] = '\0';
  char *argv[] = {argv0};*/
  /*auto ret = reinterpret_cast<int (*)(int argc, char **argv, char **envp)>(
      execution_address)(sizeof(argv) / sizeof(*argv), argv, environ);*/
  reinterpret_cast<void (*)()>(execution_address)();
  return 0;
}

void BinaryLoader::unload() {}
