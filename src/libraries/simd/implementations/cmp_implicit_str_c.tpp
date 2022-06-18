// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// TODO: Prove that this template parameter is actually more performant than a
// function parameter.
// TODO: Perfect forwarding the function parameters.
template <cat::StringControl Mask>
constexpr auto cat::compare_implicit_length_strings(auto const& vector_1,
                                                    auto const& vector_2)
    -> bool1 {
    static_assert(cat::is_same_v<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistric128(vector_1.value, vector_2.value,
                                        static_cast<unsigned char>(Mask));
}
