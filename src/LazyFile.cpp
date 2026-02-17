#include "LazyFile.hpp"

#include <cstring>
#include <string>

using namespace std::string_literals;

LazyFile::LazyFile(const char *path, const char *mode)
    : path(path), mode(mode) {}

LazyFile::shared_type LazyFile::acquire() {
  if (auto shared = file.lock())
    return shared;

  const auto raw = std::fopen(path, mode);
  if (!raw)
    throw std::runtime_error("Coudld not open "s + path + "(" + mode +
                             "): " + std::strerror(errno));

  auto shared = shared_type(raw, [](std::FILE *const file) {
    if (!file)
      return;
    if (std::fclose(file) != 0)
      throw std::runtime_error("Could not close file: "s +
                               std::strerror(errno));
  });

  file = shared;
  return shared;
}
