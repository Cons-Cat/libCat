#pragma once

#include <cat/simd>

// TODO: Use a SIMD vector concept.
auto cat::shuffle(auto in_vector, auto mask) {
    decltype(in_vector) out_vector;
    __builtin_shuffle(in_vector, out_vector, mask);
    return out_vector;
}
