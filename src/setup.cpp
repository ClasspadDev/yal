#define _POSIX_C_SOURCE 200112L
#include "addresses.h"
#include "strconv.hpp"
#include <csetjmp>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <sdk/os/debug.h>
#include <sdk/os/gui.h>
#include <sdk/os/lcd.h>
#include <unistd.h>
#include <unwind.h>

int main();

static char hhk_var[] = "HHK_SYMBOL_TABLE=________";
static char hhk_len_var[] = "HHK_SYMBOL_TABLE_LEN=________";

static char *my_env[] = {hhk_var, hhk_len_var, nullptr};

char **environ = my_env;

extern "C" void __libc_init_array();
extern "C" void __libc_fini_array();

extern "C" void cas_setup();
extern "C" void cas_cleanup();

void setupEnvironment() {
  ultohexstr(reinterpret_cast<std::uintptr_t>(addresses), hhk_var + 17);
  ultohexstr(addresses_size, hhk_len_var + 21);
}

void clearLastSymbolMap() {
  const auto env_var = std::getenv("HHK_SYMBOL_TABLE");
  if (!env_var)
    return;
  if (std::strlen(env_var) != sizeof(std::uintptr_t) * 2)
    return;
  const auto ptr = std::strtoul(env_var, nullptr, 16);
  if (!ptr)
    return;

  if (ptr == reinterpret_cast<std::uintptr_t>(addresses))
    return;

  std::free(reinterpret_cast<void *>(ptr));
}

static jmp_buf jump_buffer;
static int exit_code;

extern "C" void *setup() {
  cas_setup();
  if (setjmp(jump_buffer) != 0) {
    __libc_fini_array();
    clearLastSymbolMap();
    cas_cleanup();
    static char
        buf[sizeof(unsigned long) * 2 + 1]; // use bss section instead of stack
    ultohexstr(static_cast<unsigned long>(exit_code), buf);
    buf[sizeof(buf) - 1] = '\0';
    return GUI_DisplayMessageBox_Internal(0, "run.bin", "return", buf, 0,
                                          false);
  }
  setupEnvironment();
  __libc_init_array();
  int ret;
  try {
    try {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
      ret = main();
#pragma GCC diagnostic pop
    } catch (const std::exception &e) {
      Debug_Printf(5, 3, true, 0, "%s", e.what());
      throw;
    }
  } catch (...) {
    int index = 0;
    _Unwind_Backtrace([](_Unwind_Context *context, void *ctx) {
      const auto ip = reinterpret_cast<void *>(_Unwind_GetIP(context));
      auto &index = *static_cast<int *>(ctx);
      Debug_Printf(5, index + 4, true, 0, "%i %p\n", index, ip);
      if (index++ > 20)
        return _URC_NORMAL_STOP;
      return _URC_NO_REASON;
    }, &index);
    LCD_Refresh();
    Debug_WaitKey();
    ret = EXIT_FAILURE;
  }
  exit(ret);
}

extern "C" void _exit_address(int status) __asm__("__exit_address")
    __attribute__((noreturn));
extern "C" void _exit_address(const int status) {
  exit_code = status;
  longjmp(jump_buffer, 1);
}