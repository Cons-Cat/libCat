#pragma once

#include <cat/arithmetic>

// MIT License Copyright (c) 2022 James Edward Anhalt III -
// https://github.com/jeaiii/itoa Permission is hereby granted, free of charge,
// to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following
// conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
// "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// These are Jeaiii's `int`-to-string conversion functions, made `constexpr` by
// writing each digit pair as two `char` stores instead of one `pair_jeaiii`
// store. Clang optimizes this the same.

// These C patterns flag clang-tidy.
// NOLINTBEGIN

namespace cat::detail {

struct pair_jeaiii {
   char t, o;
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"

#define P(T)                                                                  \
   T, '0', T, '1', T, '2', T, '3', T, '4', T, '5', T, '6', T, '7', T, '8', T, \
      '9'
inline constexpr pair_jeaiii s_pairs[] = {
   P('0'), P('1'), P('2'), P('3'), P('4'),
   P('5'), P('6'), P('7'), P('8'), P('9'),
};

#define W(N, I) (b[N] = s_pairs[I].t, b[N + 1] = s_pairs[I].o)
#define A(N)                                              \
   t = (uint8::raw_type(1) << (32 + N / 5 * N * 53 / 16)) \
          / uint4::raw_type(1e##N)                        \
       + 1 + N / 6 - N / 8,                               \
   t *= u, t >>= N / 5 * N * 53 / 16, t += N / 6 * 4, W(0, t >> 32)
#define S(N) b[N] = char(uint8::raw_type(10) * uint4::raw_type(t) >> 32) + '0'
#define D(N) t = uint8::raw_type(100) * uint4::raw_type(t), W(N, t >> 32)

#define L0 b[0] = char(u) + '0'
#define L1 W(0, u)
#define L2 A(1), S(2)
#define L3 A(2), D(2)
#define L4 A(3), D(2), S(4)
#define L5 A(4), D(2), D(4)
#define L6 A(5), D(2), D(4), S(6)
#define L7 A(6), D(2), D(4), D(6)
#define L8 A(7), D(2), D(4), D(6), S(8)
#define L9 A(8), D(2), D(4), D(6), D(8)

#define LN(N) (L##N, b += N + 1)
#define LZ LN
// If you want to '\0' terminate #define LZ(N) &(L##N, b[N + 1] = '\0').

#define LG(F)                                                     \
   (u < 100             ? u < 10 ? F(0) : F(1)                    \
    : u < 1'000'000     ? u < 10'000    ? u < 1'000 ? F(2) : F(3) \
                          : u < 100'000 ? F(4)                    \
                                        : F(5)                    \
    : u < 100'000'000   ? u < 10'000'000 ? F(6) : F(7)            \
    : u < 1'000'000'000 ? F(8)                                    \
                        : F(9))

constexpr auto
u32toa_jeaiii(uint4::raw_type u, char* b [[clang::lifetimebound]]) -> char* {
   uint8::raw_type t;
   return LG(LZ);
}

constexpr auto
i32toa_jeaiii(int4::raw_type i, char* b [[clang::lifetimebound]]) -> char* {
   uint4::raw_type u = i < 0 ? *b++ = '-', 0 - uint4::raw_type(i) : i;
   uint8::raw_type t;
   return LG(LZ);
}

constexpr auto
u64toa_jeaiii(uint8::raw_type n, char* b [[clang::lifetimebound]]) -> char* {
   uint4::raw_type u;
   uint8::raw_type t;

   if (uint4::raw_type(n >> 32) == 0) {
      return u = uint4::raw_type(n), LG(LZ);
   }

   uint8::raw_type a = n / 100'000'000;

   if (uint4::raw_type(a >> 32) == 0) {
      u = uint4::raw_type(a);
      LG(LN);
   } else {
      u = uint4::raw_type(a / 100'000'000);
      LG(LN);
      u = a % 100'000'000;
      LN(7);
   }

   u = n % 100'000'000;
   return LZ(7);
}

constexpr auto
i64toa_jeaiii(int8::raw_type i, char* b [[clang::lifetimebound]]) -> char* {
   uint8::raw_type n = i < 0 ? *b++ = '-', 0 - uint8::raw_type(i) : i;
   return u64toa_jeaiii(n, b);
}

#undef P
#undef W
#undef A
#undef S
#undef D
#undef L0
#undef L1
#undef L2
#undef L3
#undef L4
#undef L5
#undef L6
#undef L7
#undef L8
#undef L9
#undef LN
#undef LZ
#undef LG

#pragma clang diagnostic pop

}  // namespace cat::detail

// NOLINTEND
