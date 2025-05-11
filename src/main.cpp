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
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sdk/calc/calc.h>
#include <sdk/os/debug.h>
#include <sdk/os/gui.h>
#include <sdk/os/lcd.h>
#include <stdexcept>

void do_override() {
  const auto guard_len = std::strlen(safe_guard);

  constexpr char prefix[] = "\\fls0\\addresses.";
  constexpr char suffix[] = ".override"; // incl \0

  // char path[/*preifx: */14 + /*guard: */guard_len + /*suffix: */9 + /*null:
  // */1] = "addresses."; // VLA is only a C99 feature
  const auto path = reinterpret_cast<char *>(
      alloca(sizeof(prefix) - 1 + guard_len + sizeof(suffix)));
  std::memcpy(path, prefix, sizeof(prefix) - 1);
  std::memcpy(path + sizeof(prefix) - 1, safe_guard, guard_len);
  std::memcpy(path + sizeof(prefix) - 1 + guard_len, suffix, sizeof(suffix));

  const auto fd = std::fopen(path, "rb");
  if (!fd)
    return;

  std::fseek(fd, 0, SEEK_END);
  const auto file_size = std::ftell(fd);
  std::fseek(fd, 0, SEEK_SET);

  const auto guard = reinterpret_cast<char *>(alloca(guard_len + 1));
  if (std::fread(guard, sizeof(char) * (guard_len + 1), 1, fd) != 1)
    throw std::runtime_error("Failed to read guard");
  if (!check_safe_guard(guard))
    throw std::runtime_error("Invalid guard");

  const unsigned long size = file_size - (guard_len + 1) + (addresses_size);
  const auto buf = new uint8_t[size];
  if (!buf)
    throw std::runtime_error("Failed to allocate override");
  std::memcpy(buf, addresses, addresses_size);

  const auto bytes_to_read = file_size - (guard_len + 1);
  if (std::fread(buf + addresses_size, bytes_to_read, 1, fd) != 1)
    throw std::runtime_error("Failed to read override");

  if (std::fclose(fd) != 0)
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

  return;
}

int main() {
  do_override();

  std::unique_ptr<Executable> choosen;
  {
    std::forward_list<std::unique_ptr<Executable>> list;
    // discover<BinaryLoader>::run(list);
    discover<ELFLoader>::run(list);
    choosen = do_gui(list);
  }

  if (!choosen)
    return 0;

  choosen->load();
  auto ret = choosen->execute();
  choosen->unload();

  return ret;
}