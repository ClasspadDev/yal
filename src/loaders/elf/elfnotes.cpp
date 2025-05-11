#include "elfnotes.hpp"
#include <cstddef>
#include <cstdint>
#include <elf.h>
#include <memory>
#include <span>

ELFNotes::ELFNotes(std::unique_ptr<const char[]> name,
                   std::unique_ptr<const std::uint8_t[]> desc,
                   std::size_t size_name, std::size_t size_desc,
                   Elf32_Word type)
    : type(type), data_name(std::move(name)), data_desc(std::move(desc)),
      size_name(size_name), size_desc(size_desc) {};

std::span<const char> ELFNotes::getName() const {
  return std::span<const char>(data_name.get(), size_name);
}

std::span<const std::uint8_t> ELFNotes::getDesc() const {
  return std::span<const std::uint8_t>(data_desc.get(), size_desc);
}
