// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* The Intel-style SIMD syntax is completely arbitrary in GCC. GNU implemented
 * it with wrapper libraries around their own basic, arguably more reasonable,
 * compiler intrinsics that already understand arithmetic operators, sets,
 * loads, and many other operations. Then, authors of a SIMD wrapper library
 * wrap *those* wrappers with new ones to enhance their quality of life with
 * features that the basic compiler intrinsics largely already had. There are
 * three layers of technology to this for no reason!
 *
 * To streamline this, libCat uses the intrinsics which GNU already provides,
 * and wraps it in one thin layer of technology. */

#include <type_traits>

namespace std::detail {

/* Vectors have weak alignment by default. It is up to users at call-site to
 * determine what alignment is appropriate for their use-case. All SIMD
 * functions are unaligned, which pays little, if any, penalty to aligned
 * vectors in modern implementations of SSE, AVX, and NEON architectures. */

template <typename T, isize Width>
struct alignas(4) SimdVector {
    static constexpr isize width = Width;
    using ScalarType = T;
    /* `vector_size` is a GCC attribute that represents SIMD data-types.
     * `using` and `alignas` do not work on a builtin vector type. Generalized
     * attribute syntax also does not appear to work. */
    typedef T  // NOLINT
        __attribute__((vector_size(sizeof(ScalarType) * Width), aligned(4)))
        IntrinsicType;
    IntrinsicType value;

    constexpr SimdVector() = default;
    // TODO: Perfect forwarding.
    constexpr SimdVector(IntrinsicType const& in_vector) {
        this->value = in_vector;
    }
    constexpr SimdVector(SimdVector<T, Width> const& operand) {
        this->value = operand.value;
    }
    // TODO: It would be nice to get this to work:
    // template <typename... UList>
    // constexpr simd_vector(UList... values) {
    //     this->value((static_cast<T>(values))...);
    // }

    // TODO: `__builtin_convertvector()` wrapped by conversion operators.

    constexpr auto operator=(SimdVector<T, Width>& operand)
        -> SimdVector<T, Width>& {
        this->value = operand.value;
        return *this;
    }

    constexpr auto operator+(SimdVector<T, Width> const& operand)
        -> SimdVector<T, Width> {
        return SimdVector<T, Width>{this->value + operand.value};
    }
    constexpr auto operator+=(SimdVector<T, Width> const& operand)
        -> SimdVector<T, Width>& {
        this->value = this->value + operand.value;
        return *this;
    }

    constexpr auto operator-(SimdVector<T, Width> const& operand)
        -> SimdVector<T, Width> {
        return SimdVector<T, Width>{this->value - operand.value};
    }
    constexpr auto operator-=(SimdVector<T, Width> const& operand)
        -> SimdVector<T, Width>& {
        this->value = this->value - operand.value;
        return *this;
    }
};

}  // namespace std::detail

// Vectors of up to 32 1-byte integers are supported by AVX2.
using i1x2 = std::detail::SimdVector<i1, 2>;
using i1x4 = std::detail::SimdVector<i1, 4>;
using i1x8 = std::detail::SimdVector<i1, 8>;
using i1x16 = std::detail::SimdVector<i1, 16>;
using i1x32 = std::detail::SimdVector<i1, 32>;
using u1x2 = std::detail::SimdVector<u1, 2>;
using u1x4 = std::detail::SimdVector<u1, 4>;
using u1x8 = std::detail::SimdVector<u1, 8>;
using u1x16 = std::detail::SimdVector<u1, 16>;
using u1x32 = std::detail::SimdVector<u1, 32>;

/* Strings need their own vectors because u1 and i1 are incompatible with some
 * intrinsic functions.*/
using charx2 = std::detail::SimdVector<char, 2>;
using charx4 = std::detail::SimdVector<char, 4>;
using charx8 = std::detail::SimdVector<char, 8>;
using charx16 = std::detail::SimdVector<char, 16>;
using charx32 = std::detail::SimdVector<char, 32>;

// Vectors of up to 16 2-byte integers are supported by AVX2.
using i2x2 = std::detail::SimdVector<i2, 2>;
using i2x4 = std::detail::SimdVector<i2, 4>;
using i2x8 = std::detail::SimdVector<i2, 8>;
using i2x16 = std::detail::SimdVector<i2, 16>;
using u2x2 = std::detail::SimdVector<u2, 2>;
using u2x4 = std::detail::SimdVector<u2, 4>;
using u2x8 = std::detail::SimdVector<u2, 8>;
using u2x16 = std::detail::SimdVector<u2, 16>;

// Vectors of up to 8 4-byte integers are supported by AVX2.
using i4x2 = std::detail::SimdVector<i4, 2>;
using i4x4 = std::detail::SimdVector<i4, 4>;
using i4x8 = std::detail::SimdVector<i4, 8>;
using u4x2 = std::detail::SimdVector<u4, 2>;
using u4x4 = std::detail::SimdVector<u4, 4>;
using u4x8 = std::detail::SimdVector<u4, 8>;

// Vectors of up to 4 8-byte integers are supported by AVX2.
using i8x2 = std::detail::SimdVector<i8, 2>;
using i8x4 = std::detail::SimdVector<i8, 4>;
using u8x2 = std::detail::SimdVector<u8, 2>;
using u8x4 = std::detail::SimdVector<u8, 4>;

// Vectors of up to 8 4-byte floats are supported by AVX2.
using f4x2 = std::detail::SimdVector<f4, 2>;
using f4x4 = std::detail::SimdVector<f4, 4>;
using f4x8 = std::detail::SimdVector<f4, 8>;

// TODO: Evaluate what support for 8-byte ints exists in x86-64.
//
// Vectors of up to 4 8-byte floats are supported by AVX2.
using f8x2 = std::detail::SimdVector<f8, 2>;
using f8x4 = std::detail::SimdVector<f8, 4>;

// Vectors of up to 32 1-byte bools are supported by AVX2.
using bool1x2 = std::detail::SimdVector<bool1, 2>;
using bool1x4 = std::detail::SimdVector<bool1, 4>;
using bool1x8 = std::detail::SimdVector<bool1, 8>;
using bool1x16 = std::detail::SimdVector<bool1, 16>;
using bool1x32 = std::detail::SimdVector<bool1, 32>;

// Vectors of up to 16 2-byte bools are supported by AVX2.
using bool2x2 = std::detail::SimdVector<bool2, 2>;
using bool2x4 = std::detail::SimdVector<bool2, 4>;
using bool2x8 = std::detail::SimdVector<bool2, 8>;
using bool2x16 = std::detail::SimdVector<bool2, 16>;

// Vectors of up to 8 4-byte bools are supported by AVX2.
using bool4x2 = std::detail::SimdVector<bool4, 2>;
using bool4x4 = std::detail::SimdVector<bool4, 4>;
using bool4x8 = std::detail::SimdVector<bool4, 8>;

namespace simd {

enum VectorMask : u1
{
    // Source data format.
    SIDD_UBYTE_OPS = 0x00,
    SIDD_UWORD_OPS = 0x01,
    SIDD_SBYTE_OPS = 0x02,
    SIDD_SWORD_OPS = 0x03,
    // Comparison operation.
    SIDD_CMP_EQUAL_ANY = 0x00,
    SIDD_CMP_RANGES = 0x04,
    SIDD_CMP_EQUAL_EACH = 0x08,
    SIDD_CMP_EQUAL_ORDERED = 0x0c,
    // Polarity.
    SIDD_POSITIVE_POLARITY = 0x00,
    SIDD_NEGATIVE_POLARITY = 0x10,
    SIDD_MASKED_POSITIVE_POLARITY = 0x20,
    SIDD_MASKED_NEGATIVE_POLARITY = 0x30,
    // Output selection in _mm_cmpXstri()
    SIDD_LEAST_SIGNIFICANT = 0x00,
    SIDD_MOST_SIGNIFICANT = 0x40,
    // Output selection in _mm_cmpXstrm ().
    SIDD_BIT_MASK = 0x00,
    SIDD_UNIT_MASK = 0x40,
};

template <typename T>
consteval auto set_zeros() -> T {
    // TODO: Is there a cleverer way to do this? Variadic templates?
    // Probably an integer_sequence.
    using ScalarType = typename T::ScalarType;
    using VectorType = std::detail::SimdVector<ScalarType, T::width>;
    using IntrinsicType = decltype(VectorType::value);

    if constexpr (T::width == 2) {
        return IntrinsicType{0, 0};
    } else if constexpr (T::width == 4) {
        return IntrinsicType{0, 0, 0, 0};
    } else if constexpr (T::width == 8) {
        return IntrinsicType{0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::width == 16) {
        return IntrinsicType{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::width == 32) {
        return IntrinsicType{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }
    __builtin_unreachable();
}

// TODO: Use a vector concept.
auto shuffle(auto in_vector, auto mask) {
    decltype(in_vector) out_vector;
    __builtin_shuffle(in_vector, out_vector, mask);
    return out_vector;
}

template <u4 Width>
auto p_string_to_p_vector(char8_t const* p_string) {
    using U = std::detail::SimdVector<char, Width>;
    return reinterpret_cast<U*>(const_cast<char8_t*>(p_string));
}

// TODO: Improve these function names.
// TODO: Perfect forwarding.
template <u1 Mask>
auto cmp_implicit_str_c(auto const& vector_1, auto const& vector_2) -> bool {
    static_assert(meta::is_same_v<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistric128(vector_1.value, vector_2.value, Mask);
}

template <u1 Mask>
auto cmp_implicit_str_i(auto const& vector_1, auto const& vector_2) -> i4 {
    static_assert(meta::is_same_v<decltype(vector_1), decltype(vector_2)>);
    return __builtin_ia32_pcmpistri128(vector_1.value, vector_2.value, Mask);
}

// TODO: Add `simd::mfence` and `simd::lfence`.
void sfence();
void zero_avx_registers();
void zero_upper_avx_registers();

// Constants for prefetch.
enum MM_HINT
{
    // MM_HINT_ET is MM_HINT_T with set 3rd bit.
    MM_HINT_ET0 = 7,
    MM_HINT_ET1 = 6,
    MM_HINT_T0 = 3,
    MM_HINT_T1 = 2,
    MM_HINT_T2 = 1,
    MM_HINT_NTA = 0
};

// This will not compile as a function.
#define prefetch(p_source, hint) \
    { __builtin_prefetch((p_source), (hint)&0x4 >> 2, (hint)&0x3); }

// TODO: Constrain parameter with a vector concept.
// TODO: This code can be simplified a lot.
/* Non-temporally copy a vector into some address. */
template <typename T>
void stream_in(void* p_destination, T const* source) {
    // TODO: Make an integral-vector concept to simplify this.
    // Streaming 4-byte floats.
    if constexpr (meta::is_same_v<T, f4x4>) {
        __builtin_ia32_movntps(p_destination, source);
    } else if constexpr (meta::is_same_v<T, f4x8>) {
        __builtin_ia32_movntps256(p_destination, source);
    }
    // Streaming 8-byte floats.
    if constexpr (meta::is_same_v<T, f8x2>) {
        __builtin_ia32_movntpd(p_destination, source);
    } else if constexpr (meta::is_same_v<T, f8x4>) {
        __builtin_ia32_movntpd256(p_destination, source);
    }
    // Streaming 1-byte ints.
    else if constexpr (meta::is_same_v<T, u1x4> || meta::is_same_v<T, i1x4>) {
        __builtin_ia32_movnti(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x8> || meta::is_same_v<T, i1x8>) {
        __builtin_ia32_movntq(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x16> ||
                         meta::is_same_v<T, i1x16>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x32> ||
                         meta::is_same_v<T, i1x32>) {
        __builtin_ia32_movntq256(p_destination, source);
    }
    // Streaming 2-byte ints.
    else if constexpr (meta::is_same_v<T, u2x2> || meta::is_same_v<T, i2x2>) {
        __builtin_ia32_movnti(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u2x4> || meta::is_same_v<T, i2x4>) {
        __builtin_ia32_movntq(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u2x8> || meta::is_same_v<T, i2x8>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u2x16> ||
                         meta::is_same_v<T, i2x16>) {
        __builtin_ia32_movntq256(p_destination, source);
    }
    // Streaming 4-byte ints.
    else if constexpr (meta::is_same_v<T, u4x2> || meta::is_same_v<T, i4x2>) {
        __builtin_ia32_movntq(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u4x4> || meta::is_same_v<T, i4x4>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u4x8> || meta::is_same_v<T, i4x8>) {
        __builtin_ia32_movntdq256(p_destination, source);
    }
    // Streaming 8-byte ints.
    else if constexpr (meta::is_same_v<T, u1x2> || meta::is_same_v<T, i1x2>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x4> || meta::is_same_v<T, i1x4>) {
        __builtin_ia32_movntdq256(p_destination, source);
    }
}

}  // namespace simd

// TODO: __builtin_cpu_init()
// must be called before these.

auto is_mmx_supported() -> bool;
auto is_sse1_supported() -> bool;
auto is_sse2_supported() -> bool;
auto is_sse3_supported() -> bool;
auto is_ssse3_supported() -> bool;
auto is_sse4_1_supported() -> bool;
auto is_sse4_2_supported() -> bool;
auto is_avx_supported() -> bool;
auto is_avx2_supported() -> bool;
auto is_avx512f_supported() -> bool;
auto is_avx512vl_supported() -> bool;
