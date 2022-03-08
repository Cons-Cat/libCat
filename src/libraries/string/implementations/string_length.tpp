// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <string>

/* T is the return type of `string_length()`. It may be signed or
 * unsigned. This function requires SSE4.2 */
template <typename T>
auto std::string_length(meta::string auto&& p_string) -> T {
    // TODO: Align pointers.
    T result = 0;
    charx16* p_memory = simd::p_string_to_p_vector<16>((p_string));
    constexpr charx16 zeros = simd::set_zeros<charx16>();

    while (true) {
        charx16 data;
        data = *p_memory;
        constexpr uint1 mask = simd::SIDD_UBYTE_OPS |
                               simd::SIDD_CMP_EQUAL_EACH |
                               simd::SIDD_LEAST_SIGNIFICANT;
        if (simd::cmp_implicit_str_c<mask>(data, zeros)) {
            int4 const index = simd::cmp_implicit_str_i<mask>(data, zeros);
            return result + index;
        }
        p_memory++;
        result += sizeof(uint1x16);
        return result;
    }
    // This point unreachable because the function would segfault first.
    __builtin_unreachable();
}
