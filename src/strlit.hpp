#pragma once

#include <algorithm>
#include <cstddef>

template <std::size_t N, typename CharT = char> struct StringLiteral {
  constexpr StringLiteral(const CharT (&str)[N]) { std::copy_n(str, N, value); }

  CharT value[N];
  static constexpr auto size = N;
};
