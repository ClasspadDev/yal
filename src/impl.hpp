#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#define __UCONCAT(a, b) #a b
#define _UCONCAT(a, b) __UCONCAT(a, b)
#define UCONCAT(b) _UCONCAT(__USER_LABEL_PREFIX__, b)

static const auto safe_guard = reinterpret_cast<const char *>(0x814fffe0);
static constexpr std::size_t safe_guard_size = 16;

static inline bool check_safe_guard(const char *const other) {
  return std::strncmp(safe_guard, other, safe_guard_size) == 0;
}

extern "C" bool relink_sdk(const std::uint8_t *mapping, std::size_t size);
extern "C" void relocate_sdk();

extern "C" bool relink_libc(const std::uint8_t *mapping,
                            std::size_t size) __asm__(UCONCAT("___relink"));
extern "C" void relocate_libc() __asm__(UCONCAT("___relocate_hhk"));

#undef UCONCAT
#undef _UCONCAT
#undef __UCONCAT