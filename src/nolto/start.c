#include <stdint.h>
#include <stddef.h>
#include <string.h>

void *setup();
void *start() __attribute__((section(".bootup")));

static void fix_pc() __attribute__((noinline));

#define run_address ((void *)0x8CFD0000)
#define load_address ((void *)0x8CFE0000)
#define app_address ((void *)0x8CFF0000)
#define diff ((uintptr_t)load_address - (uintptr_t)run_address)
#define length ((uintptr_t)app_address - (uintptr_t)run_address)

#define early_(fun) typeof(&fun) early_##fun = (typeof(&fun))((uintptr_t)&fun + diff);

void *start() {
  early_(memcpy);
  early_(fix_pc);

  //copy up to load (includes us)
  early_memcpy(run_address, load_address, diff);

  //fixup pc
  early_fix_pc();

  if ((uintptr_t)memmove >= (uintptr_t)run_address - 256) {  // we can not know the size of memmove; approximate with room
    early_(memmove);

    //copy up to early_memove (includes memmove)
    early_memmove(load_address, (void *)((uintptr_t)load_address + (uintptr_t)diff), (uintptr_t)early_memmove - (uintptr_t)run_address);

    //copy rest
    memmove((void *)(uintptr_t)early_memmove, (void *)((uintptr_t)early_memmove + (uintptr_t)diff), length - ((uintptr_t)early_memmove - (uintptr_t)run_address));
  } else {
    memmove(load_address, (void *)((uintptr_t)load_address + (uintptr_t)diff), length - diff);
  }

  return setup();
}

static void fix_pc() { // make sure this is noinline
  uintptr_t return_pointer;
  __asm__ ("sts pr,%0" : "=r" (return_pointer));
  return_pointer -= diff;
  __asm__ __volatile__ ("lds %0,pr" : : "r" (return_pointer));
  // return will activate the change
}
