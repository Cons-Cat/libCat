#pragma once

namespace cat {

// Forward declarations.
template <typename Abi, typename T>
    requires(is_same<typename Abi::Scalar, T>)
class alignas(Abi::alignment.raw) Simd;

template <typename Abi, typename T>
class alignas(Abi::alignment.raw) SimdMask;

// `Sse2Abi` is a SIMD ABI that can be expected to work on any x86-64 build
// target.
template <typename T>
struct Sse42Abi {
    using Scalar = T;

    // Produce a similar `Sse42Abi` for type `U`.
    template <typename U>
    using MakeAbi = Sse42Abi<U>;

    Sse42Abi() = delete;

    static constexpr ssize size = 16;
    static constexpr ssize lanes = size / ssizeof(T);
    static constexpr usize alignment = 16u;
};

template <typename T>
using Sse42Simd = Simd<Sse42Abi<T>, T>;

template <typename T>
using Sse42SimdMask = SimdMask<Sse42Abi<T>, T>;

}  // namespace cat
