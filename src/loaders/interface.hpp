#pragma once

#include "../strlit.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <forward_list>
#include <memory>
#include <sdk/os/file.h>
#include <stdexcept>
#include <string>
#include <type_traits>

class Executable {
public:
  virtual void load() = 0;
  virtual void unload() = 0;
  virtual int execute() = 0;
  virtual std::unique_ptr<char[]> getName() { return getPath(); }
  virtual std::unique_ptr<char[]> getPath() = 0;
  virtual std::unique_ptr<char[]> getVersion() { return nullptr; }
  virtual std::unique_ptr<char[]> getAuthor() { return nullptr; }
  virtual std::unique_ptr<char[]> getDescription() { return nullptr; }
  virtual ~Executable() = default;
};

template <typename E, typename Enable = void>
  requires std::is_base_of_v<Executable, E> && (!std::is_same_v<E, Executable>)
struct discover_impl {
  static void run(const std::forward_list<std::unique_ptr<Executable>> &list);
  discover_impl() = delete;
};

template <StringLiteral EXT> class FileBasedExecutable : public Executable {
protected:
  const std::unique_ptr<const char[]> path;
  FILE *const file;

public:
  template <typename CharT>
  FileBasedExecutable(const CharT *path)
      : path(([path] {
          auto len = std::char_traits<CharT>::length(path);
          auto uptr = std::make_unique<char[]>(len + 1);
          std::copy_n(path, len, uptr.get());
          uptr[len] = '\0';
          return uptr;
        })()),
        file(std::fopen(this->path.get(), "rb")) {
    if (!file) {
      throw std::runtime_error("Failed to open file");
    }
  }
  ~FileBasedExecutable() override { std::fclose(file); }
  std::unique_ptr<char[]> getPath() override {
    const auto len = std::char_traits<char>::length(path.get());
    auto path = std::make_unique<char[]>(len + 1);
    std::copy_n(this->path.get(), len, path.get());
    path[len] = '\0';
    return path;
  }

  static constexpr auto ext = EXT;
};

template <typename T>
struct discover_impl<
    T, std::enable_if_t<std::is_base_of_v<FileBasedExecutable<T::ext>, T> &&
                        !std::is_same_v<FileBasedExecutable<T::ext>, T>>> {
  static void run(std::forward_list<std::unique_ptr<Executable>> &list) {
    int findHandle = 0;
    File_FindInfo findInfo{};
    int ret;
    constexpr auto prefix = "\\fls0\\";
    constexpr auto prefix_len = std::char_traits<char>::length(prefix);
    constexpr auto pattern = ([prefix, prefix_len] {
      std::array<char_const16_t, prefix_len + T::ext.size + 1> arr{};
      std::copy_n(prefix, prefix_len, arr.data());
      arr[prefix_len] = '*';
      std::copy_n(T::ext.value, T::ext.size - 1, arr.data() + prefix_len + 1);
      return arr;
    })();
    auto filename = ([] {
      std::array<char_const16_t, 100> arr{};
      std::copy_n(prefix, prefix_len, arr.data());
      return arr;
    })();

    ret = File_FindFirst(pattern.data(), &findHandle,
                         filename.data() + prefix_len, &findInfo);

    while (ret >= File_Error::FILE_OK) {
      if (findInfo.type != File_FindInfo::EntryTypeFile) {
        continue;
      }
      list.emplace_front(std::make_unique<T>(filename.data()));
      ret = File_FindNext(findHandle, filename.data() + prefix_len, &findInfo);
    }
    File_FindClose(findHandle);
  }
  discover_impl() = delete;
};

template <typename T>
using discover =
    discover_impl<T>; // hide away the ugly second template argument
