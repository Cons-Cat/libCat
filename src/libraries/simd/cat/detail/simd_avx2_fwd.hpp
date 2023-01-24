#pragma once

namespace cat {

// Forward declarations.
template <typename abi_type, typename T>
    requires(cat::is_same<typename abi_type::scalar_type, T>)
class alignas(abi_type::alignment.raw) simd;

template <typename abi_type, typename T>
class alignas(abi_type::alignment.raw) simd_mask;

}  // namespace cat

namespace x64 {

// `avx2_abi` is a SIMD ABI that can be expected to work on most reasonable
// x86-64 build target.
template <typename T>
struct avx2_abi {
    using scalar_type = T;

    // Produce a similar `avx2_abi` for type `U`.
    template <typename U>
    using make_abi_type = avx2_abi<U>;

    avx2_abi() = delete;

    static constexpr ssize size = 32;
    static constexpr ssize lanes = size / ssizeof(T);
    static constexpr usize alignment = 32u;
};

template <typename T>
using avx2_simd = cat::simd<avx2_abi<T>, T>;

template <typename T>
using avx2_simd_mask = cat::simd_mask<avx2_abi<T>, T>;

template <typename T>
[[nodiscard]]
auto testc(cat::simd_mask<avx2_abi<T>, T> left,
           cat::simd_mask<avx2_abi<T>, T> right) -> int4;

template <typename T>
[[nodiscard]]
auto testz(cat::simd_mask<x64::avx2_abi<T>, T> left,
           cat::simd_mask<x64::avx2_abi<T>, T> right) -> int4;

}  // namespace x64

namespace cat {
template <typename T>
[[nodiscard]]
auto all_of(simd_mask<x64::avx2_abi<T>, T> mask) -> bool;

template <typename T>
[[nodiscard]]
auto any_of(simd_mask<x64::avx2_abi<T>, T> mask) -> bool;

template <ssize bits_count>
    requires(bits_count > 0)
class bitset;

template <typename T>
[[nodiscard]]
auto simd_to_bitset(simd_mask<x64::avx2_abi<T>, T> mask) -> bitset<32>;

}  // namespace cat
