// for setenv from <stdlib.h>
#define _POSIX_C_SOURCE 200112L
#include "addresses.h"
#include "gui.hpp"
#include "impl.hpp"
#include "loaders/binary/loader.hpp"
#include "loaders/elf/loader.hpp"
#include "loaders/interface.hpp"
#include "strconv.hpp"
#include <alloca.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sdk/calc/calc.h>
#include <stdexcept>
#include <string>

using namespace std::string_literals;

void do_override() {
  const size_t guard_len = static_cast<std::decay_t<decltype(addresses)>>(
                               std::memchr(addresses, 0, addresses_size)) -
                           addresses;

  const auto path = "\\fls0\\addresses."s +
                    reinterpret_cast<const char *>(addresses) + ".override";

  const auto file = std::fopen(path.c_str(), "rb");
  if (!file)
    return;

  if (std::fseek(file, 0, SEEK_END) != 0)
    throw std::runtime_error("Failed to seek to end of override");
  const auto file_size = std::ftell(file);
  if (std::fseek(file, 0, SEEK_SET) != 0)
    throw std::runtime_error("Failed to seek to start of override");

  const auto guard = static_cast<char *>(alloca(guard_len));
  if (std::fread(guard, sizeof(char) * guard_len, 1, file) != 1)
    throw std::runtime_error("Failed to read guard");
  if (!check_safe_guard(guard))
    throw std::runtime_error("Invalid guard");

  const unsigned long size = file_size - guard_len + addresses_size;
  const auto buf = new uint8_t[size];
  if (!buf)
    throw std::runtime_error("Failed to allocate override");
  std::memcpy(buf, addresses, addresses_size);

  if (std::fread(buf + addresses_size, file_size - guard_len, 1, file) != 1)
    throw std::runtime_error("Failed to read override");

  if (std::fclose(file) != 0)
    throw std::runtime_error("Failed to close override");

  if (!relink_sdk(buf, size))
    throw std::runtime_error("Failed to relink sdk");

  if (!relink_libc(buf, size))
    throw std::runtime_error("Failed to relink libc");

  {
    char cbuf[sizeof(std::uintptr_t) * 2 + 1];
    ultohexstr(reinterpret_cast<std::uintptr_t>(buf), cbuf);
    cbuf[sizeof(cbuf) - 1] = '\0';
    setenv("HHK_SYMBOL_TABLE", cbuf, 1);
  }
  {
    char cbuf[sizeof(std::size_t) * 2 + 1];
    ultohexstr(reinterpret_cast<std::size_t>(buf), cbuf);
    cbuf[sizeof(cbuf) - 1] = '\0';
    setenv("HHK_SYMBOL_TABLE_LEN", cbuf, 1);
  }
}

int main() {
  do_override();

  std::unique_ptr<Executable> chosen;
  {
    std::forward_list<std::unique_ptr<Executable>> list;
    if (std::strcmp(reinterpret_cast<const char *>(addresses),
                    "2017.0512.1515") == 0) // 2000 aka original hhk2
      discover<BinaryLoader>::run(list);
    discover<ELFLoader>::run(list);
    chosen = do_gui(list);
  }

  if (!chosen)
    return 0;

  calcExit();
  chosen->load();
  const auto ret = chosen->execute();
  chosen->unload();
  calcInit();

  return ret;
}