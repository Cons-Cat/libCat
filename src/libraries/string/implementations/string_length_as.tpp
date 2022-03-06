// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <string>

/* `T` is the return type of `string_length()`. It may be signed or unsigned.
 * There exists `simd::string_length_as<>()`. */
template <typename T>
constexpr auto std::string_length_as(char8_t const* p_string) -> T {
    T result = 0;
    while (p_string[result] != '\0') {
        result++;
    }
    return result;
}

// TODO: Use [[gnu::]] multiversioning attributes to support SSE4.1.
/* T is the return type of string_length_as(). It may be signed or
 * unsigned. This function requires SSE4.2 */
template <typename T>
auto simd::string_length_as(char8_t const* p_string) -> T {
    // TODO: Align pointers.
    T result = 0;
    charx16* p_memory = simd::p_string_to_p_vector<16>(p_string);
    constexpr charx16 zeros = simd::set_zeros<charx16>();

    while (true) {
        charx16 data;
        data = *p_memory;
        constexpr uint1 mask =
            SIDD_UBYTE_OPS | SIDD_CMP_EQUAL_EACH | SIDD_LEAST_SIGNIFICANT;
        if (simd::cmp_implicit_str_c<mask>(data, zeros)) {
            int1 const index = simd::cmp_implicit_str_i<mask>(data, zeros);
            return result + index;
        }
        p_memory++;
        result += sizeof(uint1x16);
        return result;
    }
    // This point unreachable because the function would segfault first.
    __builtin_unreachable();
}
