#include <stdint.h>
#include <stddef.h>
#include <string.h>

void *setup();
void *start() __attribute__((section(".bootup")));

static void fix_pc(size_t diff) __attribute__((noinline));

extern char _executable_start;
extern char end;

#define run_address ((void *)&_executable_start)
#define end_address ((void *)&end)
#define length ((uintptr_t)end_address - (uintptr_t)run_address)

#define early_(fun) typeof(&(fun)) early_##fun = (typeof(&(fun)))((uintptr_t)(fun) + (diff));

void label(); // do NOT call

void *start() {
  void *pcrel_label_address;
  #ifndef __clang__
  __asm__ (
    ".balign 4\n\t"
    "mova %cc1,r0\n\t"
    ".balign 4\n"
    "%cc1:\n\t"
    "mov%M0 r0,%0"
    : "=rm" (pcrel_label_address)
    : ":" (label) // clangd doesnt know this
    : "r0"
  );
  #else
  pcrel_label_address = nullptr;
  #endif
  size_t diff = (uintptr_t)pcrel_label_address - (uintptr_t)&label;
  void *load_address = (void *)((uintptr_t)run_address + diff);

  early_(memcpy);
  early_(fix_pc);

  //copy up to load (includes us); only works if load_address >= run_address
  early_memcpy(run_address, load_address, diff > length ? length : diff);

  //fixup pc
  early_fix_pc(diff);

  if ((uintptr_t)end_address > (uintptr_t)load_address) {
    if ((uintptr_t)memmove >= (uintptr_t)load_address - 256) {  // we can not know the size of memmove; approximate with room
      early_(memmove);

      if ((uintptr_t)early_memmove <= (uintptr_t)end_address) { // do we need to override early_memmove?
        //copy up to early_memove (includes memmove if diff >= sizeof(memmove))
        early_memmove(load_address, (void *)((uintptr_t)load_address + (uintptr_t)diff), (uintptr_t)memmove - (uintptr_t)run_address);

        //copy rest
        memmove((void *)(uintptr_t)early_memmove, (void *)((uintptr_t)early_memmove + (uintptr_t)diff), length - (((uintptr_t)memmove - (uintptr_t)run_address) + diff));
      } else {
        //copy rest
        early_memmove(load_address, (void *)((uintptr_t)load_address + (uintptr_t)diff), length - diff);
      }
    } else {
      //copy rest
      memmove(load_address, (void *)((uintptr_t)load_address + (uintptr_t)diff), length - diff);
    }
  }
  return setup();
}

static void fix_pc(size_t diff) { // make sure this is noinline
  uintptr_t return_pointer;
  __asm__ ("sts pr,%0" : "=r" (return_pointer));
  return_pointer -= diff;
  __asm__ __volatile__ ("lds %0,pr" : : "r" (return_pointer));
  // return will activate the change
}
