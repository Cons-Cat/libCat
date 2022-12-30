// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/simd_sse42.hpp>

// TODO: Improve this function's name.
// TODO: Perfect forwarding the function parameters.
// TODO: Handle the type of `char` automatically to make this type-safe.
// `control_mask` must be constant-evaluated for the intrinsic to work
// correctly.
template <x64::string_control control_mask>
constexpr auto x64::compare_implicit_length_strings(auto const& vector_1,
                                                    auto const& vector_2)
    -> bool {
    static_assert(cat::is_same<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistric128(
        vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask));
}
