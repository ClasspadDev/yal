#ifndef __clang__
#ifndef PREP_FILE
#error PREP_FILE is not defined!
#endif
#endif

#include <stdint.h>
#include <stddef.h>
#include "addresses.h"

const uint8_t addresses[] = {
#ifndef __clang__
#embed PREP_FILE
#else
0
#endif
};

const size_t addresses_size = sizeof(addresses);
