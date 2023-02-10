#pragma once

#include <cat/detail/simd_avx2_fwd.hpp>

#include <cat/bitset>
#include <cat/simd>

namespace x64 {

template <typename T>
[[nodiscard]]
auto testc(cat::simd_mask<avx2_abi<T>, T> left,
           cat::simd_mask<avx2_abi<T>, T> right) -> cat::int4 {
    if constexpr (cat::is_same<T, float>) {
        return __builtin_ia32_vtestcps256(left.raw, right.raw);
    } else if constexpr (cat::is_same<T, double>) {
        return __builtin_ia32_vtestcpd256(left.raw, right.raw);
    } else {
        auto left_raw =
            reinterpret_cast<avx2_simd<long long int>::raw_type>(left.raw);
        auto right_raw =
            reinterpret_cast<avx2_simd<long long int>::raw_type>(right.raw);
        return __builtin_ia32_ptestc256(left_raw, right_raw);
    }
}

template <typename T>
[[nodiscard]]
auto testz(cat::simd_mask<avx2_abi<T>, T> left,
           cat::simd_mask<avx2_abi<T>, T> right) -> cat::int4 {
    if constexpr (cat::is_same<T, float>) {
        return __builtin_ia32_vtestzps256(left.raw, right.raw);
    } else if constexpr (cat::is_same<T, double>) {
        return __builtin_ia32_vtestzpd256(left.raw, right.raw);
    } else {
        auto left_raw =
            reinterpret_cast<avx2_simd<long long int>::raw_type>(left.raw);
        auto right_raw =
            reinterpret_cast<avx2_simd<long long int>::raw_type>(right.raw);
        return __builtin_ia32_ptestz256(left_raw, right_raw);
    }
}

}  // namespace x64

namespace cat {

// Implementation of `simd_all_of()` for AVX2.
template <typename T>
[[nodiscard]]
auto simd_all_of(simd_mask<x64::avx2_abi<T>, T> mask) -> bool {
    return testc(mask, mask == mask) != 0;
}

// Implementation of `simd_any_of()` for AVX2.
template <typename T>
[[nodiscard]]
auto simd_any_of(simd_mask<x64::avx2_abi<T>, T> mask) -> bool {
    return testz(mask, mask == mask) == 0;
}

// Implementation of `simd_to_bitset` for AVX2.
template <typename T>
// TODO: Support larger integrals than 1.
    requires(is_floating_point<T> || (sizeof(T) == 1))
[[nodiscard]]
auto simd_to_bitset(simd_mask<x64::avx2_abi<T>, T> mask) -> bitset<32> {
    if constexpr (is_same<T, float>) {
        // Create a bitmask from the most significant bit of every `float` in
        // this vector.
        return bitset<32>::from(
            make_unsigned(__builtin_ia32_movmskps256(mask.raw)));
    } else if constexpr (is_same<T, double>) {
        // Create a bitmask from the most significant bit of every `double` in
        // this vector.
        return bitset<32>::from(
            make_unsigned(__builtin_ia32_movmskpd256(mask.raw)));
    } else {
        // Create a bitmask from the most significant bit of every byte in this
        // vector.
        return bitset<32>::from(
            make_unsigned(__builtin_ia32_pmovmskb256(mask.raw)));
    }
}

}  // namespace cat
