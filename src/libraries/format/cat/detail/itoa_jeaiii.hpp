#pragma once

// This header forward-declares Jeaiii's int-to-string conversion functions for
// use by my string formatting. The implementations, and their copyright notice,
// are located in `../../implementations/itoa_jeaiii.cpp`.

// NOLINTBEGIN

namespace cat::detail {

struct pair_jeaiii {
   char t, o;
};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#define P(T)                                                                  \
   T, '0', T, '1', T, '2', T, '3', T, '4', T, '5', T, '6', T, '7', T, '8', T, \
      '9'
static cat::detail::pair_jeaiii const s_pairs[] = {
   P('0'), P('1'), P('2'), P('3'), P('4'),
   P('5'), P('6'), P('7'), P('8'), P('9')};

#define W(N, I) *(cat::detail::pair_jeaiii*)&b[N] = s_pairs[I]
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
// if you want to '\0' terminate
// #define LZ(N) &(L##N, b[N + 1] = '\0')

#define LG(F)                                                             \
   (u < 100             ? u < 10 ? F(0) : F(1)                            \
                : u < 1'000'000 ? u < 10'000    ? u < 1'000 ? F(2) : F(3) \
                                     : u < 100'000 ? F(4)                 \
                                                : F(5)                    \
                : u < 100'000'000 ? u < 10'000'000 ? F(6) : F(7)          \
                : u < 1'000'000'000 ? F(8)                                \
                        : F(9))

auto
u32toa_jeaiii(uint4::raw_type i, char* p_b) -> char*;
auto
i32toa_jeaiii(int4::raw_type i, char* p_b) -> char*;
auto
u64toa_jeaiii(uint8::raw_type i, char* p_b) -> char*;
auto
i64toa_jeaiii(int8::raw_type i, char* p_b) -> char*;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace cat::detail

// NOLINTEND
