#pragma once

#include "../strlit.hpp"
#include "LazyFile.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <forward_list>
#include <memory>
#include <sdk/os/debug.h>
#include <sdk/os/bfile.h>
#include <sdk/os/lcd.h>
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

template <typename T> struct discover;

template <typename E>
  requires(!std::is_base_of_v<Executable, E>) || std::is_same_v<E, Executable>
struct discover<E> {
  discover() = delete;
};

template <StringLiteral EXT> class FileBasedExecutable : public Executable {
protected:
  const std::unique_ptr<const char[]> path;
  LazyFile file;

public:
  explicit FileBasedExecutable(const char *path)
      : path([path] {
        const auto len = std::char_traits<char>::length(path) + 1;
        auto ptr = std::make_unique_for_overwrite<char[]>(len);
        std::copy_n(path, len, ptr.get());
        return ptr;
      }()),
        file(this->path.get(), "rb") {}
  explicit FileBasedExecutable(void * memory, size_t size, const char *display_path)
      : path(display_path), file(memory, size, "rb") {}
  std::unique_ptr<char[]> getPath() override {
    const auto len = std::char_traits<char>::length(path.get()) + 1;
    auto path = std::make_unique<char[]>(len);
    std::copy_n(this->path.get(), len, path.get());
    return path;
  }

  static constexpr auto ext = EXT;
};

template <typename T>
  requires std::is_base_of_v<FileBasedExecutable<T::ext>, T> &&
           (!std::is_same_v<FileBasedExecutable<T::ext>, T>)
struct discover<T> {
  static void run(std::forward_list<std::unique_ptr<Executable>> &list) {
    int findHandle = 0;
    BFile_FindInfo findInfo{};
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

    auto ret = BFile_FindFirst(pattern.data(), &findHandle,
                              filename.data() + prefix_len, &findInfo);

    while (ret >= BFILE_OK) {
      do {
        if (findInfo.type != BFile_EntryType::BFILE_FILE) {
          continue;
        }
        if (filename[prefix_len] == '.') {
          continue;
        }
        auto len =
            std::char_traits<char_const16_t>::length(filename.data()) + 1;
        auto uptr = std::make_unique<char[]>(len * 2);
        SMEM_BFilePathToFuguePath(filename.data(), uptr.get());
        std::unique_ptr<T> ptr;
        try {
          ptr = std::make_unique<T>(uptr.get());
        } catch (const std::exception &e) {
          Debug_Printf(1, 3, false, 0, "Error processing file \"%s\"",
                       uptr.get());
          Debug_Printf(1, 5, false, 0, "%s", e.what());
          Debug_Printf(3, 8, false, 0, "Press any key to continue");
          LCD_Refresh();
          Debug_WaitKey();
          continue;
        }
        list.emplace_front(std::move(ptr));
      } while (false);
      ret = BFile_FindNext(findHandle, filename.data() + prefix_len, &findInfo);
    }
    BFile_FindClose(findHandle);
  }
  discover() = delete;
};
