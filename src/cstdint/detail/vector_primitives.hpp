#pragma once

#include <type_traits>

namespace std::detail {
// TODO: Replace with a general Array type?

// Size is signed to make arithmetic simpler.
// This vector is not packed, because it is optimized for scalar operations.
template <typename T, isize Size>
struct PrimitiveVector {
    static constexpr isize size = Size;
    T data[Size];

    // Do not zero-out data by default.
    PrimitiveVector<T, Size>() = default;

    // TODO: Perfect forwarding.
    template <typename U, isize USize>
    explicit PrimitiveVector<T, Size>(
        const PrimitiveVector<U, USize>& copy_vector) requires(Size >= USize) {
        // TODO: This should work for any type where sizeof(U) <= sizeof(T).
        if constexpr (is_same<T, U>()) {
            for (isize i = 0; i < (Size >= USize ? USize : Size); i++) {
                data[i] = copy_vector[i];
            }
            /* If copy_vector is smaller than this vector, zero-out the
             * remaining elements. */
            if constexpr (Size >= USize) {
                for (isize j = 0; j < Size; j++) {
                    data[j] = 0;
                }
            }
        }
    }
};
}  // namespace std::detail

/* Primitive vectors of size 3 are not rounded up to 4.
 * This is to make them more useful for scalar operations.
 * When storing them in SIMD vectors, the 4th data element will still be
 * accounted for. */

// Vectors of up to 32 8-bit integers are supported by AVX2.
using i8x2 = std::detail::PrimitiveVector<i8, 2>;
using i8x3 = std::detail::PrimitiveVector<i8, 3>;
using i8x4 = std::detail::PrimitiveVector<i8, 4>;
using i8x8 = std::detail::PrimitiveVector<i8, 8>;
using i8x16 = std::detail::PrimitiveVector<i8, 16>;
using i8x32 = std::detail::PrimitiveVector<i8, 32>;
using u8x2 = std::detail::PrimitiveVector<u8, 2>;
using u8x3 = std::detail::PrimitiveVector<u8, 3>;
using u8x4 = std::detail::PrimitiveVector<u8, 4>;
using u8x8 = std::detail::PrimitiveVector<u8, 8>;
using u8x16 = std::detail::PrimitiveVector<u8, 16>;
using u8x32 = std::detail::PrimitiveVector<u8, 32>;

// Vectors of up to 16 16-bit integers are supported by AVX2.
using i16x2 = std::detail::PrimitiveVector<i16, 2>;
using i16x3 = std::detail::PrimitiveVector<i16, 3>;
using i16x4 = std::detail::PrimitiveVector<i16, 4>;
using i16x16 = std::detail::PrimitiveVector<i16, 16>;
using u16x2 = std::detail::PrimitiveVector<u16, 2>;
using u16x3 = std::detail::PrimitiveVector<u16, 3>;
using u16x4 = std::detail::PrimitiveVector<u16, 4>;

// Vectors of up to 8 32-bit integers are supported by AVX2.
using i32x2 = std::detail::PrimitiveVector<i32, 2>;
using i32x3 = std::detail::PrimitiveVector<i32, 3>;
using i32x4 = std::detail::PrimitiveVector<i32, 4>;
using i32x8 = std::detail::PrimitiveVector<i32, 8>;
using u32x2 = std::detail::PrimitiveVector<u32, 2>;
using u32x3 = std::detail::PrimitiveVector<u32, 3>;
using u32x4 = std::detail::PrimitiveVector<u32, 4>;
using u32x8 = std::detail::PrimitiveVector<u32, 8>;

// Vectors of up to 8 32-bit floats are supported by AVX2.
using f32x2 = std::detail::PrimitiveVector<f32, 2>;
using f32x3 = std::detail::PrimitiveVector<f32, 3>;
using f32x4 = std::detail::PrimitiveVector<f32, 4>;
using f32x8 = std::detail::PrimitiveVector<f32, 8>;

// Vectors of up to 4 64-bit floats are supported by AVX2.
using f64x2 = std::detail::PrimitiveVector<f64, 2>;
using f64x3 = std::detail::PrimitiveVector<f64, 3>;
using f64x4 = std::detail::PrimitiveVector<f64, 4>;

// Vectors of up to 32 8-bit bools are supported by AVX2.
using bool8x2 = std::detail::PrimitiveVector<bool8, 2>;
using bool8x3 = std::detail::PrimitiveVector<bool8, 3>;
using bool8x4 = std::detail::PrimitiveVector<bool8, 4>;
using bool8x8 = std::detail::PrimitiveVector<bool8, 8>;
using bool8x16 = std::detail::PrimitiveVector<bool8, 16>;
using bool8x32 = std::detail::PrimitiveVector<bool8, 32>;

// Vectors of up to 16 8-bit bools are supported by AVX2.
using bool16x2 = std::detail::PrimitiveVector<bool16, 2>;
using bool16x3 = std::detail::PrimitiveVector<bool16, 3>;
using bool16x4 = std::detail::PrimitiveVector<bool16, 4>;
using bool16x8 = std::detail::PrimitiveVector<bool16, 8>;
using bool16x16 = std::detail::PrimitiveVector<bool16, 16>;

// Vectors of up to 8 32-bit bools are supported by AVX2.
using bool32x2 = std::detail::PrimitiveVector<bool32, 2>;
using bool32x3 = std::detail::PrimitiveVector<bool32, 3>;
using bool32x4 = std::detail::PrimitiveVector<bool32, 4>;
using bool32x8 = std::detail::PrimitiveVector<bool32, 8>;
