#define __MISC_VISIBLE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int _ATTRIBUTE((__format__(__printf__, 3, 0)))
    _vfiprintf_r(struct _reent *r, FILE *f, const char *c, __VALIST v) {
  return _vfprintf_r(r, f, c, v);
}

_ATTRIBUTE((__noreturn__))
void __assert(const char *, int, const char *) { abort(); }

_ATTRIBUTE((__noreturn__))
void __assert_func(const char *, int, const char *, const char *) {
  abort();
}
