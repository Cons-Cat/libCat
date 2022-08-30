#pragma once

// This header forward-declares Jeaiii's int-to-string conversion functions for
// use by my string formatting. The implementations, and their copyright notice,
// are located in `../../implementations/itoa_jeaiii.cpp`.

namespace cat::detail {

struct PairJeaiii {
    char t, o;
};

#define P(T)                                                                   \
    T, '0', T, '1', T, '2', T, '3', T, '4', T, '5', T, '6', T, '7', T, '8', T, \
        '9'
static const cat::detail::PairJeaiii s_pairs[] = {
    P('0'), P('1'), P('2'), P('3'), P('4'),
    P('5'), P('6'), P('7'), P('8'), P('9')};

#define W(N, I) *(cat::detail::PairJeaiii*)&b[N] = s_pairs[I]
#define A(N)                                                                \
    t = (uint8::Raw(1) << (32 + N / 5 * N * 53 / 16)) / uint4::Raw(1e##N) + \
        1 + N / 6 - N / 8,                                                  \
    t *= u, t >>= N / 5 * N * 53 / 16, t += N / 6 * 4, W(0, t >> 32)
#define S(N) b[N] = char(uint8::Raw(10) * uint4::Raw(t) >> 32) + '0'
#define D(N) t = uint8::Raw(100) * uint4::Raw(t), W(N, t >> 32)

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
//#define LZ(N) &(L##N, b[N + 1] = '\0')

#define LG(F)                                                       \
    (u < 100          ? u < 10 ? F(0) : F(1)                        \
              : u < 1000000 ? u < 10000    ? u < 1000 ? F(2) : F(3) \
                                 : u < 100000 ? F(4)                \
                                           : F(5)                   \
              : u < 100000000 ? u < 10000000 ? F(6) : F(7)          \
              : u < 1000000000 ? F(8)                               \
                      : F(9))

auto u32toa_jeaiii(uint4::Raw i, char* b) -> char*;
auto i32toa_jeaiii(int4::Raw i, char* b) -> char*;
auto u64toa_jeaiii(uint8::Raw i, char* b) -> char*;
auto i64toa_jeaiii(int8::Raw i, char* b) -> char*;

}  // namespace cat::detail
