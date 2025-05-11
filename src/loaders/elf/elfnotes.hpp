#pragma once

#include <cstddef>
#include <cstdint>
#include <elf.h>
#include <memory>
#include <span>

class ELFNotes {
public:
  ELFNotes(std::unique_ptr<const char[]> name,
           std::unique_ptr<const std::uint8_t[]> desc, std::size_t size_name,
           std::size_t size_desc, Elf32_Word type);

  std::span<const char> getName() const;
  std::span<const std::uint8_t> getDesc() const;
  const Elf32_Word type;

private:
  std::unique_ptr<const char[]> data_name;
  std::unique_ptr<const std::uint8_t[]> data_desc;
  std::size_t size_name;
  std::size_t size_desc;
};
