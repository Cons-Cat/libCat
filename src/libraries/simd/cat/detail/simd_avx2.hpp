#pragma once

#include <cat/detail/simd_avx2_fwd.hpp>

#include <cat/bitset>
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

// Implementation of `simd_all_of()` for AVX2.
template <typename T>
[[nodiscard]] auto simd_all_of(SimdMask<Avx2Abi<T>, T> mask) -> bool {
    return testc(mask, mask == mask) != 0;
}

// Implementation of `simd_any_of()` for AVX2.
template <typename T>
[[nodiscard]] auto simd_any_of(SimdMask<Avx2Abi<T>, T> mask) -> bool {
    return testz(mask, mask == mask) == 0;
}

// Implementation of `simd_to_bitset` for AVX2.
template <typename T>
    // TODO: Support larger integrals than 1.
    requires(is_floating_point<T> || (sizeof(T) == 1))
[[nodiscard]] auto simd_to_bitset(SimdMask<Avx2Abi<T>, T> mask) -> Bitset<32> {
    if constexpr (is_same<T, float>) {
        // Create a bitmask from the most significant bit of every `float` in
        // this vector.
        return Bitset<32>::from(
            make_unsigned(__builtin_ia32_movmskps256(mask.raw)));
    } else if constexpr (is_same<T, double>) {
        // Create a bitmask from the most significant bit of every `double` in
        // this vector.
        return Bitset<32>::from(
            make_unsigned(__builtin_ia32_movmskpd256(mask.raw)));
    } else {
        // Create a bitmask from the most significant bit of every byte in this
        // vector.
        return Bitset<32>::from(
            make_unsigned(__builtin_ia32_pmovmskb256(mask.raw)));
    }
}

}  // namespace cat
