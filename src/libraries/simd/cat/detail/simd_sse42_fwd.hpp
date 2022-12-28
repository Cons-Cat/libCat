#pragma once

namespace cat {

// Forward declarations.
template <typename abi_type, typename T>
    requires(is_same<typename abi_type::scalar_type, T>)
class alignas(abi_type::alignment.raw) simd;

template <typename abi_type, typename T>
class alignas(abi_type::alignment.raw) simd_mask;

// `Sse2abi_type` is a SIMD ABI that can be expected to work on any x86-64 build
// target.
template <typename T>
struct sse42_abi {
    using scalar_type = T;

    // Produce a similar `sse42_abi` for type `U`.
    template <typename U>
    using make_abi_type = sse42_abi<U>;

    sse42_abi() = delete;

    static constexpr ssize size = 16;
    static constexpr ssize lanes = size / ssizeof(T);
    static constexpr usize alignment = 16u;
};

template <typename T>
using sse42_mask = simd<sse42_abi<T>, T>;

template <typename T>
using sse42_simd_mask = simd_mask<sse42_abi<T>, T>;

}  // namespace cat
