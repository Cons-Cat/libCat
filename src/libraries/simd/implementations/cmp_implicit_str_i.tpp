// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// TODO: Prove that this template parameter is actually more performant than a
// function parameter.
template <simd::StringControl Mask>
constexpr auto simd::compare_implicit_length_strings_return_index(
    auto const& vector_1, auto const& vector_2) -> int4 {
    static_assert(meta::is_same_v<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistri128(vector_1.value, vector_2.value,
                                       static_cast<unsigned char>(Mask));
}
