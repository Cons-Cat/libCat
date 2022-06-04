#pragma once

namespace cat {

// Forward declarations.
template <typename Abi, typename T>
requires(is_same<typename Abi::Scalar, T>) class alignas(
    Abi::alignment.raw) Simd;

template <typename Abi, typename T>
class alignas(Abi::alignment.raw) SimdMask;

// `Avx2Abi` is a SIMD ABI that can be expected to work on most reasonable
// x86-64 build target.
template <typename T>
struct Avx2Abi {
    using Scalar = T;

    // Produce a similar `Avx2Abi` for type `U`.
    template <typename U>
    using MakeAbi = Avx2Abi<U>;

    Avx2Abi() = delete;

    static constexpr ssize size = 32;
    static constexpr ssize lanes = size / ssizeof<T>();
    static constexpr ssize alignment = 32;
};

template <typename T>
using Avx2Simd = Simd<Avx2Abi<T>, T>;

template <typename T>
using Avx2SimdMask = SimdMask<Avx2Abi<T>, T>;

template <typename T>
[[nodiscard]] auto testc(SimdMask<Avx2Abi<T>, T> const left,
                         SimdMask<Avx2Abi<T>, T> const right) -> int4;

template <typename T>
[[nodiscard]] auto testz(SimdMask<Avx2Abi<T>, T> const left,
                         SimdMask<Avx2Abi<T>, T> const right) -> int4;

template <typename T>
[[nodiscard]] auto all_of(SimdMask<Avx2Abi<T>, T> const mask) -> bool;

template <typename T>
[[nodiscard]] auto any_of(SimdMask<Avx2Abi<T>, T> const mask) -> bool;

template <typename T>
[[nodiscard]] auto move_mask(Avx2Simd<T> const vector) -> int4;

template <typename T>
[[nodiscard]] auto move_mask(SimdMask<Avx2Abi<T>, T> const mask) -> int4;

}  // namespace cat
