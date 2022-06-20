#pragma once
#include <cat/format>

// This header forward-declares Jeaiii's int-to-string conversion functions for
// use by my string formatting. The implementations, and their copyright notice,
// are located in `../../implementations/itoa_jeaiii.cpp`.

namespace cat::detail {

struct PairJeaiii {
    char t, o;
};

char* u32toa_jeaiii(uint4::Raw i, char* b);
char* i32toa_jeaiii(int4::Raw i, char* b);
char* u64toa_jeaiii(uint8::Raw i, char* b);
char* i64toa_jeaiii(int8::Raw i, char* b);

}  // namespace cat::detail
