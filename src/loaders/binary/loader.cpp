#include "loader.hpp"
#include "sdk/calc/calc.h"
#include <cstdint>
#include <cstdio>
#include <unistd.h>

void BinaryLoader::load() {
  std::fseek(file, 0, SEEK_END);
  const auto size = std::ftell(file);

  // execution_address = load_address in this format
  execution_address = nullptr;

  if (size < 0x10 - 1) {
    std::fseek(file, 0x0C, SEEK_SET);
    std::fread(&execution_address, sizeof(execution_address), 1, file);
  }

  if (reinterpret_cast<std::uintptr_t>(execution_address) >> (3 * 8) != 0x8c) {
    execution_address = reinterpret_cast<void *>(0x8cff0000);
  }

  std::fseek(file, 0, SEEK_SET);
  std::fread(execution_address, size, 1, file);
}

int BinaryLoader::execute() {
  const auto pathlen = std::char_traits<char>::length(path.get());
  auto argv0 = new char[pathlen + 1];
  std::copy_n(path.get(), pathlen, argv0);
  argv0[pathlen] = '\0';
  char *argv[] = {argv0};
  calcExit();
  auto ret = reinterpret_cast<int (*)(int argc, char **argv, char **envp)>(
      execution_address)(sizeof(argv) / sizeof(*argv), argv, environ);
  calcInit();
  return ret;
}

void BinaryLoader::unload() {}
