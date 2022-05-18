// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/simd>

// TODO: Improve this function's name.
// TODO: Perfect forwarding the function parameters.
// TODO: Handle the type of `char` automatically to make this type-safe.
// `control_mask` must be constant-evaluated for the intrinsic to work
// correctly.
template <simd::StringControl control_mask>
constexpr auto simd::compare_implicit_length_strings(auto const& vector_1,
                                                     auto const& vector_2)
    -> bool1 {
    static_assert(meta::is_same<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistric128(
        vector_1.value, vector_2.value,
        static_cast<unsigned char>(control_mask));
}
