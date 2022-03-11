// -*- mode: c++ -*-
// vim: set ft=cpp:

// TODO: Improve this function's name.
// TODO: Perfect forwarding.
template <uint1 Mask>
auto simd::cmp_implicit_str_count(auto const& vector_1, auto const& vector_2)
    -> bool {
    static_assert(meta::is_same_v<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistric128(vector_1.value, vector_2.value, Mask);
}
