// MIT License
// Copyright (c) 2022 James Edward Anhalt III - https://github.com/jeaiii/itoa
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
// "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <cat/format>

#define P(T)                                                                   \
    T, '0', T, '1', T, '2', T, '3', T, '4', T, '5', T, '6', T, '7', T, '8', T, \
        '9'
static const cat::detail::pair_jeaiii s_pairs[] = {
    P('0'), P('1'), P('2'), P('3'), P('4'),
    P('5'), P('6'), P('7'), P('8'), P('9')};

#define W(N, I) *(cat::detail::pair_jeaiii*)&b[N] = s_pairs[I]
#define A(N)                                                 \
    t = (uint8::raw_type(1) << (32 + N / 5 * N * 53 / 16)) / \
            uint4::raw_type(1e##N) +                         \
        1 + N / 6 - N / 8,                                   \
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

#define LG(F)                                                       \
    (u < 100          ? u < 10 ? F(0) : F(1)                        \
              : u < 1000000 ? u < 10000    ? u < 1000 ? F(2) : F(3) \
                                 : u < 100000 ? F(4)                \
                                           : F(5)                   \
              : u < 100000000 ? u < 10000000 ? F(6) : F(7)          \
              : u < 1000000000 ? F(8)                               \
                      : F(9))

char* cat::detail::u32toa_jeaiii(uint4::raw_type u, char* b) {
    uint8::raw_type t;
    return LG(LZ);
}

char* cat::detail::i32toa_jeaiii(int4::raw_type i, char* b) {
    uint4::raw_type u = i < 0 ? *b++ = '-', 0 - uint4::raw_type(i) : i;
    uint8::raw_type t;
    return LG(LZ);
}

char* cat::detail::u64toa_jeaiii(uint8::raw_type n, char* b) {
    uint4::raw_type u;
    uint8::raw_type t;

    if (uint4::raw_type(n >> 32) == 0) {
        return u = uint4::raw_type(n), LG(LZ);
    }

    uint8::raw_type a = n / 100000000;

    if (uint4::raw_type(a >> 32) == 0) {
        u = uint4::raw_type(a);
        LG(LN);
    } else {
        u = uint4::raw_type(a / 100000000);
        LG(LN);
        u = a % 100000000;
        LN(7);
    }

    u = n % 100000000;
    return LZ(7);
}

char* cat::detail::i64toa_jeaiii(int8::raw_type i, char* b) {
    uint8::raw_type n = i < 0 ? *b++ = '-', 0 - uint8::raw_type(i) : i;
    return u64toa_jeaiii(n, b);
}

// TODO: This code is ill-formed.
/*
consteval auto cat::to_string(int4 value) -> string {
    if (value == 0) {
        return "0";
    }

    int4::raw_type mut_value = value.raw;
    // An `int4` string never exceeds 11 characters, excluding the
    // null-terminator.
    char* p_string = new char[11];
    for (iword::raw_type i = 0; i < 11; ++i) {
        p_string[i] = '\0';
    }

    iword::raw_type i = 0;

    // Handle a negative sign.
    if (value < 0) {
        p_string[0] = '-';
        i = 1;
        mut_value = -mut_value;
    }

    // Fill `p_string` with digits one-by-one.
    while (mut_value != 0) {
        int4::raw_type remainder = mut_value % 10;
        p_string[i] =
            (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
        mut_value = mut_value / 10;
        ++i;
    }

    return p_string;
}
*/
