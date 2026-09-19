#define _POSIX_C_SOURCE 200809L // for fmemopen
#include <stdio.h>

#include "LazyFile.hpp"

#include <cstring>
#include <utility>
#include <variant>

using namespace std::string_literals;

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

LazyFile::LazyFile(const char *path, const char *mode)
    : backing(path), mode(mode) {}

LazyFile::LazyFile(void *memory, size_t size, const char *mode)
    : backing(std::pair(memory, size)), mode(mode) {}

LazyFile::shared_type LazyFile::acquire() {
  if (auto shared = file.lock())
    return shared;

  auto mode = this->mode;
  auto shared = std::visit(
      overloaded{
          [mode](const char *path) {
            const auto raw = std::fopen(path, mode);
            if (!raw)
              throw std::runtime_error("Coudld not open "s + path + "(" + mode +
                                       "): " + std::strerror(errno));

            return shared_type(raw, [](std::FILE *const file) {
              if (!file)
                return;
              if (std::fclose(file) != 0)
                throw std::runtime_error("Could not close file: "s +
                                         std::strerror(errno));
            });
          },
          [mode](std::pair<void *, size_t> memory) {
            const auto raw = fmemopen(memory.first, memory.second, mode);
            if (!raw)
              throw std::runtime_error("Coudld not open memfile ("s + mode +
                                       "): " + std::strerror(errno));

            return shared_type(raw, [](std::FILE *const file) {
              if (!file)
                return;
              if (std::fclose(file) != 0)
                throw std::runtime_error("Could not close memfile: "s +
                                         std::strerror(errno));
            });
          }},
      backing);

  file = shared;
  return shared;
}
