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
  auto env_var = std::getenv("HHK_SYMBOL_TABLE");
  if (!env_var)
    return;
  auto env_var_len = std::strlen(env_var);
  if (env_var_len != sizeof(std::uintptr_t) * 2)
    return;
  auto ptr = std::strtoul(env_var, nullptr, 16);
  if (!ptr)
    return;

  if (ptr == reinterpret_cast<std::uintptr_t>(addresses))
    return;

  std::free(reinterpret_cast<void *>(ptr));
}

static jmp_buf jump_buffer;
static int exit_code;

static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context *context,
                                           void *ctx) {
  auto ip = reinterpret_cast<void *>(_Unwind_GetIP(context));
  auto sp = reinterpret_cast<void *>(_Unwind_GetCFA(context));
  auto &y = *reinterpret_cast<int *>(ctx);
  Debug_Printf(5, y++, true, 0, "IP: %p, SP: %p\n", ip, sp);
  if (y > 20)
    return _URC_END_OF_STACK;
  return _URC_NO_REASON;
}
static void _custom_terminate() {
  int y = 3;
  _Unwind_Backtrace(unwind_callback, &y);
  LCD_Refresh();
  Debug_WaitKey();
  std::exit(EXIT_FAILURE);
}
static __attribute__((constructor(10))) void init_unwind() {
  std::set_terminate(_custom_terminate);
}

extern "C" void *setup() {
  cas_setup();
  if (setjmp(jump_buffer) != 0) {
    __libc_fini_array();
    clearLastSymbolMap();
    cas_cleanup();
    static char
        buf[sizeof(unsigned long) * 2 + 1]; // use data section instead of stack
    ultohexstr(static_cast<unsigned long>(exit_code), buf);
    buf[sizeof(buf) - 1] = '\0';
    return GUI_DisplayMessageBox_Internal(0, "run.bin", "return", buf, 0,
                                          false);
  }
  setupEnvironment();
  __libc_init_array();
  int ret;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
  ret = main();
#pragma GCC diagnostic pop
  exit(ret);
}

extern "C" void _exit_address(int status) __asm__("__exit_address")
    __attribute__((noreturn));
extern "C" void _exit_address(int status) {
  exit_code = status;
  longjmp(jump_buffer, 1);
}