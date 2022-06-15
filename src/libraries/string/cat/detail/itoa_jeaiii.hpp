#pragma once
#include <cat/string>

namespace cat::detail {

struct PairJeaiii {
    char t, o;
};

char* u32toa_jeaiii(uint4::Raw i, char* b);
char* i32toa_jeaiii(int4::Raw i, char* b);
char* u64toa_jeaiii(uint8::Raw i, char* b);
char* i64toa_jeaiii(int8::Raw i, char* b);

}  // namespace cat::detail
