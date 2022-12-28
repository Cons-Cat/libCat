// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/simd>

// `control_mask` must be constant-evaluated for the intrinsic to work
// correctly.
template <cat::string_control control_mask>
constexpr auto cat::compare_implicit_length_strings_return_index(
    auto const& vector_1, auto const& vector_2) -> int4 {
    static_assert(cat::is_same<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistri128(
        vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask));
}
