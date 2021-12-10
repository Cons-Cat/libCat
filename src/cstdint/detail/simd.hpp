#pragma once

namespace std::detail {

template <typename T, usize Width>
struct vector {
    // vector_size is a GCC attribute that represents SIMD data-types.
    T value __attribute__((vector_size(sizeof(T) * Width)));

    auto operator+(vector<T, Width> const& rhs) -> vector<T, Width> {
        return value + rhs.value;
    }
    auto operator+=(vector<T, Width> const& rhs) -> vector<T, Width>& {
        value = value + rhs.value;
        return *this;
    }
};

}  // namespace std::detail

// Vectors of up to 32 8-bit integers are supported by AVX2.
using i8x2 = std::detail::vector<i8, 2>;
using i8x4 = std::detail::vector<i8, 4>;
using i8x8 = std::detail::vector<i8, 8>;
using i8x16 = std::detail::vector<i8, 16>;
using i8x32 = std::detail::vector<i8, 32>;
using u8x2 = std::detail::vector<u8, 2>;
using u8x4 = std::detail::vector<u8, 4>;
using u8x8 = std::detail::vector<u8, 8>;
using u8x16 = std::detail::vector<u8, 16>;
using u8x32 = std::detail::vector<u8, 32>;

// Vectors of up to 16 16-bit integers are supported by AVX2.
using i16x2 = std::detail::vector<i16, 2>;
using i16x4 = std::detail::vector<i16, 4>;
using i16x16 = std::detail::vector<i16, 16>;
using u16x2 = std::detail::vector<u16, 2>;
using u16x3 = std::detail::vector<u16, 3>;
using u16x4 = std::detail::vector<u16, 4>;

// Vectors of up to 8 32-bit integers are supported by AVX2.
using i32x2 = std::detail::vector<i32, 2>;
using i32x4 = std::detail::vector<i32, 4>;
using i32x8 = std::detail::vector<i32, 8>;
using u32x2 = std::detail::vector<u32, 2>;
using u32x4 = std::detail::vector<u32, 4>;
using u32x8 = std::detail::vector<u32, 8>;

// Vectors of up to 8 32-bit floats are supported by AVX2.
using f32x2 = std::detail::vector<f32, 2>;
using f32x4 = std::detail::vector<f32, 4>;
using f32x8 = std::detail::vector<f32, 8>;

// TODO: Evaluate what support for 64-bit ints exists in x86-64.
// Vectors of up to 4 64-bit floats are supported by AVX2.
// using f64x2 = std::detail::vector<f64, 2>;
// using f64x4 = std::detail::vector<f64, 4>;

// Vectors of up to 32 8-bit bools are supported by AVX2.
using bool8x2 = std::detail::vector<bool8, 2>;
using bool8x4 = std::detail::vector<bool8, 4>;
using bool8x8 = std::detail::vector<bool8, 8>;
using bool8x16 = std::detail::vector<bool8, 16>;
using bool8x32 = std::detail::vector<bool8, 32>;

// Vectors of up to 16 8-bit bools are supported by AVX2.
using bool16x2 = std::detail::vector<bool16, 2>;
using bool16x4 = std::detail::vector<bool16, 4>;
using bool16x8 = std::detail::vector<bool16, 8>;
using bool16x16 = std::detail::vector<bool16, 16>;

// Vectors of up to 8 32-bit bools are supported by AVX2.
using bool32x2 = std::detail::vector<bool32, 2>;
using bool32x4 = std::detail::vector<bool32, 4>;
using bool32x8 = std::detail::vector<bool32, 8>;
