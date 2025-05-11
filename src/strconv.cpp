#include "strconv.hpp"
#include <cstddef>

void ultohexstr(unsigned long value, char *str) {
  str += sizeof(unsigned long) * 2 - 1;
  for (std::size_t i = 0; i < sizeof(unsigned long) * 2;
       value >>= 4, str--, i++)
    *str = ((value & 0b1111) >= 10 ? 'A' + (value & 0b1111) - 10
                                   : '0' + (value & 0b1111));
}
