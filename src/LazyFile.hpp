#pragma once
#include <cstdio>
#include <memory>

class LazyFile {
public:
  using shared_type = std::shared_ptr<std::FILE>;
  explicit LazyFile(const char *path, const char *mode);
  shared_type acquire();

private:
  const char *const path;
  const char *const mode;
  std::weak_ptr<std::FILE> file;
};
