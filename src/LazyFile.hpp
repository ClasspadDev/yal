#pragma once
#include <cstdio>
#include <memory>
#include <utility>
#include <variant>

class LazyFile {
public:
  using shared_type = std::shared_ptr<std::FILE>;
  explicit LazyFile(const char *path, const char *mode);
  explicit LazyFile(void *memory, size_t size, const char *mode);
  shared_type acquire();

private:
  const std::variant<const char *, std::pair<void *, size_t>> backing;
  const char *const mode;
  std::weak_ptr<std::FILE> file;
};
