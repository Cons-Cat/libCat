#pragma once

#include <cat/detail/simd_avx2_fwd.hpp>

#include <cat/meta>
#include <cat/simd>

namespace cat {

template <typename T>
[[nodiscard]] auto testc(SimdMask<Avx2Abi<T>, T> left,
                         SimdMask<Avx2Abi<T>, T> right) -> int4 {
    if constexpr (is_same<T, float>) {
        return __builtin_ia32_vtestcps256(left.raw, right.raw);
    } else if constexpr (is_same<T, double>) {
        return __builtin_ia32_vtestcpd256(left.raw, right.raw);
    } else {
        auto left_raw =
            reinterpret_cast<Avx2Simd<long long int>::Raw>(left.raw);
        auto right_raw =
            reinterpret_cast<Avx2Simd<long long int>::Raw>(right.raw);
        return __builtin_ia32_ptestc256(left_raw, right_raw);
    }
}

template <typename T>
[[nodiscard]] auto testz(SimdMask<Avx2Abi<T>, T> left,
                         SimdMask<Avx2Abi<T>, T> right) -> int4 {
    if constexpr (is_same<T, float>) {
        return __builtin_ia32_vtestzps256(left.raw, right.raw);
    } else if constexpr (is_same<T, double>) {
        return __builtin_ia32_vtestzpd256(left.raw, right.raw);
    } else {
        auto left_raw =
            reinterpret_cast<Avx2Simd<long long int>::Raw>(left.raw);
        auto right_raw =
            reinterpret_cast<Avx2Simd<long long int>::Raw>(right.raw);
        return __builtin_ia32_ptestz256(left_raw, right_raw);
    }
}

// Implementation of `all_of()` for AVX2.
template <typename T>
[[nodiscard]] auto all_of(SimdMask<Avx2Abi<T>, T> mask) -> bool {
    return testc(mask, mask == mask) != 0;
}

// Implementation of `any_of()` for AVX2.
template <typename T>
[[nodiscard]] auto any_of(SimdMask<Avx2Abi<T>, T> mask) -> bool {
    return testz(mask, mask == mask) == 0;
}

// TODO: Return a `Bitset`.
// Implementation of `move_mask` for AVX2.
template <typename T>
[[nodiscard]] auto move_mask(Avx2Simd<T> vector) -> int4 {
    if constexpr (is_same<T, float>) {
        // Create a bitmask from the most significant bit of every `float` in
        // this vector.
        return __builtin_ia32_movmskps256(vector.raw);
    } else if constexpr (is_same<T, double>) {
        // Create a bitmask from the most significant bit of every `double` in
        // this vector.
        return __builtin_ia32_movmskpd256(vector.raw);
    } else {
        // Create a bitmask from the most significant bit of every byte in this
        // vector.
        return __builtin_ia32_pmovmskb256(vector.raw);
    }
}

// TODO: Return a `Bitset`.
// Implementation of `move_mask` for AVX2.
template <typename T>
[[nodiscard]] auto move_mask(SimdMask<Avx2Abi<T>, T> mask) -> int4 {
    if constexpr (is_same<T, float>) {
        // Create a bitmask from the most significant bit of every `float` in
        // this vector.
        return __builtin_ia32_movmskps256(mask.raw);
    } else if constexpr (is_same<T, double>) {
        // Create a bitmask from the most significant bit of every `double` in
        // this vector.
        return __builtin_ia32_movmskpd256(mask.raw);
    } else {
        // Create a bitmask from the most significant bit of every byte in this
        // vector.
        return __builtin_ia32_pmovmskb256(mask.raw);
    }
}

}  // namespace cat
