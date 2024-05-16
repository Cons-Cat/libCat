#pragma once
// Copyright 2020-2022 Junekey Jeon
//
// The contents of this file may be used under the terms of
// the Apache License v2.0 with LLVM Exceptions.
//
//    (See accompanying file LICENSE-Apache or copy at
//     https://llvm.org/foundation/relicensing/LICENSE.txt)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

// NOLINTBEGIN

#include <cat/memory>
#include <cat/string>

// Suppress additional buffer overrun check.
// I have no idea why MSVC thinks some functions here are vulnerable to the
// buffer overrun attacks. No, they aren't.
#if defined(__GNUC__) || defined(__clang__)
#define JKJ_SAFEBUFFERS
#define JKJ_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define JKJ_SAFEBUFFERS __declspec(safebuffers)
#define JKJ_FORCEINLINE __forceinline
#else
#define JKJ_SAFEBUFFERS
#define JKJ_FORCEINLINE inline
#endif

#if defined(__has_builtin)
#define JKJ_DRAGONBOX_HAS_BUILTIN(x) __has_builtin(x)
#else
#define JKJ_DRAGONBOX_HAS_BUILTIN(x) false
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace cat::detail::dragonbox {
namespace detail {
    template <class T>
    constexpr uword::raw_type physical_bits =
        sizeof(T) * limits<unsigned char>::digits;

    template <class T>
        requires(is_unsigned<T>)
    inline constexpr uword::raw_type value_bits = limits<T>::digits;
}  // namespace detail

// These classes expose encoding specs of IEEE-754-like floating-point formats.
// Currently available formats are IEEE754-binary32 & IEEE754-binary64.

struct ieee754_binary32 {
    static constexpr int significand_bits = 23;
    static constexpr int exponent_bits = 8;
    static constexpr int min_exponent = -126;
    static constexpr int max_exponent = 127;
    static constexpr int exponent_bias = -127;
    static constexpr int decimal_digits = 9;
};

struct ieee754_binary64 {
    static constexpr int significand_bits = 52;
    static constexpr int exponent_bits = 11;
    static constexpr int min_exponent = -1'022;
    static constexpr int max_exponent = 1'023;
    static constexpr int exponent_bias = -1'023;
    static constexpr int decimal_digits = 17;
};

// A floating-point traits class defines ways to interpret a bit pattern of
// given size as an encoding of floating-point number. This is a default
// implementation of such a traits class, supporting ways to interpret 32-bits
// into a binary32-encoded floating-point number and to interpret 64-bits into a
// binary64-encoded floating-point number. Users might specialize this class to
// change the default behavior for certain types.
template <class T>
struct default_float_traits {
    // I don't know if there is a truly reliable way of detecting
    // IEEE-754 binary32/binary64 formats; I just did my best here.
    static_assert(
        limits<T>::is_iec559 && limits<T>::radix == 2 &&
            (detail::physical_bits<T> == 32 || detail::physical_bits<T> == 64),
        "default_ieee754_traits only works for 32-bits or 64-bits types "
        "supporting binary32 or binary64 formats!");

    // The type that is being viewed.
    using type = T;

    // Refers to the format specification class.
    using format = conditional<detail::physical_bits<T> == 32, ieee754_binary32,
                               ieee754_binary64>;

    // Defines an unsigned integer type that is large enough to carry a variable
    // of type T. Most of the operations will be done on this integer type.
    using carrier_uint = conditional<detail::physical_bits<T> == 32,
                                     uint4::raw_type, uint8::raw_type>;
    static_assert(sizeof(carrier_uint) == sizeof(T));

    // Number of bits in the above unsigned integer type.
    static constexpr int carrier_bits =
        int(detail::physical_bits<carrier_uint>);

    // Convert from carrier_uint into the original type.
    // Depending on the floating-point encoding format, this operation might not
    // be possible for some specific bit patterns. However, the contract is that
    // u always denotes a valid bit pattern, so this function must be assumed to
    // be noexcept.
    static T
    carrier_to_float(carrier_uint u) noexcept {
        T x;
        copy_memory_small(&u, &x, sizeof(carrier_uint));
        return x;
    }

    // Same as above.
    static carrier_uint
    float_to_carrier(T x) noexcept {
        carrier_uint u;
        copy_memory_small(&x, &u, sizeof(carrier_uint));
        return u;
    }

    // Extract exponent bits from a bit pattern.
    // The result must be aligned to the LSB so that there is no additional zero
    // paddings on the right. This function does not do bias adjustment.
    static constexpr unsigned int
    extract_exponent_bits(carrier_uint u) noexcept {
        constexpr int significand_bits = format::significand_bits;
        constexpr int exponent_bits = format::exponent_bits;
        static_assert(detail::value_bits<unsigned int> > exponent_bits);
        constexpr auto exponent_bits_mask =
            (unsigned int)(((unsigned int)(1) << exponent_bits) - 1);
        return (unsigned int)(u >> significand_bits) & exponent_bits_mask;
    }

    // Extract significand bits from a bit pattern.
    // The result must be aligned to the LSB so that there is no additional zero
    // paddings on the right. The result does not contain the implicit bit.
    static constexpr carrier_uint
    extract_significand_bits(carrier_uint u) noexcept {
        constexpr auto mask =
            carrier_uint((carrier_uint(1) << format::significand_bits) - 1);
        return carrier_uint(u & mask);
    }

    // Remove the exponent bits and extract significand bits together with the
    // sign bit.
    static constexpr carrier_uint
    remove_exponent_bits(carrier_uint u, unsigned int exponent_bits) noexcept {
        return u ^ (carrier_uint(exponent_bits) << format::significand_bits);
    }

    // Shift the obtained signed significand bits to the left by 1 to remove the
    // sign bit.
    static constexpr carrier_uint
    remove_sign_bit_and_shift(carrier_uint u) noexcept {
        return carrier_uint(carrier_uint(u) << 1);
    }

    // The actual value of exponent is obtained by adding this value to the
    // extracted exponent bits.
    static constexpr int exponent_bias =
        1 - (1 << (carrier_bits - format::significand_bits - 2));

    // Obtain the actual value of the binary exponent from the extracted
    // exponent bits.
    static constexpr int
    binary_exponent(unsigned int exponent_bits) noexcept {
        if (exponent_bits == 0) {
            return format::min_exponent;
        } else {
            return int(exponent_bits) + format::exponent_bias;
        }
    }

    // Obtain the actual value of the binary exponent from the extracted
    // significand bits and exponent bits.
    static constexpr carrier_uint
    binary_significand(carrier_uint significand_bits,
                       unsigned int exponent_bits) noexcept {
        if (exponent_bits == 0) {
            return significand_bits;
        } else {
            return significand_bits |
                   (carrier_uint(1) << format::significand_bits);
        }
    }

    /* Various boolean observer functions */

    static constexpr bool
    is_nonzero(carrier_uint u) noexcept {
        return (u << 1) != 0;
    }

    static constexpr bool
    is_positive(carrier_uint u) noexcept {
        constexpr auto sign_bit = carrier_uint(1) << (format::significand_bits +
                                                      format::exponent_bits);
        return u < sign_bit;
    }

    static constexpr bool
    is_negative(carrier_uint u) noexcept {
        return !is_positive(u);
    }

    static constexpr bool
    is_finite(unsigned int exponent_bits) noexcept {
        constexpr unsigned int exponent_bits_all_set =
            (1u << format::exponent_bits) - 1;
        return exponent_bits != exponent_bits_all_set;
    }

    static constexpr bool
    has_all_zero_significand_bits(carrier_uint u) noexcept {
        return (u << 1) == 0;
    }

    static constexpr bool
    has_even_significand_bits(carrier_uint u) noexcept {
        return u % 2 == 0;
    }
};

// Convenient wrappers for floating-point traits classes.
// In order to reduce the argument passing overhead, these classes should be as
// simple as possible (e.g., no inheritance, no private non-static data member,
// etc.; this is an unfortunate fact about common ABI convention).

template <class T, class Traits = default_float_traits<T>>
struct float_bits;

template <class T, class Traits = default_float_traits<T>>
struct signed_significand_bits;

template <class T, class Traits>
struct float_bits {
    using type = T;
    using traits_type = Traits;
    using carrier_uint = traits_type::carrier_uint;

    carrier_uint u;

    float_bits() = default;

    constexpr explicit float_bits(carrier_uint bit_pattern) noexcept
        : u{bit_pattern} {
    }

    constexpr explicit float_bits(T float_value) noexcept
        : u{traits_type::float_to_carrier(float_value)} {
    }

    constexpr T
    to_float() const noexcept {
        return traits_type::carrier_to_float(u);
    }

    // Extract exponent bits from a bit pattern.
    // The result must be aligned to the LSB so that there is no additional zero
    // paddings on the right. This function does not do bias adjustment.
    constexpr unsigned int
    extract_exponent_bits() const noexcept {
        return traits_type::extract_exponent_bits(u);
    }

    // Extract significand bits from a bit pattern.
    // The result must be aligned to the LSB so that there is no additional zero
    // paddings on the right. The result does not contain the implicit bit.
    constexpr carrier_uint
    extract_significand_bits() const noexcept {
        return traits_type::extract_significand_bits(u);
    }

    // Remove the exponent bits and extract significand bits together with the
    // sign bit.
    constexpr auto
    remove_exponent_bits(unsigned int exponent_bits) const noexcept {
        return signed_significand_bits<type, traits_type>(
            traits_type::remove_exponent_bits(u, exponent_bits));
    }

    // Obtain the actual value of the binary exponent from the extracted
    // exponent bits.
    static constexpr int
    binary_exponent(unsigned int exponent_bits) noexcept {
        return traits_type::binary_exponent(exponent_bits);
    }

    constexpr int
    binary_exponent() const noexcept {
        return binary_exponent(extract_exponent_bits());
    }

    // Obtain the actual value of the binary exponent from the extracted
    // significand bits and exponent bits.
    static constexpr carrier_uint
    binary_significand(carrier_uint significand_bits,
                       unsigned int exponent_bits) noexcept {
        return traits_type::binary_significand(significand_bits, exponent_bits);
    }

    constexpr carrier_uint
    binary_significand() const noexcept {
        return binary_significand(extract_significand_bits(),
                                  extract_exponent_bits());
    }

    constexpr bool
    is_nonzero() const noexcept {
        return traits_type::is_nonzero(u);
    }

    constexpr bool
    is_positive() const noexcept {
        return traits_type::is_positive(u);
    }

    constexpr bool
    is_negative() const noexcept {
        return traits_type::is_negative(u);
    }

    constexpr bool
    is_finite(unsigned int exponent_bits) const noexcept {
        return traits_type::is_finite(exponent_bits);
    }

    constexpr bool
    is_finite() const noexcept {
        return traits_type::is_finite(extract_exponent_bits());
    }

    constexpr bool
    has_even_significand_bits() const noexcept {
        return traits_type::has_even_significand_bits(u);
    }
};

template <class T, class Traits>
struct signed_significand_bits {
    using type = T;
    using traits_type = Traits;
    using carrier_uint = traits_type::carrier_uint;

    carrier_uint u;

    signed_significand_bits() = default;

    constexpr explicit signed_significand_bits(
        carrier_uint bit_pattern) noexcept
        : u{bit_pattern} {
    }

    // Shift the obtained signed significand bits to the left by 1 to remove the
    // sign bit.
    constexpr carrier_uint
    remove_sign_bit_and_shift() const noexcept {
        return traits_type::remove_sign_bit_and_shift(u);
    }

    constexpr bool
    is_positive() const noexcept {
        return traits_type::is_positive(u);
    }

    constexpr bool
    is_negative() const noexcept {
        return traits_type::is_negative(u);
    }

    constexpr bool
    has_all_zero_significand_bits() const noexcept {
        return traits_type::has_all_zero_significand_bits(u);
    }

    constexpr bool
    has_even_significand_bits() const noexcept {
        return traits_type::has_even_significand_bits(u);
    }
};

namespace detail {
    ////////////////////////////////////////////////////////////////////////////////////////
    // Bit operation intrinsics.
    ////////////////////////////////////////////////////////////////////////////////////////

    namespace bits {
        // Most compilers should be able to optimize this into the ROR
        // instruction.
        inline uint4::raw_type
        rotr(uint4::raw_type n, uint4::raw_type r) noexcept {
            r &= 31;
            return (n >> r) | (n << (32 - r));
        }

        inline uint8::raw_type
        rotr(uint8::raw_type n, uint4::raw_type r) noexcept {
            r &= 63;
            return (n >> r) | (n << (64 - r));
        }
    }  // namespace bits

    ////////////////////////////////////////////////////////////////////////////////////////
    // Utilities for wide unsigned integer arithmetic.
    ////////////////////////////////////////////////////////////////////////////////////////

    namespace wuint {
        // Compilers might support built-in 128-bit integer types. However, it
        // seems that emulating them with a pair of 64-bit integers actually
        // produces a better code, so we avoid using those built-ins. That said,
        // they are still useful for implementing 64-bit x 64-bit -> 128-bit
        // multiplication.

        // clang-format off
    #if defined(__SIZEOF_INT128__)
            // To silence "error: ISO C++ does not support '__int128' for 'type name'
            // [-Wpedantic]"
        #if defined(__GNUC__)
            __extension__
        #endif
            using builtin_uint128_t = unsigned __int128;
    #endif
        // clang-format on

        struct uint128 {
            uint128() = default;

            uint8::raw_type high_;
            uint8::raw_type low_;

            constexpr uint128(uint8::raw_type high,
                              uint8::raw_type low) noexcept
                : high_{high}, low_{low} {
            }

            constexpr uint8::raw_type
            high() const noexcept {
                return high_;
            }

            constexpr uint8::raw_type
            low() const noexcept {
                return low_;
            }

            uint128&
            operator+=(uint8::raw_type n) & noexcept {
#if JKJ_DRAGONBOX_HAS_BUILTIN(__builtin_addcll)
                unsigned long long carry;
                low_ = __builtin_addcll(low_, n, 0, &carry);
                high_ = __builtin_addcll(high_, 0, carry, &carry);
#elif JKJ_DRAGONBOX_HAS_BUILTIN(__builtin_ia32_addcarryx_u64)
                unsigned long long result;
                auto carry = __builtin_ia32_addcarryx_u64(0, low_, n, &result);
                low_ = result;
                __builtin_ia32_addcarryx_u64(carry, high_, 0, &result);
                high_ = result;
#elif defined(_MSC_VER) && defined(_M_X64)
                auto carry = _addcarry_u64(0, low_, n, &low_);
                _addcarry_u64(carry, high_, 0, &high_);
#else
                auto sum = low_ + n;
                high_ += (sum < low_ ? 1 : 0);
                low_ = sum;
#endif
                return *this;
            }
        };

        static inline uint8::raw_type
        umul64(uint4::raw_type x, uint4::raw_type y) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
            return __emulu(x, y);
#else
            return x * uint8::raw_type(y);
#endif
        }

        // Get 128-bit result of multiplication of two 64-bit unsigned integers.
        JKJ_SAFEBUFFERS inline uint128
        umul128(uint8::raw_type x, uint8::raw_type y) noexcept {
#if defined(__SIZEOF_INT128__)
            auto result = builtin_uint128_t(x) * builtin_uint128_t(y);
            return {uint8::raw_type(result >> 64), uint8::raw_type(result)};
#elif defined(_MSC_VER) && defined(_M_X64)
            uint128 result;
            result.low_ = _umul128(x, y, &result.high_);
            return result;
#else
            auto a = uint4::raw_type(x >> 32);
            auto b = uint4::raw_type(x);
            auto c = uint4::raw_type(y >> 32);
            auto d = uint4::raw_type(y);

            auto ac = umul64(a, c);
            auto bc = umul64(b, c);
            auto ad = umul64(a, d);
            auto bd = umul64(b, d);

            auto intermediate =
                (bd >> 32) + uint4::raw_type(ad) + uint4::raw_type(bc);

            return {ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32),
                    (intermediate << 32) + uint4::raw_type(bd)};
#endif
        }

        JKJ_SAFEBUFFERS inline uint8::raw_type
        umul128_upper64(uint8::raw_type x, uint8::raw_type y) noexcept {
#if defined(__SIZEOF_INT128__)
            auto result = builtin_uint128_t(x) * builtin_uint128_t(y);
            return uint8::raw_type(result >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
            return __umulh(x, y);
#else
            auto a = uint4::raw_type(x >> 32);
            auto b = uint4::raw_type(x);
            auto c = uint4::raw_type(y >> 32);
            auto d = uint4::raw_type(y);

            auto ac = umul64(a, c);
            auto bc = umul64(b, c);
            auto ad = umul64(a, d);
            auto bd = umul64(b, d);

            auto intermediate =
                (bd >> 32) + uint4::raw_type(ad) + uint4::raw_type(bc);

            return ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32);
#endif
        }

        // Get upper 128-bits of multiplication of a 64-bit unsigned integer and
        // a 128-bit unsigned integer.
        JKJ_SAFEBUFFERS inline uint128
        umul192_upper128(uint8::raw_type x, uint128 y) noexcept {
            auto r = umul128(x, y.high());
            r += umul128_upper64(x, y.low());
            return r;
        }

        // Get upper 64-bits of multiplication of a 32-bit unsigned integer and
        // a 64-bit unsigned integer.
        inline uint8::raw_type
        umul96_upper64(uint4::raw_type x, uint8::raw_type y) noexcept {
#if defined(__SIZEOF_INT128__) || (defined(_MSC_VER) && defined(_M_X64))
            return umul128_upper64(uint8::raw_type(x) << 32, y);
#else
            auto yh = uint4::raw_type(y >> 32);
            auto yl = uint4::raw_type(y);

            auto xyh = umul64(x, yh);
            auto xyl = umul64(x, yl);

            return xyh + (xyl >> 32);
#endif
        }

        // Get lower 128-bits of multiplication of a 64-bit unsigned integer and
        // a 128-bit unsigned integer.
        JKJ_SAFEBUFFERS inline uint128
        umul192_lower128(uint8::raw_type x, uint128 y) noexcept {
            auto high = x * y.high();
            auto high_low = umul128(x, y.low());
            return {high + high_low.high(), high_low.low()};
        }

        // Get lower 64-bits of multiplication of a 32-bit unsigned integer and
        // a 64-bit unsigned integer.
        inline uint8::raw_type
        umul96_lower64(uint4::raw_type x, uint8::raw_type y) noexcept {
            return x * y;
        }
    }  // namespace wuint

    ////////////////////////////////////////////////////////////////////////////////////////
    // Some simple utilities for constexpr computation.
    ////////////////////////////////////////////////////////////////////////////////////////

    template <int k, class Int>
    constexpr Int
    compute_power(Int a) noexcept {
        static_assert(k >= 0);
        Int p = 1;
        for (int i = 0; i < k; ++i) {
            p *= a;
        }
        return p;
    }

    template <int a, class UInt>
    constexpr int
    count_factors(UInt n) noexcept {
        static_assert(a > 1);
        int c = 0;
        while (n % a == 0) {
            n /= a;
            ++c;
        }
        return c;
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    // Utilities for fast/constexpr log computation.
    ////////////////////////////////////////////////////////////////////////////////////////

    namespace log {
        static_assert((-1 >> 1) == -1,
                      "right-shift for signed integers must be arithmetic");

        // Compute floor(e * c - s).
        enum class multiply : uint4::raw_type {
        };
        enum class subtract : uint4::raw_type {
        };
        enum class shift : uword::raw_type {
        };
        enum class min_exponent : int4::raw_type {
        };
        enum class max_exponent : int4::raw_type {
        };

        template <multiply m, subtract f, shift k, min_exponent e_min,
                  max_exponent e_max>
        constexpr int
        compute(int e) noexcept {
            // assert(int4::raw_type(e_min) <= e && e <= int4::raw_type(e_max));
            return int(
                (int4::raw_type(e) * int4::raw_type(m) - int4::raw_type(f)) >>
                uword::raw_type(k));
        }

        // For constexpr computation.
        // Returns -1 when n = 0.
        template <class UInt>
        constexpr int
        floor_log2(UInt n) noexcept {
            int count = -1;
            while (n != 0) {
                ++count;
                n >>= 1;
            }
            return count;
        }

        static constexpr int floor_log10_pow2_min_exponent = -2'620;
        static constexpr int floor_log10_pow2_max_exponent = 2'620;

        constexpr int
        floor_log10_pow2(int e) noexcept {
            using namespace log;
            return compute<multiply(315'653), subtract(0), shift(20),
                           min_exponent(floor_log10_pow2_min_exponent),
                           max_exponent(floor_log10_pow2_max_exponent)>(e);
        }

        static constexpr int floor_log2_pow10_min_exponent = -1'233;
        static constexpr int floor_log2_pow10_max_exponent = 1'233;

        constexpr int
        floor_log2_pow10(int e) noexcept {
            using namespace log;
            return compute<multiply(1'741'647), subtract(0), shift(19),
                           min_exponent(floor_log2_pow10_min_exponent),
                           max_exponent(floor_log2_pow10_max_exponent)>(e);
        }

        static constexpr int
            floor_log10_pow2_minus_log10_4_over_3_min_exponent = -2'985;
        static constexpr int
            floor_log10_pow2_minus_log10_4_over_3_max_exponent = 2'936;

        constexpr int
        floor_log10_pow2_minus_log10_4_over_3(int e) noexcept {
            using namespace log;
            return compute<
                multiply(631'305), subtract(261'663), shift(21),
                min_exponent(
                    floor_log10_pow2_minus_log10_4_over_3_min_exponent),
                max_exponent(
                    floor_log10_pow2_minus_log10_4_over_3_max_exponent)>(e);
        }

        static constexpr int floor_log5_pow2_min_exponent = -1'831;
        static constexpr int floor_log5_pow2_max_exponent = 1'831;

        constexpr int
        floor_log5_pow2(int e) noexcept {
            using namespace log;
            return compute<multiply(225'799), subtract(0), shift(19),
                           min_exponent(floor_log5_pow2_min_exponent),
                           max_exponent(floor_log5_pow2_max_exponent)>(e);
        }

        static constexpr int floor_log5_pow2_minus_log5_3_min_exponent = -3'543;
        static constexpr int floor_log5_pow2_minus_log5_3_max_exponent = 2'427;

        constexpr int
        floor_log5_pow2_minus_log5_3(int e) noexcept {
            using namespace log;
            return compute<
                multiply(451'597), subtract(715'764), shift(20),
                min_exponent(floor_log5_pow2_minus_log5_3_min_exponent),
                max_exponent(floor_log5_pow2_minus_log5_3_max_exponent)>(e);
        }
    }  // namespace log

    ////////////////////////////////////////////////////////////////////////////////////////
    // Utilities for fast divisibility tests.
    ////////////////////////////////////////////////////////////////////////////////////////

    namespace div {
        // Replace n by floor(n / 10^N).
        // Returns true if and only if n is divisible by 10^N.
        // Precondition: n <= 10^(N+1)
        // !!It takes an in-out parameter!!
        template <int N>
        struct divide_by_pow10_info;

        template <>
        struct divide_by_pow10_info<1> {
            static constexpr uint4::raw_type magic_number = 6'554;
            static constexpr int shift_amount = 16;
        };

        template <>
        struct divide_by_pow10_info<2> {
            static constexpr uint4::raw_type magic_number = 656;
            static constexpr int shift_amount = 16;
        };

        template <int N>
        constexpr bool
        check_divisibility_and_divide_by_pow10(uint4::raw_type& n) noexcept {
            // Make sure the computation for max_n does not overflow.
            static_assert(N + 1 <= log::floor_log10_pow2(31));
            // assert(n <= compute_power<N + 1>(uint4::raw_type(10)));

            using info = divide_by_pow10_info<N>;
            n *= info::magic_number;

            constexpr auto mask =
                uint4::raw_type(uint4::raw_type(1) << info::shift_amount) - 1;
            bool result = ((n & mask) < info::magic_number);

            n >>= info::shift_amount;
            return result;
        }

        // Compute floor(n / 10^N) for small n and N.
        // Precondition: n <= 10^(N+1)
        template <int N>
        constexpr uint4::raw_type
        small_division_by_pow10(uint4::raw_type n) noexcept {
            // Make sure the computation for max_n does not overflow.
            static_assert(N + 1 <= log::floor_log10_pow2(31));
            // assert(n <= compute_power<N + 1>(uint4::raw_type(10)));

            return (n * divide_by_pow10_info<N>::magic_number) >>
                   divide_by_pow10_info<N>::shift_amount;
        }

        // Compute floor(n / 10^N) for small N.
        // Precondition: n <= n_max
        template <int N, class UInt, UInt n_max>
        constexpr UInt
        divide_by_pow10(UInt n) noexcept {
            static_assert(N >= 0);

            // Specialize for 32-bit division by 100.
            // Compiler is supposed to generate the identical code for just
            // writing "n / 100", but for some reason MSVC generates an
            // inefficient code (mul + mov for no apparent reason, instead of
            // single imul), so we does this manually.
            if constexpr (cat::is_same<UInt, uint4::raw_type> && N == 2) {
                return uint4::raw_type(
                    wuint::umul64(n, uint4::raw_type(1'374'389'535)) >> 37);
            }
            // Specialize for 64-bit division by 1000.
            // Ensure that the correctness condition is met.
            if constexpr (cat::is_same<UInt, uint8::raw_type> && N == 3 &&
                          n_max <=
                              uint8::raw_type(15'534'100'272'597'517'998ull)) {
                return wuint::umul128_upper64(
                           n, uint8::raw_type(2'361'183'241'434'822'607ull)) >>
                       7;
            } else {
                constexpr auto divisor = compute_power<N>(UInt(10));
                return n / divisor;
            }
        }
    }  // namespace div
}  // namespace detail

////////////////////////////////////////////////////////////////////////////////////////
// Return types for the main interface function.
////////////////////////////////////////////////////////////////////////////////////////

template <class UInt, bool is_signed, bool trailing_zero_flag>
struct decimal_fp;

template <class UInt>
struct decimal_fp<UInt, false, false> {
    using carrier_uint = UInt;

    carrier_uint significand;
    int exponent;
};

template <class UInt>
struct decimal_fp<UInt, true, false> {
    using carrier_uint = UInt;

    carrier_uint significand;
    int exponent;
    bool is_negative;
};

template <class UInt>
struct decimal_fp<UInt, false, true> {
    using carrier_uint = UInt;

    carrier_uint significand;
    int exponent;
    bool may_have_trailing_zeros;
};

template <class UInt>
struct decimal_fp<UInt, true, true> {
    using carrier_uint = UInt;

    carrier_uint significand;
    int exponent;
    bool is_negative;
    bool may_have_trailing_zeros;
};

template <class UInt>
using unsigned_decimal_fp = decimal_fp<UInt, false, false>;

template <class UInt>
using signed_decimal_fp = decimal_fp<UInt, true, false>;

////////////////////////////////////////////////////////////////////////////////////////
// Computed cache entries.
////////////////////////////////////////////////////////////////////////////////////////

namespace detail {
    template <class FloatFormat>
    struct cache_holder;

    template <>
    struct cache_holder<ieee754_binary32> {
        using cache_entry_type = uint8::raw_type;
        static constexpr int cache_bits = 64;
        static constexpr int min_k = -31;
        static constexpr int max_k = 46;
        static constexpr cache_entry_type cache[] = {
            0x81ceb32c'4b43fcf5, 0xa2425ff7'5e14fc32, 0xcad2f7f5'359a3b3f,
            0xfd87b5f2'8300ca0e, 0x9e74d1b7'91e07e49, 0xc6120625'76589ddb,
            0xf79687ae'd3eec552, 0x9abe14cd'44753b53, 0xc16d9a00'95928a28,
            0xf1c90080'baf72cb2, 0x971da050'74da7bef, 0xbce50864'92111aeb,
            0xec1e4a7d'b69561a6, 0x9392ee8e'921d5d08, 0xb877aa32'36a4b44a,
            0xe69594be'c44de15c, 0x901d7cf7'3ab0acda, 0xb424dc35'095cd810,
            0xe12e1342'4bb40e14, 0x8cbccc09'6f5088cc, 0xafebff0b'cb24aaff,
            0xdbe6fece'bdedd5bf, 0x89705f41'36b4a598, 0xabcc7711'8461cefd,
            0xd6bf94d5'e57a42bd, 0x8637bd05'af6c69b6, 0xa7c5ac47'1b478424,
            0xd1b71758'e219652c, 0x83126e97'8d4fdf3c, 0xa3d70a3d'70a3d70b,
            0xcccccccc'cccccccd, 0x80000000'00000000, 0xa0000000'00000000,
            0xc8000000'00000000, 0xfa000000'00000000, 0x9c400000'00000000,
            0xc3500000'00000000, 0xf4240000'00000000, 0x98968000'00000000,
            0xbebc2000'00000000, 0xee6b2800'00000000, 0x9502f900'00000000,
            0xba43b740'00000000, 0xe8d4a510'00000000, 0x9184e72a'00000000,
            0xb5e620f4'80000000, 0xe35fa931'a0000000, 0x8e1bc9bf'04000000,
            0xb1a2bc2e'c5000000, 0xde0b6b3a'76400000, 0x8ac72304'89e80000,
            0xad78ebc5'ac620000, 0xd8d726b7'177a8000, 0x87867832'6eac9000,
            0xa968163f'0a57b400, 0xd3c21bce'cceda100, 0x84595161'401484a0,
            0xa56fa5b9'9019a5c8, 0xcecb8f27'f4200f3a, 0x813f3978'f8940985,
            0xa18f07d7'36b90be6, 0xc9f2c9cd'04674edf, 0xfc6f7c40'45812297,
            0x9dc5ada8'2b70b59e, 0xc5371912'364ce306, 0xf684df56'c3e01bc7,
            0x9a130b96'3a6c115d, 0xc097ce7b'c90715b4, 0xf0bdc21a'bb48db21,
            0x96769950'b50d88f5, 0xbc143fa4'e250eb32, 0xeb194f8e'1ae525fe,
            0x92efd1b8'd0cf37bf, 0xb7abc627'050305ae, 0xe596b7b0'c643c71a,
            0x8f7e32ce'7bea5c70, 0xb35dbf82'1ae4f38c, 0xe0352f62'a19e306f};
    };

    template <>
    struct cache_holder<ieee754_binary64> {
        using cache_entry_type = wuint::uint128;
        static constexpr int cache_bits = 128;
        static constexpr int min_k = -292;
        static constexpr int max_k = 326;
        static constexpr cache_entry_type cache[] = {
            {0xff77b1fc'bebcdc4f, 0x25e8e89c'13bb0f7b},
            {0x9faacf3d'f73609b1, 0x77b19161'8c54e9ad},
            {0xc795830d'75038c1d, 0xd59df5b9'ef6a2418},
            {0xf97ae3d0'd2446f25, 0x4b057328'6b44ad1e},
            {0x9becce62'836ac577, 0x4ee367f9'430aec33},
            {0xc2e801fb'244576d5, 0x229c41f7'93cda740},
            {0xf3a20279'ed56d48a, 0x6b435275'78c11110},
            {0x9845418c'345644d6, 0x830a1389'6b78aaaa},
            {0xbe5691ef'416bd60c, 0x23cc986b'c656d554},
            {0xedec366b'11c6cb8f, 0x2cbfbe86'b7ec8aa9},
            {0x94b3a202'eb1c3f39, 0x7bf7d714'32f3d6aa},
            {0xb9e08a83'a5e34f07, 0xdaf5ccd9'3fb0cc54},
            {0xe858ad24'8f5c22c9, 0xd1b3400f'8f9cff69},
            {0x91376c36'd99995be, 0x23100809'b9c21fa2},
            {0xb5854744'8ffffb2d, 0xabd40a0c'2832a78b},
            {0xe2e69915'b3fff9f9, 0x16c90c8f'323f516d},
            {0x8dd01fad'907ffc3b, 0xae3da7d9'7f6792e4},
            {0xb1442798'f49ffb4a, 0x99cd11cf'df41779d},
            {0xdd95317f'31c7fa1d, 0x40405643'd711d584},
            {0x8a7d3eef'7f1cfc52, 0x482835ea'666b2573},
            {0xad1c8eab'5ee43b66, 0xda324365'0005eed0},
            {0xd863b256'369d4a40, 0x90bed43e'40076a83},
            {0x873e4f75'e2224e68, 0x5a7744a6'e804a292},
            {0xa90de353'5aaae202, 0x711515d0'a205cb37},
            {0xd3515c28'31559a83, 0x0d5a5b44'ca873e04},
            {0x8412d999'1ed58091, 0xe858790a'fe9486c3},
            {0xa5178fff'668ae0b6, 0x626e974d'be39a873},
            {0xce5d73ff'402d98e3, 0xfb0a3d21'2dc81290},
            {0x80fa687f'881c7f8e, 0x7ce66634'bc9d0b9a},
            {0xa139029f'6a239f72, 0x1c1fffc1'ebc44e81},
            {0xc9874347'44ac874e, 0xa327ffb2'66b56221},
            {0xfbe91419'15d7a922, 0x4bf1ff9f'0062baa9},
            {0x9d71ac8f'ada6c9b5, 0x6f773fc3'603db4aa},
            {0xc4ce17b3'99107c22, 0xcb550fb4'384d21d4},
            {0xf6019da0'7f549b2b, 0x7e2a53a1'46606a49},
            {0x99c10284'4f94e0fb, 0x2eda7444'cbfc426e},
            {0xc0314325'637a1939, 0xfa911155'fefb5309},
            {0xf03d93ee'bc589f88, 0x793555ab'7eba27cb},
            {0x96267c75'35b763b5, 0x4bc1558b'2f3458df},
            {0xbbb01b92'83253ca2, 0x9eb1aaed'fb016f17},
            {0xea9c2277'23ee8bcb, 0x465e15a9'79c1cadd},
            {0x92a1958a'7675175f, 0x0bfacd89'ec191eca},
            {0xb749faed'14125d36, 0xcef980ec'671f667c},
            {0xe51c79a8'5916f484, 0x82b7e127'80e7401b},
            {0x8f31cc09'37ae58d2, 0xd1b2ecb8'b0908811},
            {0xb2fe3f0b'8599ef07, 0x861fa7e6'dcb4aa16},
            {0xdfbdcece'67006ac9, 0x67a791e0'93e1d49b},
            {0x8bd6a141'006042bd, 0xe0c8bb2c'5c6d24e1},
            {0xaecc4991'4078536d, 0x58fae9f7'73886e19},
            {0xda7f5bf5'90966848, 0xaf39a475'506a899f},
            {0x888f9979'7a5e012d, 0x6d8406c9'52429604},
            {0xaab37fd7'd8f58178, 0xc8e5087b'a6d33b84},
            {0xd5605fcd'cf32e1d6, 0xfb1e4a9a'90880a65},
            {0x855c3be0'a17fcd26, 0x5cf2eea0'9a550680},
            {0xa6b34ad8'c9dfc06f, 0xf42faa48'c0ea481f},
            {0xd0601d8e'fc57b08b, 0xf13b94da'f124da27},
            {0x823c1279'5db6ce57, 0x76c53d08'd6b70859},
            {0xa2cb1717'b52481ed, 0x54768c4b'0c64ca6f},
            {0xcb7ddcdd'a26da268, 0xa9942f5d'cf7dfd0a},
            {0xfe5d5415'0b090b02, 0xd3f93b35'435d7c4d},
            {0x9efa548d'26e5a6e1, 0xc47bc501'4a1a6db0},
            {0xc6b8e9b0'709f109a, 0x359ab641'9ca1091c},
            {0xf867241c'8cc6d4c0, 0xc30163d2'03c94b63},
            {0x9b407691'd7fc44f8, 0x79e0de63'425dcf1e},
            {0xc2109436'4dfb5636, 0x985915fc'12f542e5},
            {0xf294b943'e17a2bc4, 0x3e6f5b7b'17b2939e},
            {0x979cf3ca'6cec5b5a, 0xa705992c'eecf9c43},
            {0xbd8430bd'08277231, 0x50c6ff78'2a838354},
            {0xece53cec'4a314ebd, 0xa4f8bf56'35246429},
            {0x940f4613'ae5ed136, 0x871b7795'e136be9a},
            {0xb9131798'99f68584, 0x28e2557b'59846e40},
            {0xe757dd7e'c07426e5, 0x331aeada'2fe589d0},
            {0x9096ea6f'3848984f, 0x3ff0d2c8'5def7622},
            {0xb4bca50b'065abe63, 0x0fed077a'756b53aa},
            {0xe1ebce4d'c7f16dfb, 0xd3e84959'12c62895},
            {0x8d3360f0'9cf6e4bd, 0x64712dd7'abbbd95d},
            {0xb080392c'c4349dec, 0xbd8d794d'96aacfb4},
            {0xdca04777'f541c567, 0xecf0d7a0'fc5583a1},
            {0x89e42caa'f9491b60, 0xf41686c4'9db57245},
            {0xac5d37d5'b79b6239, 0x311c2875'c522ced6},
            {0xd77485cb'25823ac7, 0x7d633293'366b828c},
            {0x86a8d39e'f77164bc, 0xae5dff9c'02033198},
            {0xa8530886'b54dbdeb, 0xd9f57f83'0283fdfd},
            {0xd267caa8'62a12d66, 0xd072df63'c324fd7c},
            {0x8380dea9'3da4bc60, 0x4247cb9e'59f71e6e},
            {0xa4611653'8d0deb78, 0x52d9be85'f074e609},
            {0xcd795be8'70516656, 0x67902e27'6c921f8c},
            {0x806bd971'4632dff6, 0x00ba1cd8'a3db53b7},
            {0xa086cfcd'97bf97f3, 0x80e8a40e'ccd228a5},
            {0xc8a883c0'fdaf7df0, 0x6122cd12'8006b2ce},
            {0xfad2a4b1'3d1b5d6c, 0x796b8057'20085f82},
            {0x9cc3a6ee'c6311a63, 0xcbe33036'74053bb1},
            {0xc3f490aa'77bd60fc, 0xbedbfc44'11068a9d},
            {0xf4f1b4d5'15acb93b, 0xee92fb55'15482d45},
            {0x99171105'2d8bf3c5, 0x751bdd15'2d4d1c4b},
            {0xbf5cd546'78eef0b6, 0xd262d45a'78a0635e},
            {0xef340a98'172aace4, 0x86fb8971'16c87c35},
            {0x9580869f'0e7aac0e, 0xd45d35e6'ae3d4da1},
            {0xbae0a846'd2195712, 0x89748360'59cca10a},
            {0xe998d258'869facd7, 0x2bd1a438'703fc94c},
            {0x91ff8377'5423cc06, 0x7b6306a3'4627ddd0},
            {0xb67f6455'292cbf08, 0x1a3bc84c'17b1d543},
            {0xe41f3d6a'7377eeca, 0x20caba5f'1d9e4a94},
            {0x8e938662'882af53e, 0x547eb47b'7282ee9d},
            {0xb23867fb'2a35b28d, 0xe99e619a'4f23aa44},
            {0xdec681f9'f4c31f31, 0x6405fa00'e2ec94d5},
            {0x8b3c113c'38f9f37e, 0xde83bc40'8dd3dd05},
            {0xae0b158b'4738705e, 0x9624ab50'b148d446},
            {0xd98ddaee'19068c76, 0x3badd624'dd9b0958},
            {0x87f8a8d4'cfa417c9, 0xe54ca5d7'0a80e5d7},
            {0xa9f6d30a'038d1dbc, 0x5e9fcf4c'cd211f4d},
            {0xd47487cc'8470652b, 0x7647c320'00696720},
            {0x84c8d4df'd2c63f3b, 0x29ecd9f4'0041e074},
            {0xa5fb0a17'c777cf09, 0xf4681071'00525891},
            {0xcf79cc9d'b955c2cc, 0x7182148d'4066eeb5},
            {0x81ac1fe2'93d599bf, 0xc6f14cd8'48405531},
            {0xa21727db'38cb002f, 0xb8ada00e'5a506a7d},
            {0xca9cf1d2'06fdc03b, 0xa6d90811'f0e4851d},
            {0xfd442e46'88bd304a, 0x908f4a16'6d1da664},
            {0x9e4a9cec'15763e2e, 0x9a598e4e'043287ff},
            {0xc5dd4427'1ad3cdba, 0x40eff1e1'853f29fe},
            {0xf7549530'e188c128, 0xd12bee59'e68ef47d},
            {0x9a94dd3e'8cf578b9, 0x82bb74f8'301958cf},
            {0xc13a148e'3032d6e7, 0xe36a5236'3c1faf02},
            {0xf18899b1'bc3f8ca1, 0xdc44e6c3'cb279ac2},
            {0x96f5600f'15a7b7e5, 0x29ab103a'5ef8c0ba},
            {0xbcb2b812'db11a5de, 0x7415d448'f6b6f0e8},
            {0xebdf6617'91d60f56, 0x111b495b'3464ad22},
            {0x936b9fce'bb25c995, 0xcab10dd9'00beec35},
            {0xb84687c2'69ef3bfb, 0x3d5d514f'40eea743},
            {0xe65829b3'046b0afa, 0x0cb4a5a3'112a5113},
            {0x8ff71a0f'e2c2e6dc, 0x47f0e785'eaba72ac},
            {0xb3f4e093'db73a093, 0x59ed2167'65690f57},
            {0xe0f218b8'd25088b8, 0x306869c1'3ec3532d},
            {0x8c974f73'83725573, 0x1e414218'c73a13fc},
            {0xafbd2350'644eeacf, 0xe5d1929e'f90898fb},
            {0xdbac6c24'7d62a583, 0xdf45f746'b74abf3a},
            {0x894bc396'ce5da772, 0x6b8bba8c'328eb784},
            {0xab9eb47c'81f5114f, 0x066ea92f'3f326565},
            {0xd686619b'a27255a2, 0xc80a537b'0efefebe},
            {0x8613fd01'45877585, 0xbd06742c'e95f5f37},
            {0xa798fc41'96e952e7, 0x2c481138'23b73705},
            {0xd17f3b51'fca3a7a0, 0xf75a1586'2ca504c6},
            {0x82ef8513'3de648c4, 0x9a984d73'dbe722fc},
            {0xa3ab6658'0d5fdaf5, 0xc13e60d0'd2e0ebbb},
            {0xcc963fee'10b7d1b3, 0x318df905'079926a9},
            {0xffbbcfe9'94e5c61f, 0xfdf17746'497f7053},
            {0x9fd561f1'fd0f9bd3, 0xfeb6ea8b'edefa634},
            {0xc7caba6e'7c5382c8, 0xfe64a52e'e96b8fc1},
            {0xf9bd690a'1b68637b, 0x3dfdce7a'a3c673b1},
            {0x9c1661a6'51213e2d, 0x06bea10c'a65c084f},
            {0xc31bfa0f'e5698db8, 0x486e494f'cff30a63},
            {0xf3e2f893'dec3f126, 0x5a89dba3'c3efccfb},
            {0x986ddb5c'6b3a76b7, 0xf8962946'5a75e01d},
            {0xbe895233'86091465, 0xf6bbb397'f1135824},
            {0xee2ba6c0'678b597f, 0x746aa07d'ed582e2d},
            {0x94db4838'40b717ef, 0xa8c2a44e'b4571cdd},
            {0xba121a46'50e4ddeb, 0x92f34d62'616ce414},
            {0xe896a0d7'e51e1566, 0x77b020ba'f9c81d18},
            {0x915e2486'ef32cd60, 0x0ace1474'dc1d122f},
            {0xb5b5ada8'aaff80b8, 0x0d819992'132456bb},
            {0xe3231912'd5bf60e6, 0x10e1fff6'97ed6c6a},
            {0x8df5efab'c5979c8f, 0xca8d3ffa'1ef463c2},
            {0xb1736b96'b6fd83b3, 0xbd308ff8'a6b17cb3},
            {0xddd0467c'64bce4a0, 0xac7cb3f6'd05ddbdf},
            {0x8aa22c0d'bef60ee4, 0x6bcdf07a'423aa96c},
            {0xad4ab711'2eb3929d, 0x86c16c98'd2c953c7},
            {0xd89d64d5'7a607744, 0xe871c7bf'077ba8b8},
            {0x87625f05'6c7c4a8b, 0x11471cd7'64ad4973},
            {0xa93af6c6'c79b5d2d, 0xd598e40d'3dd89bd0},
            {0xd389b478'79823479, 0x4aff1d10'8d4ec2c4},
            {0x843610cb'4bf160cb, 0xcedf722a'585139bb},
            {0xa54394fe'1eedb8fe, 0xc2974eb4'ee658829},
            {0xce947a3d'a6a9273e, 0x733d2262'29feea33},
            {0x811ccc66'8829b887, 0x0806357d'5a3f5260},
            {0xa163ff80'2a3426a8, 0xca07c2dc'b0cf26f8},
            {0xc9bcff60'34c13052, 0xfc89b393'dd02f0b6},
            {0xfc2c3f38'41f17c67, 0xbbac2078'd443ace3},
            {0x9d9ba783'2936edc0, 0xd54b944b'84aa4c0e},
            {0xc5029163'f384a931, 0x0a9e795e'65d4df12},
            {0xf64335bc'f065d37d, 0x4d4617b5'ff4a16d6},
            {0x99ea0196'163fa42e, 0x504bced1'bf8e4e46},
            {0xc06481fb'9bcf8d39, 0xe45ec286'2f71e1d7},
            {0xf07da27a'82c37088, 0x5d767327'bb4e5a4d},
            {0x964e858c'91ba2655, 0x3a6a07f8'd510f870},
            {0xbbe226ef'b628afea, 0x890489f7'0a55368c},
            {0xeadab0ab'a3b2dbe5, 0x2b45ac74'ccea842f},
            {0x92c8ae6b'464fc96f, 0x3b0b8bc9'0012929e},
            {0xb77ada06'17e3bbcb, 0x09ce6ebb'40173745},
            {0xe5599087'9ddcaabd, 0xcc420a6a'101d0516},
            {0x8f57fa54'c2a9eab6, 0x9fa94682'4a12232e},
            {0xb32df8e9'f3546564, 0x47939822'dc96abfa},
            {0xdff97724'70297ebd, 0x59787e2b'93bc56f8},
            {0x8bfbea76'c619ef36, 0x57eb4edb'3c55b65b},
            {0xaefae514'77a06b03, 0xede62292'0b6b23f2},
            {0xdab99e59'958885c4, 0xe95fab36'8e45ecee},
            {0x88b402f7'fd75539b, 0x11dbcb02'18ebb415},
            {0xaae103b5'fcd2a881, 0xd652bdc2'9f26a11a},
            {0xd59944a3'7c0752a2, 0x4be76d33'46f04960},
            {0x857fcae6'2d8493a5, 0x6f70a440'0c562ddc},
            {0xa6dfbd9f'b8e5b88e, 0xcb4ccd50'0f6bb953},
            {0xd097ad07'a71f26b2, 0x7e2000a4'1346a7a8},
            {0x825ecc24'c873782f, 0x8ed40066'8c0c28c9},
            {0xa2f67f2d'fa90563b, 0x72890080'2f0f32fb},
            {0xcbb41ef9'79346bca, 0x4f2b40a0'3ad2ffba},
            {0xfea126b7'd78186bc, 0xe2f610c8'4987bfa9},
            {0x9f24b832'e6b0f436, 0x0dd9ca7d'2df4d7ca},
            {0xc6ede63f'a05d3143, 0x91503d1c'79720dbc},
            {0xf8a95fcf'88747d94, 0x75a44c63'97ce912b},
            {0x9b69dbe1'b548ce7c, 0xc986afbe'3ee11abb},
            {0xc24452da'229b021b, 0xfbe85bad'ce996169},
            {0xf2d56790'ab41c2a2, 0xfae27299'423fb9c4},
            {0x97c560ba'6b0919a5, 0xdccd879f'c967d41b},
            {0xbdb6b8e9'05cb600f, 0x5400e987'bbc1c921},
            {0xed246723'473e3813, 0x290123e9'aab23b69},
            {0x9436c076'0c86e30b, 0xf9a0b672'0aaf6522},
            {0xb9447093'8fa89bce, 0xf808e40e'8d5b3e6a},
            {0xe7958cb8'7392c2c2, 0xb60b1d12'30b20e05},
            {0x90bd77f3'483bb9b9, 0xb1c6f22b'5e6f48c3},
            {0xb4ecd5f0'1a4aa828, 0x1e38aeb6'360b1af4},
            {0xe2280b6c'20dd5232, 0x25c6da63'c38de1b1},
            {0x8d590723'948a535f, 0x579c487e'5a38ad0f},
            {0xb0af48ec'79ace837, 0x2d835a9d'f0c6d852},
            {0xdcdb1b27'98182244, 0xf8e43145'6cf88e66},
            {0x8a08f0f8'bf0f156b, 0x1b8e9ecb'641b5900},
            {0xac8b2d36'eed2dac5, 0xe272467e'3d222f40},
            {0xd7adf884'aa879177, 0x5b0ed81d'cc6abb10},
            {0x86ccbb52'ea94baea, 0x98e94712'9fc2b4ea},
            {0xa87fea27'a539e9a5, 0x3f2398d7'47b36225},
            {0xd29fe4b1'8e88640e, 0x8eec7f0d'19a03aae},
            {0x83a3eeee'f9153e89, 0x1953cf68'300424ad},
            {0xa48ceaaa'b75a8e2b, 0x5fa8c342'3c052dd8},
            {0xcdb02555'653131b6, 0x3792f412'cb06794e},
            {0x808e1755'5f3ebf11, 0xe2bbd88b'bee40bd1},
            {0xa0b19d2a'b70e6ed6, 0x5b6aceae'ae9d0ec5},
            {0xc8de0475'64d20a8b, 0xf245825a'5a445276},
            {0xfb158592'be068d2e, 0xeed6e2f0'f0d56713},
            {0x9ced737b'b6c4183d, 0x55464dd6'9685606c},
            {0xc428d05a'a4751e4c, 0xaa97e14c'3c26b887},
            {0xf5330471'4d9265df, 0xd53dd99f'4b3066a9},
            {0x993fe2c6'd07b7fab, 0xe546a803'8efe402a},
            {0xbf8fdb78'849a5f96, 0xde985204'72bdd034},
            {0xef73d256'a5c0f77c, 0x963e6685'8f6d4441},
            {0x95a86376'27989aad, 0xdde70013'79a44aa9},
            {0xbb127c53'b17ec159, 0x5560c018'580d5d53},
            {0xe9d71b68'9dde71af, 0xaab8f01e'6e10b4a7},
            {0x92267121'62ab070d, 0xcab39613'04ca70e9},
            {0xb6b00d69'bb55c8d1, 0x3d607b97'c5fd0d23},
            {0xe45c10c4'2a2b3b05, 0x8cb89a7d'b77c506b},
            {0x8eb98a7a'9a5b04e3, 0x77f3608e'92adb243},
            {0xb267ed19'40f1c61c, 0x55f038b2'37591ed4},
            {0xdf01e85f'912e37a3, 0x6b6c46de'c52f6689},
            {0x8b61313b'babce2c6, 0x2323ac4b'3b3da016},
            {0xae397d8a'a96c1b77, 0xabec975e'0a0d081b},
            {0xd9c7dced'53c72255, 0x96e7bd35'8c904a22},
            {0x881cea14'545c7575, 0x7e50d641'77da2e55},
            {0xaa242499'697392d2, 0xdde50bd1'd5d0b9ea},
            {0xd4ad2dbf'c3d07787, 0x955e4ec6'4b44e865},
            {0x84ec3c97'da624ab4, 0xbd5af13b'ef0b113f},
            {0xa6274bbd'd0fadd61, 0xecb1ad8a'eacdd58f},
            {0xcfb11ead'453994ba, 0x67de18ed'a5814af3},
            {0x81ceb32c'4b43fcf4, 0x80eacf94'8770ced8},
            {0xa2425ff7'5e14fc31, 0xa1258379'a94d028e},
            {0xcad2f7f5'359a3b3e, 0x096ee458'13a04331},
            {0xfd87b5f2'8300ca0d, 0x8bca9d6e'188853fd},
            {0x9e74d1b7'91e07e48, 0x775ea264'cf55347e},
            {0xc6120625'76589dda, 0x95364afe'032a819e},
            {0xf79687ae'd3eec551, 0x3a83ddbd'83f52205},
            {0x9abe14cd'44753b52, 0xc4926a96'72793543},
            {0xc16d9a00'95928a27, 0x75b7053c'0f178294},
            {0xf1c90080'baf72cb1, 0x5324c68b'12dd6339},
            {0x971da050'74da7bee, 0xd3f6fc16'ebca5e04},
            {0xbce50864'92111aea, 0x88f4bb1c'a6bcf585},
            {0xec1e4a7d'b69561a5, 0x2b31e9e3'd06c32e6},
            {0x9392ee8e'921d5d07, 0x3aff322e'62439fd0},
            {0xb877aa32'36a4b449, 0x09befeb9'fad487c3},
            {0xe69594be'c44de15b, 0x4c2ebe68'7989a9b4},
            {0x901d7cf7'3ab0acd9, 0x0f9d3701'4bf60a11},
            {0xb424dc35'095cd80f, 0x538484c1'9ef38c95},
            {0xe12e1342'4bb40e13, 0x2865a5f2'06b06fba},
            {0x8cbccc09'6f5088cb, 0xf93f87b7'442e45d4},
            {0xafebff0b'cb24aafe, 0xf78f69a5'1539d749},
            {0xdbe6fece'bdedd5be, 0xb573440e'5a884d1c},
            {0x89705f41'36b4a597, 0x31680a88'f8953031},
            {0xabcc7711'8461cefc, 0xfdc20d2b'36ba7c3e},
            {0xd6bf94d5'e57a42bc, 0x3d329076'04691b4d},
            {0x8637bd05'af6c69b5, 0xa63f9a49'c2c1b110},
            {0xa7c5ac47'1b478423, 0x0fcf80dc'33721d54},
            {0xd1b71758'e219652b, 0xd3c36113'404ea4a9},
            {0x83126e97'8d4fdf3b, 0x645a1cac'083126ea},
            {0xa3d70a3d'70a3d70a, 0x3d70a3d7'0a3d70a4},
            {0xcccccccc'cccccccc, 0xcccccccc'cccccccd},
            {0x80000000'00000000, 0x00000000'00000000},
            {0xa0000000'00000000, 0x00000000'00000000},
            {0xc8000000'00000000, 0x00000000'00000000},
            {0xfa000000'00000000, 0x00000000'00000000},
            {0x9c400000'00000000, 0x00000000'00000000},
            {0xc3500000'00000000, 0x00000000'00000000},
            {0xf4240000'00000000, 0x00000000'00000000},
            {0x98968000'00000000, 0x00000000'00000000},
            {0xbebc2000'00000000, 0x00000000'00000000},
            {0xee6b2800'00000000, 0x00000000'00000000},
            {0x9502f900'00000000, 0x00000000'00000000},
            {0xba43b740'00000000, 0x00000000'00000000},
            {0xe8d4a510'00000000, 0x00000000'00000000},
            {0x9184e72a'00000000, 0x00000000'00000000},
            {0xb5e620f4'80000000, 0x00000000'00000000},
            {0xe35fa931'a0000000, 0x00000000'00000000},
            {0x8e1bc9bf'04000000, 0x00000000'00000000},
            {0xb1a2bc2e'c5000000, 0x00000000'00000000},
            {0xde0b6b3a'76400000, 0x00000000'00000000},
            {0x8ac72304'89e80000, 0x00000000'00000000},
            {0xad78ebc5'ac620000, 0x00000000'00000000},
            {0xd8d726b7'177a8000, 0x00000000'00000000},
            {0x87867832'6eac9000, 0x00000000'00000000},
            {0xa968163f'0a57b400, 0x00000000'00000000},
            {0xd3c21bce'cceda100, 0x00000000'00000000},
            {0x84595161'401484a0, 0x00000000'00000000},
            {0xa56fa5b9'9019a5c8, 0x00000000'00000000},
            {0xcecb8f27'f4200f3a, 0x00000000'00000000},
            {0x813f3978'f8940984, 0x40000000'00000000},
            {0xa18f07d7'36b90be5, 0x50000000'00000000},
            {0xc9f2c9cd'04674ede, 0xa4000000'00000000},
            {0xfc6f7c40'45812296, 0x4d000000'00000000},
            {0x9dc5ada8'2b70b59d, 0xf0200000'00000000},
            {0xc5371912'364ce305, 0x6c280000'00000000},
            {0xf684df56'c3e01bc6, 0xc7320000'00000000},
            {0x9a130b96'3a6c115c, 0x3c7f4000'00000000},
            {0xc097ce7b'c90715b3, 0x4b9f1000'00000000},
            {0xf0bdc21a'bb48db20, 0x1e86d400'00000000},
            {0x96769950'b50d88f4, 0x13144480'00000000},
            {0xbc143fa4'e250eb31, 0x17d955a0'00000000},
            {0xeb194f8e'1ae525fd, 0x5dcfab08'00000000},
            {0x92efd1b8'd0cf37be, 0x5aa1cae5'00000000},
            {0xb7abc627'050305ad, 0xf14a3d9e'40000000},
            {0xe596b7b0'c643c719, 0x6d9ccd05'd0000000},
            {0x8f7e32ce'7bea5c6f, 0xe4820023'a2000000},
            {0xb35dbf82'1ae4f38b, 0xdda2802c'8a800000},
            {0xe0352f62'a19e306e, 0xd50b2037'ad200000},
            {0x8c213d9d'a502de45, 0x4526f422'cc340000},
            {0xaf298d05'0e4395d6, 0x9670b12b'7f410000},
            {0xdaf3f046'51d47b4c, 0x3c0cdd76'5f114000},
            {0x88d8762b'f324cd0f, 0xa5880a69'fb6ac800},
            {0xab0e93b6'efee0053, 0x8eea0d04'7a457a00},
            {0xd5d238a4'abe98068, 0x72a49045'98d6d880},
            {0x85a36366'eb71f041, 0x47a6da2b'7f864750},
            {0xa70c3c40'a64e6c51, 0x999090b6'5f67d924},
            {0xd0cf4b50'cfe20765, 0xfff4b4e3'f741cf6d},
            {0x82818f12'81ed449f, 0xbff8f10e'7a8921a5},
            {0xa321f2d7'226895c7, 0xaff72d52'192b6a0e},
            {0xcbea6f8c'eb02bb39, 0x9bf4f8a6'9f764491},
            {0xfee50b70'25c36a08, 0x02f236d0'4753d5b5},
            {0x9f4f2726'179a2245, 0x01d76242'2c946591},
            {0xc722f0ef'9d80aad6, 0x424d3ad2'b7b97ef6},
            {0xf8ebad2b'84e0d58b, 0xd2e08987'65a7deb3},
            {0x9b934c3b'330c8577, 0x63cc55f4'9f88eb30},
            {0xc2781f49'ffcfa6d5, 0x3cbf6b71'c76b25fc},
            {0xf316271c'7fc3908a, 0x8bef464e'3945ef7b},
            {0x97edd871'cfda3a56, 0x97758bf0'e3cbb5ad},
            {0xbde94e8e'43d0c8ec, 0x3d52eeed'1cbea318},
            {0xed63a231'd4c4fb27, 0x4ca7aaa8'63ee4bde},
            {0x945e455f'24fb1cf8, 0x8fe8caa9'3e74ef6b},
            {0xb975d6b6'ee39e436, 0xb3e2fd53'8e122b45},
            {0xe7d34c64'a9c85d44, 0x60dbbca8'7196b617},
            {0x90e40fbe'ea1d3a4a, 0xbc8955e9'46fe31ce},
            {0xb51d13ae'a4a488dd, 0x6babab63'98bdbe42},
            {0xe264589a'4dcdab14, 0xc696963c'7eed2dd2},
            {0x8d7eb760'70a08aec, 0xfc1e1de5'cf543ca3},
            {0xb0de6538'8cc8ada8, 0x3b25a55f'43294bcc},
            {0xdd15fe86'affad912, 0x49ef0eb7'13f39ebf},
            {0x8a2dbf14'2dfcc7ab, 0x6e356932'6c784338},
            {0xacb92ed9'397bf996, 0x49c2c37f'07965405},
            {0xd7e77a8f'87daf7fb, 0xdc33745e'c97be907},
            {0x86f0ac99'b4e8dafd, 0x69a028bb'3ded71a4},
            {0xa8acd7c0'222311bc, 0xc40832ea'0d68ce0d},
            {0xd2d80db0'2aabd62b, 0xf50a3fa4'90c30191},
            {0x83c7088e'1aab65db, 0x792667c6'da79e0fb},
            {0xa4b8cab1'a1563f52, 0x577001b8'91185939},
            {0xcde6fd5e'09abcf26, 0xed4c0226'b55e6f87},
            {0x80b05e5a'c60b6178, 0x544f8158'315b05b5},
            {0xa0dc75f1'778e39d6, 0x696361ae'3db1c722},
            {0xc913936d'd571c84c, 0x03bc3a19'cd1e38ea},
            {0xfb587849'4ace3a5f, 0x04ab48a0'4065c724},
            {0x9d174b2d'cec0e47b, 0x62eb0d64'283f9c77},
            {0xc45d1df9'42711d9a, 0x3ba5d0bd'324f8395},
            {0xf5746577'930d6500, 0xca8f44ec'7ee3647a},
            {0x9968bf6a'bbe85f20, 0x7e998b13'cf4e1ecc},
            {0xbfc2ef45'6ae276e8, 0x9e3fedd8'c321a67f},
            {0xefb3ab16'c59b14a2, 0xc5cfe94e'f3ea101f},
            {0x95d04aee'3b80ece5, 0xbba1f1d1'58724a13},
            {0xbb445da9'ca61281f, 0x2a8a6e45'ae8edc98},
            {0xea157514'3cf97226, 0xf52d09d7'1a3293be},
            {0x924d692c'a61be758, 0x593c2626'705f9c57},
            {0xb6e0c377'cfa2e12e, 0x6f8b2fb0'0c77836d},
            {0xe498f455'c38b997a, 0x0b6dfb9c'0f956448},
            {0x8edf98b5'9a373fec, 0x4724bd41'89bd5ead},
            {0xb2977ee3'00c50fe7, 0x58edec91'ec2cb658},
            {0xdf3d5e9b'c0f653e1, 0x2f2967b6'6737e3ee},
            {0x8b865b21'5899f46c, 0xbd79e0d2'0082ee75},
            {0xae67f1e9'aec07187, 0xecd85906'80a3aa12},
            {0xda01ee64'1a708de9, 0xe80e6f48'20cc9496},
            {0x884134fe'908658b2, 0x3109058d'147fdcde},
            {0xaa51823e'34a7eede, 0xbd4b46f0'599fd416},
            {0xd4e5e2cd'c1d1ea96, 0x6c9e18ac'7007c91b},
            {0x850fadc0'9923329e, 0x03e2cf6b'c604ddb1},
            {0xa6539930'bf6bff45, 0x84db8346'b786151d},
            {0xcfe87f7c'ef46ff16, 0xe6126418'65679a64},
            {0x81f14fae'158c5f6e, 0x4fcb7e8f'3f60c07f},
            {0xa26da399'9aef7749, 0xe3be5e33'0f38f09e},
            {0xcb090c80'01ab551c, 0x5cadf5bf'd3072cc6},
            {0xfdcb4fa0'02162a63, 0x73d9732f'c7c8f7f7},
            {0x9e9f11c4'014dda7e, 0x2867e7fd'dcdd9afb},
            {0xc646d635'01a1511d, 0xb281e1fd'541501b9},
            {0xf7d88bc2'4209a565, 0x1f225a7c'a91a4227},
            {0x9ae75759'6946075f, 0x3375788d'e9b06959},
            {0xc1a12d2f'c3978937, 0x0052d6b1'641c83af},
            {0xf209787b'b47d6b84, 0xc0678c5d'bd23a49b},
            {0x9745eb4d'50ce6332, 0xf840b7ba'963646e1},
            {0xbd176620'a501fbff, 0xb650e5a9'3bc3d899},
            {0xec5d3fa8'ce427aff, 0xa3e51f13'8ab4cebf},
            {0x93ba47c9'80e98cdf, 0xc66f336c'36b10138},
            {0xb8a8d9bb'e123f017, 0xb80b0047'445d4185},
            {0xe6d3102a'd96cec1d, 0xa60dc059'157491e6},
            {0x9043ea1a'c7e41392, 0x87c89837'ad68db30},
            {0xb454e4a1'79dd1877, 0x29babe45'98c311fc},
            {0xe16a1dc9'd8545e94, 0xf4296dd6'fef3d67b},
            {0x8ce2529e'2734bb1d, 0x1899e4a6'5f58660d},
            {0xb01ae745'b101e9e4, 0x5ec05dcf'f72e7f90},
            {0xdc21a117'1d42645d, 0x76707543'f4fa1f74},
            {0x899504ae'72497eba, 0x6a06494a'791c53a9},
            {0xabfa45da'0edbde69, 0x0487db9d'17636893},
            {0xd6f8d750'9292d603, 0x45a9d284'5d3c42b7},
            {0x865b8692'5b9bc5c2, 0x0b8a2392'ba45a9b3},
            {0xa7f26836'f282b732, 0x8e6cac77'68d7141f},
            {0xd1ef0244'af2364ff, 0x3207d795'430cd927},
            {0x8335616a'ed761f1f, 0x7f44e6bd'49e807b9},
            {0xa402b9c5'a8d3a6e7, 0x5f16206c'9c6209a7},
            {0xcd036837'130890a1, 0x36dba887'c37a8c10},
            {0x80222122'6be55a64, 0xc2494954'da2c978a},
            {0xa02aa96b'06deb0fd, 0xf2db9baa'10b7bd6d},
            {0xc83553c5'c8965d3d, 0x6f928294'94e5acc8},
            {0xfa42a8b7'3abbf48c, 0xcb772339'ba1f17fa},
            {0x9c69a972'84b578d7, 0xff2a7604'14536efc},
            {0xc38413cf'25e2d70d, 0xfef51385'19684abb},
            {0xf46518c2'ef5b8cd1, 0x7eb25866'5fc25d6a},
            {0x98bf2f79'd5993802, 0xef2f773f'fbd97a62},
            {0xbeeefb58'4aff8603, 0xaafb550f'facfd8fb},
            {0xeeaaba2e'5dbf6784, 0x95ba2a53'f983cf39},
            {0x952ab45c'fa97a0b2, 0xdd945a74'7bf26184},
            {0xba756174'393d88df, 0x94f97111'9aeef9e5},
            {0xe912b9d1'478ceb17, 0x7a37cd56'01aab85e},
            {0x91abb422'ccb812ee, 0xac62e055'c10ab33b},
            {0xb616a12b'7fe617aa, 0x577b986b'314d600a},
            {0xe39c4976'5fdf9d94, 0xed5a7e85'fda0b80c},
            {0x8e41ade9'fbebc27d, 0x14588f13'be847308},
            {0xb1d21964'7ae6b31c, 0x596eb2d8'ae258fc9},
            {0xde469fbd'99a05fe3, 0x6fca5f8e'd9aef3bc},
            {0x8aec23d6'80043bee, 0x25de7bb9'480d5855},
            {0xada72ccc'20054ae9, 0xaf561aa7'9a10ae6b},
            {0xd910f7ff'28069da4, 0x1b2ba151'8094da05},
            {0x87aa9aff'79042286, 0x90fb44d2'f05d0843},
            {0xa99541bf'57452b28, 0x353a1607'ac744a54},
            {0xd3fa922f'2d1675f2, 0x42889b89'97915ce9},
            {0x847c9b5d'7c2e09b7, 0x69956135'febada12},
            {0xa59bc234'db398c25, 0x43fab983'7e699096},
            {0xcf02b2c2'1207ef2e, 0x94f967e4'5e03f4bc},
            {0x8161afb9'4b44f57d, 0x1d1be0ee'bac278f6},
            {0xa1ba1ba7'9e1632dc, 0x6462d92a'69731733},
            {0xca28a291'859bbf93, 0x7d7b8f75'03cfdcff},
            {0xfcb2cb35'e702af78, 0x5cda7352'44c3d43f},
            {0x9defbf01'b061adab, 0x3a088813'6afa64a8},
            {0xc56baec2'1c7a1916, 0x088aaa18'45b8fdd1},
            {0xf6c69a72'a3989f5b, 0x8aad549e'57273d46},
            {0x9a3c2087'a63f6399, 0x36ac54e2'f678864c},
            {0xc0cb28a9'8fcf3c7f, 0x84576a1b'b416a7de},
            {0xf0fdf2d3'f3c30b9f, 0x656d44a2'a11c51d6},
            {0x969eb7c4'7859e743, 0x9f644ae5'a4b1b326},
            {0xbc4665b5'96706114, 0x873d5d9f'0dde1fef},
            {0xeb57ff22'fc0c7959, 0xa90cb506'd155a7eb},
            {0x9316ff75'dd87cbd8, 0x09a7f124'42d588f3},
            {0xb7dcbf53'54e9bece, 0x0c11ed6d'538aeb30},
            {0xe5d3ef28'2a242e81, 0x8f1668c8'a86da5fb},
            {0x8fa47579'1a569d10, 0xf96e017d'694487bd},
            {0xb38d92d7'60ec4455, 0x37c981dc'c395a9ad},
            {0xe070f78d'3927556a, 0x85bbe253'f47b1418},
            {0x8c469ab8'43b89562, 0x93956d74'78ccec8f},
            {0xaf584166'54a6babb, 0x387ac8d1'970027b3},
            {0xdb2e51bf'e9d0696a, 0x06997b05'fcc0319f},
            {0x88fcf317'f22241e2, 0x441fece3'bdf81f04},
            {0xab3c2fdd'eeaad25a, 0xd527e81c'ad7626c4},
            {0xd60b3bd5'6a5586f1, 0x8a71e223'd8d3b075},
            {0x85c70565'62757456, 0xf6872d56'67844e4a},
            {0xa738c6be'bb12d16c, 0xb428f8ac'016561dc},
            {0xd106f86e'69d785c7, 0xe13336d7'01beba53},
            {0x82a45b45'0226b39c, 0xecc00246'61173474},
            {0xa34d7216'42b06084, 0x27f002d7'f95d0191},
            {0xcc20ce9b'd35c78a5, 0x31ec038d'f7b441f5},
            {0xff290242'c83396ce, 0x7e670471'75a15272},
            {0x9f79a169'bd203e41, 0x0f0062c6'e984d387},
            {0xc75809c4'2c684dd1, 0x52c07b78'a3e60869},
            {0xf92e0c35'37826145, 0xa7709a56'ccdf8a83},
            {0x9bbcc7a1'42b17ccb, 0x88a66076'400bb692},
            {0xc2abf989'935ddbfe, 0x6acff893'd00ea436},
            {0xf356f7eb'f83552fe, 0x0583f6b8'c4124d44},
            {0x98165af3'7b2153de, 0xc3727a33'7a8b704b},
            {0xbe1bf1b0'59e9a8d6, 0x744f18c0'592e4c5d},
            {0xeda2ee1c'7064130c, 0x1162def0'6f79df74},
            {0x9485d4d1'c63e8be7, 0x8addcb56'45ac2ba9},
            {0xb9a74a06'37ce2ee1, 0x6d953e2b'd7173693},
            {0xe8111c87'c5c1ba99, 0xc8fa8db6'ccdd0438},
            {0x910ab1d4'db9914a0, 0x1d9c9892'400a22a3},
            {0xb54d5e4a'127f59c8, 0x2503beb6'd00cab4c},
            {0xe2a0b5dc'971f303a, 0x2e44ae64'840fd61e},
            {0x8da471a9'de737e24, 0x5ceaecfe'd289e5d3},
            {0xb10d8e14'56105dad, 0x7425a83e'872c5f48},
            {0xdd50f199'6b947518, 0xd12f124e'28f7771a},
            {0x8a5296ff'e33cc92f, 0x82bd6b70'd99aaa70},
            {0xace73cbf'dc0bfb7b, 0x636cc64d'1001550c},
            {0xd8210bef'd30efa5a, 0x3c47f7e0'5401aa4f},
            {0x8714a775'e3e95c78, 0x65acfaec'34810a72},
            {0xa8d9d153'5ce3b396, 0x7f1839a7'41a14d0e},
            {0xd31045a8'341ca07c, 0x1ede4811'1209a051},
            {0x83ea2b89'2091e44d, 0x934aed0a'ab460433},
            {0xa4e4b66b'68b65d60, 0xf81da84d'56178540},
            {0xce1de406'42e3f4b9, 0x36251260'ab9d668f},
            {0x80d2ae83'e9ce78f3, 0xc1d72b7c'6b42601a},
            {0xa1075a24'e4421730, 0xb24cf65b'8612f820},
            {0xc94930ae'1d529cfc, 0xdee033f2'6797b628},
            {0xfb9b7cd9'a4a7443c, 0x169840ef'017da3b2},
            {0x9d412e08'06e88aa5, 0x8e1f2895'60ee864f},
            {0xc491798a'08a2ad4e, 0xf1a6f2ba'b92a27e3},
            {0xf5b5d7ec'8acb58a2, 0xae10af69'6774b1dc},
            {0x9991a6f3'd6bf1765, 0xacca6da1'e0a8ef2a},
            {0xbff610b0'cc6edd3f, 0x17fd090a'58d32af4},
            {0xeff394dc'ff8a948e, 0xddfc4b4c'ef07f5b1},
            {0x95f83d0a'1fb69cd9, 0x4abdaf10'1564f98f},
            {0xbb764c4c'a7a4440f, 0x9d6d1ad4'1abe37f2},
            {0xea53df5f'd18d5513, 0x84c86189'216dc5ee},
            {0x92746b9b'e2f8552c, 0x32fd3cf5'b4e49bb5},
            {0xb7118682'dbb66a77, 0x3fbc8c33'221dc2a2},
            {0xe4d5e823'92a40515, 0x0fabaf3f'eaa5334b},
            {0x8f05b116'3ba6832d, 0x29cb4d87'f2a7400f},
            {0xb2c71d5b'ca9023f8, 0x743e20e9'ef511013},
            {0xdf78e4b2'bd342cf6, 0x914da924'6b255417},
            {0x8bab8eef'b6409c1a, 0x1ad089b6'c2f7548f},
            {0xae9672ab'a3d0c320, 0xa184ac24'73b529b2},
            {0xda3c0f56'8cc4f3e8, 0xc9e5d72d'90a2741f},
            {0x88658996'17fb1871, 0x7e2fa67c'7a658893},
            {0xaa7eebfb'9df9de8d, 0xddbb901b'98feeab8},
            {0xd51ea6fa'85785631, 0x552a7422'7f3ea566},
            {0x8533285c'936b35de, 0xd53a8895'8f872760},
            {0xa67ff273'b8460356, 0x8a892aba'f368f138},
            {0xd01fef10'a657842c, 0x2d2b7569'b0432d86},
            {0x8213f56a'67f6b29b, 0x9c3b2962'0e29fc74},
            {0xa298f2c5'01f45f42, 0x8349f3ba'91b47b90},
            {0xcb3f2f76'42717713, 0x241c70a9'36219a74},
            {0xfe0efb53'd30dd4d7, 0xed238cd3'83aa0111},
            {0x9ec95d14'63e8a506, 0xf4363804'324a40ab},
            {0xc67bb459'7ce2ce48, 0xb143c605'3edcd0d6},
            {0xf81aa16f'dc1b81da, 0xdd94b786'8e94050b},
            {0x9b10a4e5'e9913128, 0xca7cf2b4'191c8327},
            {0xc1d4ce1f'63f57d72, 0xfd1c2f61'1f63a3f1},
            {0xf24a01a7'3cf2dccf, 0xbc633b39'673c8ced},
            {0x976e4108'8617ca01, 0xd5be0503'e085d814},
            {0xbd49d14a'a79dbc82, 0x4b2d8644'd8a74e19},
            {0xec9c459d'51852ba2, 0xddf8e7d6'0ed1219f},
            {0x93e1ab82'52f33b45, 0xcabb90e5'c942b504},
            {0xb8da1662'e7b00a17, 0x3d6a751f'3b936244},
            {0xe7109bfb'a19c0c9d, 0x0cc51267'0a783ad5},
            {0x906a617d'450187e2, 0x27fb2b80'668b24c6},
            {0xb484f9dc'9641e9da, 0xb1f9f660'802dedf7},
            {0xe1a63853'bbd26451, 0x5e7873f8'a0396974},
            {0x8d07e334'55637eb2, 0xdb0b487b'6423e1e9},
            {0xb049dc01'6abc5e5f, 0x91ce1a9a'3d2cda63},
            {0xdc5c5301'c56b75f7, 0x7641a140'cc7810fc},
            {0x89b9b3e1'1b6329ba, 0xa9e904c8'7fcb0a9e},
            {0xac2820d9'623bf429, 0x546345fa'9fbdcd45},
            {0xd732290f'bacaf133, 0xa97c1779'47ad4096},
            {0x867f59a9'd4bed6c0, 0x49ed8eab'cccc485e},
            {0xa81f3014'49ee8c70, 0x5c68f256'bfff5a75},
            {0xd226fc19'5c6a2f8c, 0x73832eec'6fff3112},
            {0x83585d8f'd9c25db7, 0xc831fd53'c5ff7eac},
            {0xa42e74f3'd032f525, 0xba3e7ca8'b77f5e56},
            {0xcd3a1230'c43fb26f, 0x28ce1bd2'e55f35ec},
            {0x80444b5e'7aa7cf85, 0x7980d163'cf5b81b4},
            {0xa0555e36'1951c366, 0xd7e105bc'c3326220},
            {0xc86ab5c3'9fa63440, 0x8dd9472b'f3fefaa8},
            {0xfa856334'878fc150, 0xb14f98f6'f0feb952},
            {0x9c935e00'd4b9d8d2, 0x6ed1bf9a'569f33d4},
            {0xc3b83581'09e84f07, 0x0a862f80'ec4700c9},
            {0xf4a642e1'4c6262c8, 0xcd27bb61'2758c0fb},
            {0x98e7e9cc'cfbd7dbd, 0x8038d51c'b897789d},
            {0xbf21e440'03acdd2c, 0xe0470a63'e6bd56c4},
            {0xeeea5d50'04981478, 0x1858ccfc'e06cac75},
            {0x95527a52'02df0ccb, 0x0f37801e'0c43ebc9},
            {0xbaa718e6'8396cffd, 0xd3056025'8f54e6bb},
            {0xe950df20'247c83fd, 0x47c6b82e'f32a206a},
            {0x91d28b74'16cdd27e, 0x4cdc331d'57fa5442},
            {0xb6472e51'1c81471d, 0xe0133fe4'adf8e953},
            {0xe3d8f9e5'63a198e5, 0x58180fdd'd97723a7},
            {0x8e679c2f'5e44ff8f, 0x570f09ea'a7ea7649},
            {0xb201833b'35d63f73, 0x2cd2cc65'51e513db},
            {0xde81e40a'034bcf4f, 0xf8077f7e'a65e58d2},
            {0x8b112e86'420f6191, 0xfb04afaf'27faf783},
            {0xadd57a27'd29339f6, 0x79c5db9a'f1f9b564},
            {0xd94ad8b1'c7380874, 0x18375281'ae7822bd},
            {0x87cec76f'1c830548, 0x8f229391'0d0b15b6},
            {0xa9c2794a'e3a3c69a, 0xb2eb3875'504ddb23},
            {0xd433179d'9c8cb841, 0x5fa60692'a46151ec},
            {0x849feec2'81d7f328, 0xdbc7c41b'a6bcd334},
            {0xa5c7ea73'224deff3, 0x12b9b522'906c0801},
            {0xcf39e50f'eae16bef, 0xd768226b'34870a01},
            {0x81842f29'f2cce375, 0xe6a11583'00d46641},
            {0xa1e53af4'6f801c53, 0x60495ae3'c1097fd1},
            {0xca5e89b1'8b602368, 0x385bb19c'b14bdfc5},
            {0xfcf62c1d'ee382c42, 0x46729e03'dd9ed7b6},
            {0x9e19db92'b4e31ba9, 0x6c07a2c2'6a8346d2},
            {0xc5a05277'621be293, 0xc7098b73'05241886},
            {0xf7086715'3aa2db38, 0xb8cbee4f'c66d1ea8}
        };
    };

    // Compressed cache for double
    struct compressed_cache_detail {
        static constexpr int compression_ratio = 27;
        static constexpr uword::raw_type compressed_table_size =
            (cache_holder<ieee754_binary64>::max_k -
             cache_holder<ieee754_binary64>::min_k + compression_ratio) /
            compression_ratio;

        struct cache_holder_t {
            wuint::uint128 table[compressed_table_size];
        };

        static constexpr cache_holder_t cache = [] {
            cache_holder_t res{};
            for (uword::raw_type i = 0; i < compressed_table_size; ++i) {
                res.table[i] =
                    cache_holder<ieee754_binary64>::cache[i *
                                                          compression_ratio];
            }
            return res;
        }();

        struct pow5_holder_t {
            uint8::raw_type table[compression_ratio];
        };

        static constexpr pow5_holder_t pow5 = [] {
            pow5_holder_t res{};
            uint8::raw_type p = 1;
            for (uword::raw_type i = 0; i < compression_ratio; ++i) {
                res.table[i] = p;
                p *= 5;
            }
            return res;
        }();
    };
}  // namespace detail

////////////////////////////////////////////////////////////////////////////////////////
// Policies.
////////////////////////////////////////////////////////////////////////////////////////

namespace detail {
    // Forward declare the implementation class.
    template <class Float, class FloatTraits = default_float_traits<Float>>
    struct impl;

    namespace policy_impl {
        // Sign policies.
        namespace sign {
            struct base {};

            struct ignore : base {
                using sign_policy = ignore;
                static constexpr bool return_has_sign = false;

                template <class SignedSignificandBits, class return_type>
                static constexpr void
                handle_sign(SignedSignificandBits, return_type&) noexcept {
                }
            };

            struct return_sign : base {
                using sign_policy = return_sign;
                static constexpr bool return_has_sign = true;

                template <class SignedSignificandBits, class return_type>
                static constexpr void
                handle_sign(SignedSignificandBits s, return_type& r) noexcept {
                    r.is_negative = s.is_negative();
                }
            };
        }  // namespace sign

        // Trailing zero policies.
        namespace trailing_zero {
            struct base {};

            struct ignore : base {
                using trailing_zero_policy = ignore;
                static constexpr bool report_trailing_zeros = false;

                template <class Impl, class return_type>
                static constexpr void
                on_trailing_zeros(return_type&) noexcept {
                }

                template <class Impl, class return_type>
                static constexpr void
                no_trailing_zeros(return_type&) noexcept {
                }
            };

            struct remove : base {
                using trailing_zero_policy = remove;
                static constexpr bool report_trailing_zeros = false;

                template <class Impl, class return_type>
                JKJ_FORCEINLINE static constexpr void
                on_trailing_zeros(return_type& r) noexcept {
                    r.exponent += Impl::remove_trailing_zeros(r.significand);
                }

                template <class Impl, class return_type>
                static constexpr void
                no_trailing_zeros(return_type&) noexcept {
                }
            };

            struct report : base {
                using trailing_zero_policy = report;
                static constexpr bool report_trailing_zeros = true;

                template <class Impl, class return_type>
                static constexpr void
                on_trailing_zeros(return_type& r) noexcept {
                    r.may_have_trailing_zeros = true;
                }

                template <class Impl, class return_type>
                static constexpr void
                no_trailing_zeros(return_type& r) noexcept {
                    r.may_have_trailing_zeros = false;
                }
            };
        }  // namespace trailing_zero

        // Decimal-to-binary rounding mode policies.
        namespace decimal_to_binary_rounding {
            struct base {};

            enum class tag_t {
                to_nearest,
                left_closed_directed,
                right_closed_directed
            };

            namespace interval_type {
                struct symmetric_boundary {
                    static constexpr bool is_symmetric = true;
                    bool is_closed;

                    constexpr bool
                    include_left_endpoint() const noexcept {
                        return is_closed;
                    }

                    constexpr bool
                    include_right_endpoint() const noexcept {
                        return is_closed;
                    }
                };

                struct asymmetric_boundary {
                    static constexpr bool is_symmetric = false;
                    bool is_left_closed;

                    constexpr bool
                    include_left_endpoint() const noexcept {
                        return is_left_closed;
                    }

                    constexpr bool
                    include_right_endpoint() const noexcept {
                        return !is_left_closed;
                    }
                };

                struct closed {
                    static constexpr bool is_symmetric = true;

                    static constexpr bool
                    include_left_endpoint() noexcept {
                        return true;
                    }

                    static constexpr bool
                    include_right_endpoint() noexcept {
                        return true;
                    }
                };

                struct open {
                    static constexpr bool is_symmetric = true;

                    static constexpr bool
                    include_left_endpoint() noexcept {
                        return false;
                    }

                    static constexpr bool
                    include_right_endpoint() noexcept {
                        return false;
                    }
                };

                struct left_closed_right_open {
                    static constexpr bool is_symmetric = false;

                    static constexpr bool
                    include_left_endpoint() noexcept {
                        return true;
                    }

                    static constexpr bool
                    include_right_endpoint() noexcept {
                        return false;
                    }
                };

                struct right_closed_left_open {
                    static constexpr bool is_symmetric = false;

                    static constexpr bool
                    include_left_endpoint() noexcept {
                        return false;
                    }

                    static constexpr bool
                    include_right_endpoint() noexcept {
                        return true;
                    }
                };
            }  // namespace interval_type

            struct nearest_to_even : base {
                using decimal_to_binary_rounding_policy = nearest_to_even;
                static constexpr auto tag = tag_t::to_nearest;
                using normal_interval_type = interval_type::symmetric_boundary;
                using shorter_interval_type = interval_type::closed;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(nearest_to_even{});
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_normal_interval_case(SignedSignificandBits s,
                                            Func&& f) noexcept {
                    return f(s.has_even_significand_bits());
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_shorter_interval_case(SignedSignificandBits,
                                             Func&& f) noexcept {
                    return f();
                }
            };

            struct nearest_to_odd : base {
                using decimal_to_binary_rounding_policy = nearest_to_odd;
                static constexpr auto tag = tag_t::to_nearest;
                using normal_interval_type = interval_type::symmetric_boundary;
                using shorter_interval_type = interval_type::open;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(nearest_to_odd{});
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_normal_interval_case(SignedSignificandBits s,
                                            Func&& f) noexcept {
                    return f(!s.has_even_significand_bits());
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_shorter_interval_case(SignedSignificandBits,
                                             Func&& f) noexcept {
                    return f();
                }
            };

            struct nearest_toward_plus_infinity : base {
                using decimal_to_binary_rounding_policy =
                    nearest_toward_plus_infinity;
                static constexpr auto tag = tag_t::to_nearest;
                using normal_interval_type = interval_type::asymmetric_boundary;
                using shorter_interval_type =
                    interval_type::asymmetric_boundary;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(nearest_toward_plus_infinity{});
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_normal_interval_case(SignedSignificandBits s,
                                            Func&& f) noexcept {
                    return f(!s.is_negative());
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_shorter_interval_case(SignedSignificandBits s,
                                             Func&& f) noexcept {
                    return f(!s.is_negative());
                }
            };

            struct nearest_toward_minus_infinity : base {
                using decimal_to_binary_rounding_policy =
                    nearest_toward_minus_infinity;
                static constexpr auto tag = tag_t::to_nearest;
                using normal_interval_type = interval_type::asymmetric_boundary;
                using shorter_interval_type =
                    interval_type::asymmetric_boundary;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(nearest_toward_minus_infinity{});
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_normal_interval_case(SignedSignificandBits s,
                                            Func&& f) noexcept {
                    return f(s.is_negative());
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_shorter_interval_case(SignedSignificandBits s,
                                             Func&& f) noexcept {
                    return f(s.is_negative());
                }
            };

            struct nearest_toward_zero : base {
                using decimal_to_binary_rounding_policy = nearest_toward_zero;
                static constexpr auto tag = tag_t::to_nearest;
                using normal_interval_type =
                    interval_type::right_closed_left_open;
                using shorter_interval_type =
                    interval_type::right_closed_left_open;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(nearest_toward_zero{});
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_normal_interval_case(SignedSignificandBits,
                                            Func&& f) noexcept {
                    return f();
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_shorter_interval_case(SignedSignificandBits,
                                             Func&& f) noexcept {
                    return f();
                }
            };

            struct nearest_away_from_zero : base {
                using decimal_to_binary_rounding_policy =
                    nearest_away_from_zero;
                static constexpr auto tag = tag_t::to_nearest;
                using normal_interval_type =
                    interval_type::left_closed_right_open;
                using shorter_interval_type =
                    interval_type::left_closed_right_open;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(nearest_away_from_zero{});
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_normal_interval_case(SignedSignificandBits,
                                            Func&& f) noexcept {
                    return f();
                }

                template <class SignedSignificandBits, class Func>
                static constexpr auto
                invoke_shorter_interval_case(SignedSignificandBits,
                                             Func&& f) noexcept {
                    return f();
                }
            };

            namespace detail {
                struct nearest_always_closed {
                    static constexpr auto tag = tag_t::to_nearest;
                    using normal_interval_type = interval_type::closed;
                    using shorter_interval_type = interval_type::closed;

                    template <class SignedSignificandBits, class Func>
                    static constexpr auto
                    invoke_normal_interval_case(SignedSignificandBits,
                                                Func&& f) noexcept {
                        return f();
                    }

                    template <class SignedSignificandBits, class Func>
                    static constexpr auto
                    invoke_shorter_interval_case(SignedSignificandBits,
                                                 Func&& f) noexcept {
                        return f();
                    }
                };

                struct nearest_always_open {
                    static constexpr auto tag = tag_t::to_nearest;
                    using normal_interval_type = interval_type::open;
                    using shorter_interval_type = interval_type::open;

                    template <class SignedSignificandBits, class Func>
                    static constexpr auto
                    invoke_normal_interval_case(SignedSignificandBits,
                                                Func&& f) noexcept {
                        return f();
                    }

                    template <class SignedSignificandBits, class Func>
                    static constexpr auto
                    invoke_shorter_interval_case(SignedSignificandBits,
                                                 Func&& f) noexcept {
                        return f();
                    }
                };
            }  // namespace detail

            struct nearest_to_even_static_boundary : base {
                using decimal_to_binary_rounding_policy =
                    nearest_to_even_static_boundary;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits s, Func&& f) noexcept {
                    if (s.has_even_significand_bits()) {
                        return f(detail::nearest_always_closed{});
                    } else {
                        return f(detail::nearest_always_open{});
                    }
                }
            };

            struct nearest_to_odd_static_boundary : base {
                using decimal_to_binary_rounding_policy =
                    nearest_to_odd_static_boundary;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits s, Func&& f) noexcept {
                    if (s.has_even_significand_bits()) {
                        return f(detail::nearest_always_open{});
                    } else {
                        return f(detail::nearest_always_closed{});
                    }
                }
            };

            struct nearest_toward_plus_infinity_static_boundary : base {
                using decimal_to_binary_rounding_policy =
                    nearest_toward_plus_infinity_static_boundary;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits s, Func&& f) noexcept {
                    if (s.is_negative()) {
                        return f(nearest_toward_zero{});
                    } else {
                        return f(nearest_away_from_zero{});
                    }
                }
            };

            struct nearest_toward_minus_infinity_static_boundary : base {
                using decimal_to_binary_rounding_policy =
                    nearest_toward_minus_infinity_static_boundary;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits s, Func&& f) noexcept {
                    if (s.is_negative()) {
                        return f(nearest_away_from_zero{});
                    } else {
                        return f(nearest_toward_zero{});
                    }
                }
            };

            namespace detail {
                struct left_closed_directed {
                    static constexpr auto tag = tag_t::left_closed_directed;
                };

                struct right_closed_directed {
                    static constexpr auto tag = tag_t::right_closed_directed;
                };
            }  // namespace detail

            struct toward_plus_infinity : base {
                using decimal_to_binary_rounding_policy = toward_plus_infinity;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits s, Func&& f) noexcept {
                    if (s.is_negative()) {
                        return f(detail::left_closed_directed{});
                    } else {
                        return f(detail::right_closed_directed{});
                    }
                }
            };

            struct toward_minus_infinity : base {
                using decimal_to_binary_rounding_policy = toward_minus_infinity;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits s, Func&& f) noexcept {
                    if (s.is_negative()) {
                        return f(detail::right_closed_directed{});
                    } else {
                        return f(detail::left_closed_directed{});
                    }
                }
            };

            struct toward_zero : base {
                using decimal_to_binary_rounding_policy = toward_zero;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(detail::left_closed_directed{});
                }
            };

            struct away_from_zero : base {
                using decimal_to_binary_rounding_policy = away_from_zero;

                template <class SignedSignificandBits, class Func>
                static auto
                delegate(SignedSignificandBits, Func&& f) noexcept {
                    return f(detail::right_closed_directed{});
                }
            };
        }  // namespace decimal_to_binary_rounding

        // Binary-to-decimal rounding policies.
        // (Always assumes nearest rounding modes.)
        namespace binary_to_decimal_rounding {
            struct base {};

            enum class tag_t {
                do_not_care,
                to_even,
                to_odd,
                away_from_zero,
                toward_zero
            };

            struct do_not_care : base {
                using binary_to_decimal_rounding_policy = do_not_care;
                static constexpr auto tag = tag_t::do_not_care;

                template <class return_type>
                static constexpr bool
                prefer_round_down(return_type const&) noexcept {
                    return false;
                }
            };

            struct to_even : base {
                using binary_to_decimal_rounding_policy = to_even;
                static constexpr auto tag = tag_t::to_even;

                template <class return_type>
                static constexpr bool
                prefer_round_down(return_type const& r) noexcept {
                    return r.significand % 2 != 0;
                }
            };

            struct to_odd : base {
                using binary_to_decimal_rounding_policy = to_odd;
                static constexpr auto tag = tag_t::to_odd;

                template <class return_type>
                static constexpr bool
                prefer_round_down(return_type const& r) noexcept {
                    return r.significand % 2 == 0;
                }
            };

            struct away_from_zero : base {
                using binary_to_decimal_rounding_policy = away_from_zero;
                static constexpr auto tag = tag_t::away_from_zero;

                template <class return_type>
                static constexpr bool
                prefer_round_down(return_type const&) noexcept {
                    return false;
                }
            };

            struct toward_zero : base {
                using binary_to_decimal_rounding_policy = toward_zero;
                static constexpr auto tag = tag_t::toward_zero;

                template <class return_type>
                static constexpr bool
                prefer_round_down(return_type const&) noexcept {
                    return true;
                }
            };
        }  // namespace binary_to_decimal_rounding

        // Cache policies.
        namespace cache {
            struct base {};

            struct full : base {
                using cache_policy = full;

                template <class FloatFormat>
                static constexpr
                    typename cache_holder<FloatFormat>::cache_entry_type
                    get_cache(int k) noexcept {
                    // assert(k >= cache_holder<FloatFormat>::min_k &&
                    // k <= cache_holder<FloatFormat>::max_k);
                    return cache_holder<FloatFormat>::cache[uword::raw_type(
                        k - cache_holder<FloatFormat>::min_k)];
                }
            };

            struct compact : base {
                using cache_policy = compact;

                template <class FloatFormat>
                static constexpr
                    typename cache_holder<FloatFormat>::cache_entry_type
                    get_cache(int k) noexcept {
                    // assert(k >= cache_holder<FloatFormat>::min_k &&
                    // k <= cache_holder<FloatFormat>::max_k);

                    if constexpr (cat::is_same<FloatFormat, ieee754_binary64>) {
                        // Compute the base index.
                        auto const cache_index =
                            int(uint4::raw_type(
                                    k - cache_holder<FloatFormat>::min_k) /
                                compressed_cache_detail::compression_ratio);
                        auto const kb =
                            cache_index *
                                compressed_cache_detail::compression_ratio +
                            cache_holder<FloatFormat>::min_k;
                        auto const offset = k - kb;

                        // Get the base cache.
                        auto const base_cache =
                            compressed_cache_detail::cache.table[cache_index];

                        if (offset == 0) {
                            return base_cache;
                        } else {
                            // Compute the required amount of bit-shift.
                            auto const alpha =
                                log::floor_log2_pow10(kb + offset) -
                                log::floor_log2_pow10(kb) - offset;
                            // assert(alpha > 0 && alpha < 64);

                            // Try to recover the real cache.
                            auto const pow5 =
                                compressed_cache_detail::pow5.table[offset];
                            auto recovered_cache =
                                wuint::umul128(base_cache.high(), pow5);
                            auto const middle_low =
                                wuint::umul128(base_cache.low(), pow5);

                            recovered_cache += middle_low.high();

                            auto const high_to_middle = recovered_cache.high()
                                                        << (64 - alpha);
                            auto const middle_to_low = recovered_cache.low()
                                                       << (64 - alpha);

                            recovered_cache = wuint::uint128{
                                (recovered_cache.low() >> alpha) |
                                    high_to_middle,
                                ((middle_low.low() >> alpha) | middle_to_low)};

                            // assert(recovered_cache.low() + 1 != 0);
                            recovered_cache = {recovered_cache.high(),
                                               recovered_cache.low() + 1};

                            return recovered_cache;
                        }
                    } else {
                        // Just use the full cache for anything other
                        // than binary64
                        return cache_holder<FloatFormat>::cache[uword::raw_type(
                            k - cache_holder<FloatFormat>::min_k)];
                    }
                }
            };
        }  // namespace cache
    }  // namespace policy_impl
}  // namespace detail

namespace policy {
    namespace sign {
        inline constexpr auto ignore = detail::policy_impl::sign::ignore{};
        inline constexpr auto return_sign =
            detail::policy_impl::sign::return_sign{};
    }  // namespace sign

    namespace trailing_zero {
        inline constexpr auto ignore =
            detail::policy_impl::trailing_zero::ignore{};
        inline constexpr auto remove =
            detail::policy_impl::trailing_zero::remove{};
        inline constexpr auto report =
            detail::policy_impl::trailing_zero::report{};
    }  // namespace trailing_zero

    namespace decimal_to_binary_rounding {
        inline constexpr auto nearest_to_even =
            detail::policy_impl::decimal_to_binary_rounding::nearest_to_even{};
        inline constexpr auto nearest_to_odd =
            detail::policy_impl::decimal_to_binary_rounding::nearest_to_odd{};
        inline constexpr auto nearest_toward_plus_infinity =
            detail::policy_impl::decimal_to_binary_rounding::
                nearest_toward_plus_infinity{};
        inline constexpr auto nearest_toward_minus_infinity =
            detail::policy_impl::decimal_to_binary_rounding::
                nearest_toward_minus_infinity{};
        inline constexpr auto nearest_toward_zero = detail::policy_impl::
            decimal_to_binary_rounding::nearest_toward_zero{};
        inline constexpr auto nearest_away_from_zero = detail::policy_impl::
            decimal_to_binary_rounding::nearest_away_from_zero{};

        inline constexpr auto nearest_to_even_static_boundary =
            detail::policy_impl::decimal_to_binary_rounding::
                nearest_to_even_static_boundary{};
        inline constexpr auto nearest_to_odd_static_boundary =
            detail::policy_impl::decimal_to_binary_rounding::
                nearest_to_odd_static_boundary{};
        inline constexpr auto nearest_toward_plus_infinity_static_boundary =
            detail::policy_impl::decimal_to_binary_rounding::
                nearest_toward_plus_infinity_static_boundary{};
        inline constexpr auto nearest_toward_minus_infinity_static_boundary =
            detail::policy_impl::decimal_to_binary_rounding::
                nearest_toward_minus_infinity_static_boundary{};

        inline constexpr auto toward_plus_infinity = detail::policy_impl::
            decimal_to_binary_rounding::toward_plus_infinity{};
        inline constexpr auto toward_minus_infinity = detail::policy_impl::
            decimal_to_binary_rounding::toward_minus_infinity{};
        inline constexpr auto toward_zero =
            detail::policy_impl::decimal_to_binary_rounding::toward_zero{};
        inline constexpr auto away_from_zero =
            detail::policy_impl::decimal_to_binary_rounding::away_from_zero{};
    }  // namespace decimal_to_binary_rounding

    namespace binary_to_decimal_rounding {
        inline constexpr auto do_not_care =
            detail::policy_impl::binary_to_decimal_rounding::do_not_care{};
        inline constexpr auto to_even =
            detail::policy_impl::binary_to_decimal_rounding::to_even{};
        inline constexpr auto to_odd =
            detail::policy_impl::binary_to_decimal_rounding::to_odd{};
        inline constexpr auto away_from_zero =
            detail::policy_impl::binary_to_decimal_rounding::away_from_zero{};
        inline constexpr auto toward_zero =
            detail::policy_impl::binary_to_decimal_rounding::toward_zero{};
    }  // namespace binary_to_decimal_rounding

    namespace cache {
        inline constexpr auto full = detail::policy_impl::cache::full{};
        inline constexpr auto compact = detail::policy_impl::cache::compact{};
    }  // namespace cache
}  // namespace policy

namespace detail {
    ////////////////////////////////////////////////////////////////////////////////////////
    // The main algorithm.
    ////////////////////////////////////////////////////////////////////////////////////////

    template <class Float, class FloatTraits>
    struct impl : private FloatTraits, private FloatTraits::format {
        using format = FloatTraits::format;
        using carrier_uint = FloatTraits::carrier_uint;

        using FloatTraits::carrier_bits;
        using format::decimal_digits;
        using format::exponent_bias;
        using format::max_exponent;
        using format::min_exponent;
        using format::significand_bits;

        static constexpr int kappa =
            cat::is_same<format, ieee754_binary32> ? 1 : 2;
        static_assert(kappa >= 1);
        static_assert(carrier_bits >=
                      significand_bits + 2 + log::floor_log2_pow10(kappa + 1));

        static constexpr int min_k = [] {
            constexpr auto a = -log::floor_log10_pow2_minus_log10_4_over_3(
                int(max_exponent - significand_bits));
            constexpr auto b =
                -log::floor_log10_pow2(int(max_exponent - significand_bits)) +
                kappa;
            return a < b ? a : b;
        }();
        static_assert(min_k >= cache_holder<format>::min_k);

        static constexpr int max_k = [] {
            // We do invoke shorter_interval_case for exponent == min_exponent
            // case, so we should not add 1 here.
            constexpr auto a = -log::floor_log10_pow2_minus_log10_4_over_3(
                int(min_exponent - significand_bits /*+ 1*/));
            constexpr auto b =
                -log::floor_log10_pow2(int(min_exponent - significand_bits)) +
                kappa;
            return a > b ? a : b;
        }();
        static_assert(max_k <= cache_holder<format>::max_k);

        using cache_entry_type =
            typename cache_holder<format>::cache_entry_type;
        static constexpr auto cache_bits = cache_holder<format>::cache_bits;

        static constexpr int
            case_shorter_interval_left_endpoint_lower_threshold = 2;
        static constexpr int
            case_shorter_interval_left_endpoint_upper_threshold =
                2 +
                log::floor_log2(
                    compute_power<count_factors<5>((carrier_uint(1)
                                                    << (significand_bits + 2)) -
                                                   1) +
                                  1>(10) /
                    3);

        static constexpr int
            case_shorter_interval_right_endpoint_lower_threshold = 0;
        static constexpr int
            case_shorter_interval_right_endpoint_upper_threshold =
                2 +
                log::floor_log2(
                    compute_power<count_factors<5>((carrier_uint(1)
                                                    << (significand_bits + 1)) +
                                                   1) +
                                  1>(10) /
                    3);

        static constexpr int shorter_interval_tie_lower_threshold =
            -log::floor_log5_pow2_minus_log5_3(significand_bits + 4) - 2 -
            significand_bits;
        static constexpr int shorter_interval_tie_upper_threshold =
            -log::floor_log5_pow2(significand_bits + 2) - 2 - significand_bits;

        struct compute_mul_result {
            carrier_uint result;
            bool is_integer;
        };

        struct compute_mul_parity_result {
            bool parity;
            bool is_integer;
        };

        //// The main algorithm assumes the input is a normal/subnormal finite
        /// number

        template <class return_type, class interval_type,
                  class TrailingZeroPolicy, class BinaryToDecimalRoundingPolicy,
                  class CachePolicy, class... AdditionalArgs>
        JKJ_SAFEBUFFERS static return_type
        compute_nearest_normal(carrier_uint const two_fc, int const exponent,
                               AdditionalArgs... additional_args) noexcept {
            //////////////////////////////////////////////////////////////////////
            // Step 1: Schubfach multiplier calculation
            //////////////////////////////////////////////////////////////////////

            return_type ret_value;
            interval_type interval{additional_args...};

            // Compute k and beta.
            int const minus_k = log::floor_log10_pow2(exponent) - kappa;
            auto const cache =
                CachePolicy::template get_cache<format>(-minus_k);
            int const beta = exponent + log::floor_log2_pow10(-minus_k);

            // Compute zi and deltai.
            // 10^kappa <= deltai < 10^(kappa + 1)
            auto const deltai = compute_delta(cache, beta);
            // For the case of binary32, the result of integer check is not
            // correct for 29711844 * 2^-82
            // = 6.1442653300000000008655037797566933477355632930994033813476...
            // * 10^-18 and 29711844 * 2^-81
            // = 1.2288530660000000001731007559513386695471126586198806762695...
            // * 10^-17, and they are the unique counterexamples. However, since
            // 29711844 is even, this does not cause any problem for the
            // endpoints calculations; it can only cause a problem when we need
            // to perform integer check for the center. Fortunately, with these
            // inputs, that branch is never executed, so we are fine.
            auto const [zi, is_z_integer] =
                compute_mul((two_fc | 1) << beta, cache);

            //////////////////////////////////////////////////////////////////////
            // Step 2: Try larger divisor; remove trailing zeros if necessary
            //////////////////////////////////////////////////////////////////////

            constexpr auto big_divisor =
                compute_power<kappa + 1>(uint4::raw_type(10));
            constexpr auto small_divisor =
                compute_power<kappa>(uint4::raw_type(10));

            // Using an upper bound on zi, we might be able to optimize the
            // division better than the compiler; we are computing zi /
            // big_divisor here.
            ret_value.significand = div::divide_by_pow10<
                kappa + 1, carrier_uint,
                (carrier_uint(1) << (significand_bits + 1)) * big_divisor - 1>(
                zi);
            auto r = uint4::raw_type(zi - big_divisor * ret_value.significand);

            if (r < deltai) {
                // Exclude the right endpoint if necessary.
                if (r == 0 &&
                    (is_z_integer & !interval.include_right_endpoint())) {
                    if constexpr (BinaryToDecimalRoundingPolicy::tag ==
                                  policy_impl::binary_to_decimal_rounding::
                                      tag_t::do_not_care) {
                        ret_value.significand *= 10;
                        ret_value.exponent = minus_k + kappa;
                        --ret_value.significand;
                        TrailingZeroPolicy::template no_trailing_zeros<impl>(
                            ret_value);
                        return ret_value;
                    } else {
                        --ret_value.significand;
                        r = big_divisor;
                        goto small_divisor_case_label;
                    }
                }
            } else if (r > deltai) {
                goto small_divisor_case_label;
            } else {
                // r == deltai; compare fractional parts.
                auto const [xi_parity, x_is_integer] =
                    compute_mul_parity(two_fc - 1, cache, beta);

                if (!(xi_parity |
                      (x_is_integer & interval.include_left_endpoint()))) {
                    goto small_divisor_case_label;
                }
            }
            ret_value.exponent = minus_k + kappa + 1;

            // We may need to remove trailing zeros.
            TrailingZeroPolicy::template on_trailing_zeros<impl>(ret_value);
            return ret_value;

            //////////////////////////////////////////////////////////////////////
            // Step 3: Find the significand with the smaller divisor
            //////////////////////////////////////////////////////////////////////

small_divisor_case_label:
            TrailingZeroPolicy::template no_trailing_zeros<impl>(ret_value);
            ret_value.significand *= 10;
            ret_value.exponent = minus_k + kappa;

            if constexpr (BinaryToDecimalRoundingPolicy::tag ==
                          policy_impl::binary_to_decimal_rounding::tag_t::
                              do_not_care) {
                // Normally, we want to compute
                // ret_value.significand += r / small_divisor
                // and return, but we need to take care of the case that the
                // resulting value is exactly the right endpoint, while that is
                // not included in the interval.
                if (!interval.include_right_endpoint()) {
                    // Is r divisible by 10^kappa?
                    if (is_z_integer &&
                        div::check_divisibility_and_divide_by_pow10<kappa>(r)) {
                        // This should be in the interval.
                        ret_value.significand += r - 1;
                    } else {
                        ret_value.significand += r;
                    }
                } else {
                    ret_value.significand +=
                        div::small_division_by_pow10<kappa>(r);
                }
            } else {
                auto dist = r - (deltai / 2) + (small_divisor / 2);
                bool const approx_y_parity =
                    ((dist ^ (small_divisor / 2)) & 1) != 0;

                // Is dist divisible by 10^kappa?
                bool const divisible_by_small_divisor =
                    div::check_divisibility_and_divide_by_pow10<kappa>(dist);

                // Add dist / 10^kappa to the significand.
                ret_value.significand += dist;

                if (divisible_by_small_divisor) {
                    // Check z^(f) >= epsilon^(f).
                    // We have either yi == zi - epsiloni or yi == (zi -
                    // epsiloni) - 1, where yi == zi - epsiloni if and only if
                    // z^(f) >= epsilon^(f). Since there are only 2
                    // possibilities, we only need to care about the parity.
                    // Also, zi and r should have the same parity since the
                    // divisor is an even number.
                    auto const [yi_parity, is_y_integer] =
                        compute_mul_parity(two_fc, cache, beta);
                    if (yi_parity != approx_y_parity) {
                        --ret_value.significand;
                    } else {
                        // If z^(f) >= epsilon^(f), we might have a tie
                        // when z^(f) == epsilon^(f), or equivalently, when y is
                        // an integer. For tie-to-up case, we can just choose
                        // the upper one.
                        if (BinaryToDecimalRoundingPolicy::prefer_round_down(
                                ret_value) &
                            is_y_integer) {
                            --ret_value.significand;
                        }
                    }
                }
            }
            return ret_value;
        }

        template <class return_type, class interval_type,
                  class TrailingZeroPolicy, class BinaryToDecimalRoundingPolicy,
                  class CachePolicy, class... AdditionalArgs>
        JKJ_SAFEBUFFERS static return_type
        compute_nearest_shorter(int const exponent,
                                AdditionalArgs... additional_args) noexcept {
            return_type ret_value;
            interval_type interval{additional_args...};

            // Compute k and beta.
            int const minus_k =
                log::floor_log10_pow2_minus_log10_4_over_3(exponent);
            int const beta = exponent + log::floor_log2_pow10(-minus_k);

            // Compute xi and zi.
            auto const cache =
                CachePolicy::template get_cache<format>(-minus_k);

            auto xi =
                compute_left_endpoint_for_shorter_interval_case(cache, beta);
            auto zi =
                compute_right_endpoint_for_shorter_interval_case(cache, beta);

            // If we don't accept the right endpoint and
            // if the right endpoint is an integer, decrease it.
            if (!interval.include_right_endpoint() &&
                is_right_endpoint_integer_shorter_interval(exponent)) {
                --zi;
            }
            // If we don't accept the left endpoint or
            // if the left endpoint is not an integer, increase it.
            if (!interval.include_left_endpoint() ||
                !is_left_endpoint_integer_shorter_interval(exponent)) {
                ++xi;
            }

            // Try bigger divisor.
            ret_value.significand = zi / 10;

            // If succeed, remove trailing zeros if necessary and return.
            if (ret_value.significand * 10 >= xi) {
                ret_value.exponent = minus_k + 1;
                TrailingZeroPolicy::template on_trailing_zeros<impl>(ret_value);
                return ret_value;
            }

            // Otherwise, compute the round-up of y.
            TrailingZeroPolicy::template no_trailing_zeros<impl>(ret_value);
            ret_value.significand =
                compute_round_up_for_shorter_interval_case(cache, beta);
            ret_value.exponent = minus_k;

            // When tie occurs, choose one of them according to the rule.
            if (BinaryToDecimalRoundingPolicy::prefer_round_down(ret_value) &&
                exponent >= shorter_interval_tie_lower_threshold &&
                exponent <= shorter_interval_tie_upper_threshold) {
                --ret_value.significand;
            } else if (ret_value.significand < xi) {
                ++ret_value.significand;
            }
            return ret_value;
        }

        template <class return_type, class TrailingZeroPolicy,
                  class CachePolicy>
        JKJ_SAFEBUFFERS static return_type
        compute_left_closed_directed(carrier_uint const two_fc,
                                     int exponent) noexcept {
            //////////////////////////////////////////////////////////////////////
            // Step 1: Schubfach multiplier calculation
            //////////////////////////////////////////////////////////////////////

            return_type ret_value;

            // Compute k and beta.
            int const minus_k = log::floor_log10_pow2(exponent) - kappa;
            auto const cache =
                CachePolicy::template get_cache<format>(-minus_k);
            int const beta = exponent + log::floor_log2_pow10(-minus_k);

            // Compute xi and deltai.
            // 10^kappa <= deltai < 10^(kappa + 1)
            auto const deltai = compute_delta(cache, beta);
            auto [xi, is_x_integer] = compute_mul(two_fc << beta, cache);

            // Deal with the unique exceptional cases
            // 29711844 * 2^-82
            // = 6.1442653300000000008655037797566933477355632930994033813476...
            // * 10^-18 and 29711844 * 2^-81
            // = 1.2288530660000000001731007559513386695471126586198806762695...
            // * 10^-17 for binary32.
            if constexpr (cat::is_same<format, ieee754_binary32>) {
                if (exponent <= -80) {
                    is_x_integer = false;
                }
            }

            if (!is_x_integer) {
                ++xi;
            }

            //////////////////////////////////////////////////////////////////////
            // Step 2: Try larger divisor; remove trailing zeros if necessary
            //////////////////////////////////////////////////////////////////////

            constexpr auto big_divisor =
                compute_power<kappa + 1>(uint4::raw_type(10));

            // Using an upper bound on xi, we might be able to optimize the
            // division better than the compiler; we are computing xi /
            // big_divisor here.
            ret_value.significand = div::divide_by_pow10<
                kappa + 1, carrier_uint,
                (carrier_uint(1) << (significand_bits + 1)) * big_divisor - 1>(
                xi);
            auto r = uint4::raw_type(xi - big_divisor * ret_value.significand);

            if (r != 0) {
                ++ret_value.significand;
                r = big_divisor - r;
            }

            if (r > deltai) {
                goto small_divisor_case_label;
            } else if (r == deltai) {
                // Compare the fractional parts.
                // This branch is never taken for the exceptional cases
                // 2f_c = 29711482, e = -81
                // (6.1442649164096937243516663440523473127541365101933479309082...
                // * 10^-18) and 2f_c = 29711482, e = -80
                // (1.2288529832819387448703332688104694625508273020386695861816...
                // * 10^-17).
                auto const [zi_parity, is_z_integer] =
                    compute_mul_parity(two_fc + 2, cache, beta);
                if (zi_parity || is_z_integer) {
                    goto small_divisor_case_label;
                }
            }

            // The ceiling is inside, so we are done.
            ret_value.exponent = minus_k + kappa + 1;
            TrailingZeroPolicy::template on_trailing_zeros<impl>(ret_value);
            return ret_value;

            //////////////////////////////////////////////////////////////////////
            // Step 3: Find the significand with the smaller divisor
            //////////////////////////////////////////////////////////////////////

small_divisor_case_label:
            ret_value.significand *= 10;
            ret_value.significand -= div::small_division_by_pow10<kappa>(r);
            ret_value.exponent = minus_k + kappa;
            TrailingZeroPolicy::template no_trailing_zeros<impl>(ret_value);
            return ret_value;
        }

        template <class return_type, class TrailingZeroPolicy,
                  class CachePolicy>
        JKJ_SAFEBUFFERS static return_type
        compute_right_closed_directed(carrier_uint const two_fc,
                                      int const exponent,
                                      bool shorter_interval) noexcept {
            //////////////////////////////////////////////////////////////////////
            // Step 1: Schubfach multiplier calculation
            //////////////////////////////////////////////////////////////////////

            return_type ret_value;

            // Compute k and beta.
            int const minus_k =
                log::floor_log10_pow2(exponent - (shorter_interval ? 1 : 0)) -
                kappa;
            auto const cache =
                CachePolicy::template get_cache<format>(-minus_k);
            int const beta = exponent + log::floor_log2_pow10(-minus_k);

            // Compute zi and deltai.
            // 10^kappa <= deltai < 10^(kappa + 1)
            auto const deltai = shorter_interval
                                    ? compute_delta(cache, beta - 1)
                                    : compute_delta(cache, beta);
            carrier_uint const zi = compute_mul(two_fc << beta, cache).result;

            //////////////////////////////////////////////////////////////////////
            // Step 2: Try larger divisor; remove trailing zeros if necessary
            //////////////////////////////////////////////////////////////////////

            constexpr auto big_divisor =
                compute_power<kappa + 1>(uint4::raw_type(10));

            // Using an upper bound on zi, we might be able to optimize the
            // division better than the compiler; we are computing zi /
            // big_divisor here.
            ret_value.significand = div::divide_by_pow10<
                kappa + 1, carrier_uint,
                (carrier_uint(1) << (significand_bits + 1)) * big_divisor - 1>(
                zi);
            auto const r =
                uint4::raw_type(zi - big_divisor * ret_value.significand);

            if (r > deltai) {
                goto small_divisor_case_label;
            } else if (r == deltai) {
                // Compare the fractional parts.
                if (!compute_mul_parity(two_fc - (shorter_interval ? 1 : 2),
                                        cache, beta)
                         .parity) {
                    goto small_divisor_case_label;
                }
            }

            // The floor is inside, so we are done.
            ret_value.exponent = minus_k + kappa + 1;
            TrailingZeroPolicy::template on_trailing_zeros<impl>(ret_value);
            return ret_value;

            //////////////////////////////////////////////////////////////////////
            // Step 3: Find the significand with the small divisor
            //////////////////////////////////////////////////////////////////////

small_divisor_case_label:
            ret_value.significand *= 10;
            ret_value.significand += div::small_division_by_pow10<kappa>(r);
            ret_value.exponent = minus_k + kappa;
            TrailingZeroPolicy::template no_trailing_zeros<impl>(ret_value);
            return ret_value;
        }

        // Remove trailing zeros from n and return the number of zeros removed.
        JKJ_FORCEINLINE static int
        remove_trailing_zeros(carrier_uint& n) noexcept {
            // assert(n != 0);

            if constexpr (cat::is_same<format, ieee754_binary32>) {
                constexpr auto mod_inv_5 = uint4::raw_type(0xcccccccd);
                constexpr auto mod_inv_25 = mod_inv_5 * mod_inv_5;

                int s = 0;
                while (true) {
                    auto q = bits::rotr(n * mod_inv_25, 2);
                    if (q <= limits<uint4::raw_type>::max() / 100) {
                        n = q;
                        s += 2;
                    } else {
                        break;
                    }
                }
                auto q = bits::rotr(n * mod_inv_5, 1);
                if (q <= limits<uint4::raw_type>::max() / 10) {
                    n = q;
                    s |= 1;
                }

                return s;
            } else {
                static_assert(cat::is_same<format, ieee754_binary64>);

                // Divide by 10^8 and reduce to 32-bits if divisible.
                // Since ret_value.significand <= (2^53 * 1000 - 1) / 1000 <
                // 10^16, n is at most of 16 digits.

                // This magic number is ceil(2^90 / 10^8).
                constexpr auto magic_number =
                    uint8::raw_type(12'379'400'392'853'802'749ull);
                auto nm = wuint::umul128(n, magic_number);

                // Is n is divisible by 10^8?
                if ((nm.high() & ((uint8::raw_type(1) << (90 - 64)) - 1)) ==
                        0 &&
                    nm.low() < magic_number) {
                    // If yes, work with the quotient.
                    auto n32 = uint4::raw_type(nm.high() >> (90 - 64));

                    constexpr auto mod_inv_5 = uint4::raw_type(0xcccccccd);
                    constexpr auto mod_inv_25 = mod_inv_5 * mod_inv_5;

                    int s = 8;
                    while (true) {
                        auto q = bits::rotr(n32 * mod_inv_25, 2);
                        if (q <= limits<uint4::raw_type>::max() / 100) {
                            n32 = q;
                            s += 2;
                        } else {
                            break;
                        }
                    }
                    auto q = bits::rotr(n32 * mod_inv_5, 1);
                    if (q <= limits<uint4::raw_type>::max() / 10) {
                        n32 = q;
                        s |= 1;
                    }

                    n = n32;
                    return s;
                }

                // If n is not divisible by 10^8, work with n itself.
                constexpr auto mod_inv_5 = uint8::raw_type(0xcccccccc'cccccccd);
                constexpr auto mod_inv_25 = mod_inv_5 * mod_inv_5;

                int s = 0;
                while (true) {
                    auto q = bits::rotr(n * mod_inv_25, 2);
                    if (q <= limits<uint8::raw_type>::max() / 100) {
                        n = q;
                        s += 2;
                    } else {
                        break;
                    }
                }
                auto q = bits::rotr(n * mod_inv_5, 1);
                if (q <= limits<uint8::raw_type>::max() / 10) {
                    n = q;
                    s |= 1;
                }

                return s;
            }
        }

        static compute_mul_result
        compute_mul(carrier_uint u, cache_entry_type const& cache) noexcept {
            if constexpr (cat::is_same<format, ieee754_binary32>) {
                auto r = wuint::umul96_upper64(u, cache);
                return {carrier_uint(r >> 32), carrier_uint(r) == 0};
            } else {
                static_assert(cat::is_same<format, ieee754_binary64>);
                auto r = wuint::umul192_upper128(u, cache);
                return {r.high(), r.low() == 0};
            }
        }

        static constexpr uint4::raw_type
        compute_delta(cache_entry_type const& cache, int beta) noexcept {
            if constexpr (cat::is_same<format, ieee754_binary32>) {
                return uint4::raw_type(cache >> (cache_bits - 1 - beta));
            } else {
                static_assert(cat::is_same<format, ieee754_binary64>);
                return uint4::raw_type(cache.high() >>
                                       (carrier_bits - 1 - beta));
            }
        }

        static compute_mul_parity_result
        compute_mul_parity(carrier_uint two_f, cache_entry_type const& cache,
                           int beta) noexcept {
            // assert(beta >= 1);
            // assert(beta < 64);

            if constexpr (cat::is_same<format, ieee754_binary32>) {
                auto r = wuint::umul96_lower64(two_f, cache);
                return {((r >> (64 - beta)) & 1) != 0,
                        uint4::raw_type(r >> (32 - beta)) == 0};
            } else {
                static_assert(cat::is_same<format, ieee754_binary64>);
                auto r = wuint::umul192_lower128(two_f, cache);
                return {((r.high() >> (64 - beta)) & 1) != 0,
                        ((r.high() << beta) | (r.low() >> (64 - beta))) == 0};
            }
        }

        static constexpr carrier_uint
        compute_left_endpoint_for_shorter_interval_case(
            cache_entry_type const& cache, int beta) noexcept {
            if constexpr (cat::is_same<format, ieee754_binary32>) {
                return carrier_uint(
                    (cache - (cache >> (significand_bits + 2))) >>
                    (cache_bits - significand_bits - 1 - beta));
            } else {
                static_assert(cat::is_same<format, ieee754_binary64>);
                return (cache.high() -
                        (cache.high() >> (significand_bits + 2))) >>
                       (carrier_bits - significand_bits - 1 - beta);
            }
        }

        static constexpr carrier_uint
        compute_right_endpoint_for_shorter_interval_case(
            cache_entry_type const& cache, int beta) noexcept {
            if constexpr (cat::is_same<format, ieee754_binary32>) {
                return carrier_uint(
                    (cache + (cache >> (significand_bits + 1))) >>
                    (cache_bits - significand_bits - 1 - beta));
            } else {
                static_assert(cat::is_same<format, ieee754_binary64>);
                return (cache.high() +
                        (cache.high() >> (significand_bits + 1))) >>
                       (carrier_bits - significand_bits - 1 - beta);
            }
        }

        static constexpr carrier_uint
        compute_round_up_for_shorter_interval_case(
            cache_entry_type const& cache, int beta) noexcept {
            if constexpr (cat::is_same<format, ieee754_binary32>) {
                return (carrier_uint(cache >> (cache_bits - significand_bits -
                                               2 - beta)) +
                        1) /
                       2;
            } else {
                static_assert(cat::is_same<format, ieee754_binary64>);
                return ((cache.high() >>
                         (carrier_bits - significand_bits - 2 - beta)) +
                        1) /
                       2;
            }
        }

        static constexpr bool
        is_right_endpoint_integer_shorter_interval(int exponent) noexcept {
            return exponent >=
                       case_shorter_interval_right_endpoint_lower_threshold &&
                   exponent <=
                       case_shorter_interval_right_endpoint_upper_threshold;
        }

        static constexpr bool
        is_left_endpoint_integer_shorter_interval(int exponent) noexcept {
            return exponent >=
                       case_shorter_interval_left_endpoint_lower_threshold &&
                   exponent <=
                       case_shorter_interval_left_endpoint_upper_threshold;
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////
    // Policy holder.
    ////////////////////////////////////////////////////////////////////////////////////////

    namespace policy_impl {
        // The library will specify a list of accepted kinds of policies and
        // their defaults, and the user will pass a list of policies. The aim of
        // helper classes/functions here is to do the following:
        //   1. Check if the policy parameters given by the user are all valid;
        //   that means,
        //      each of them should be of the kinds specified by the library.
        //      If that's not the case, then the compilation fails.
        //   2. Check if multiple policy parameters for the same kind is
        //   specified by the user.
        //      If that's the case, then the compilation fails.
        //   3. Build a class deriving from all policies the user have given,
        //   and also from
        //      the default policies if the user did not specify one for some
        //      kinds.
        // A policy belongs to a certain kind if it is deriving from a base
        // class.

        // For a given kind, find a policy belonging to that kind.
        // Check if there are more than one such policies.
        enum class policy_found_info {
            not_found,
            unique,
            repeated
        };

        template <class Policy, policy_found_info info>
        struct found_policy_pair {
            using policy = Policy;
            static constexpr auto found_info = info;
        };

        template <class Base, class DefaultPolicy>
        struct base_default_pair {
            using base = Base;

            template <class FoundPolicyInfo>
            static constexpr FoundPolicyInfo
            get_policy_impl(FoundPolicyInfo) {
                return {};
            }

            template <class FoundPolicyInfo, class FirstPolicy,
                      class... remainingPolicies>
            static constexpr auto
            get_policy_impl(FoundPolicyInfo, FirstPolicy,
                            remainingPolicies... remainings) {
                if constexpr (cat::is_base_of<Base, FirstPolicy>) {
                    if constexpr (FoundPolicyInfo::found_info ==
                                  policy_found_info::not_found) {
                        return get_policy_impl(
                            found_policy_pair<FirstPolicy,
                                              policy_found_info::unique>{},
                            remainings...);
                    } else {
                        return get_policy_impl(
                            found_policy_pair<FirstPolicy,
                                              policy_found_info::repeated>{},
                            remainings...);
                    }
                } else {
                    return get_policy_impl(FoundPolicyInfo(), remainings...);
                }
            }

            template <class... Policies>
            static constexpr auto
            get_policy(Policies... policies) {
                return get_policy_impl(
                    found_policy_pair<DefaultPolicy,
                                      policy_found_info::not_found>{},
                    policies...);
            }
        };

        template <class... BaseDefaultPairs>
        struct base_default_pair_list {};

        // Check if a given policy belongs to one of the kinds specified by the
        // library.
        template <class Policy>
        constexpr bool
        check_policy_validity(Policy, base_default_pair_list<>) {
            return false;
        }

        template <class Policy, class FirstBaseDefaultPair,
                  class... remainingBaseDefaultPairs>
        constexpr bool
        check_policy_validity(
            Policy, base_default_pair_list<FirstBaseDefaultPair,
                                           remainingBaseDefaultPairs...>) {
            return cat::is_base_of<typename FirstBaseDefaultPair::base,
                                   Policy> ||
                   check_policy_validity(
                       Policy(),
                       base_default_pair_list<remainingBaseDefaultPairs...>{});
        }

        template <class BaseDefaultPairList>
        constexpr bool
        check_policy_list_validity(BaseDefaultPairList) {
            return true;
        }

        template <class BaseDefaultPairList, class FirstPolicy,
                  class... remainingPolicies>
        constexpr bool
        check_policy_list_validity(BaseDefaultPairList, FirstPolicy,
                                   remainingPolicies... remaining_policies) {
            return check_policy_validity(FirstPolicy(),
                                         BaseDefaultPairList()) &&
                   check_policy_list_validity(BaseDefaultPairList(),
                                              remaining_policies...);
        }

        // Build policy_holder.
        template <bool repeated_, class... FoundPolicyPairs>
        struct found_policy_pair_list {
            static constexpr bool repeated = repeated_;
        };

        template <class... Policies>
        struct policy_holder : Policies... {};

        template <bool repeated, class... FoundPolicyPairs, class... Policies>
        constexpr auto
        make_policy_holder_impl(
            base_default_pair_list<>,
            found_policy_pair_list<repeated, FoundPolicyPairs...>,
            Policies...) {
            return found_policy_pair_list<repeated, FoundPolicyPairs...>{};
        }

        template <class FirstBaseDefaultPair,
                  class... remainingBaseDefaultPairs, bool repeated,
                  class... FoundPolicyPairs, class... Policies>
        constexpr auto
        make_policy_holder_impl(
            base_default_pair_list<FirstBaseDefaultPair,
                                   remainingBaseDefaultPairs...>,
            found_policy_pair_list<repeated, FoundPolicyPairs...>,
            Policies... policies) {
            using new_found_policy_pair =
                decltype(FirstBaseDefaultPair::get_policy(policies...));

            return make_policy_holder_impl(
                base_default_pair_list<remainingBaseDefaultPairs...>{},
                found_policy_pair_list < repeated ||
                    new_found_policy_pair::found_info ==
                        policy_found_info::repeated,
                new_found_policy_pair, FoundPolicyPairs... > {}, policies...);
        }

        template <bool repeated, class... raw_typePolicies>
        constexpr auto
        convert_to_policy_holder(found_policy_pair_list<repeated>,
                                 raw_typePolicies...) {
            return policy_holder<raw_typePolicies...>{};
        }

        template <bool repeated, class FirstFoundPolicyPair,
                  class... remainingFoundPolicyPairs, class... raw_typePolicies>
        constexpr auto
        convert_to_policy_holder(
            found_policy_pair_list<repeated, FirstFoundPolicyPair,
                                   remainingFoundPolicyPairs...>,
            raw_typePolicies... policies) {
            return convert_to_policy_holder(
                found_policy_pair_list<repeated,
                                       remainingFoundPolicyPairs...>{},
                typename FirstFoundPolicyPair::policy{}, policies...);
        }

        template <class BaseDefaultPairList, class... Policies>
        constexpr auto
        make_policy_holder(BaseDefaultPairList, Policies... policies) {
            static_assert(
                check_policy_list_validity(BaseDefaultPairList(),
                                           Policies()...),
                "cat::detail::dragonbox: an invalid policy is specified");

            using policy_pair_list = decltype(make_policy_holder_impl(
                BaseDefaultPairList(), found_policy_pair_list<false>{},
                policies...));

            static_assert(!policy_pair_list::repeated,
                          "cat::detail::dragonbox: each policy should be "
                          "specified at most once");

            return convert_to_policy_holder(policy_pair_list{});
        }
    }  // namespace policy_impl
}  // namespace detail

////////////////////////////////////////////////////////////////////////////////////////
// The interface function.
////////////////////////////////////////////////////////////////////////////////////////

template <class Float, class FloatTraits = default_float_traits<Float>,
          class... Policies>
JKJ_SAFEBUFFERS auto
to_decimal(signed_significand_bits<Float, FloatTraits> signed_significand_bits,
           unsigned int exponent_bits, Policies... policies) noexcept {
    // Build policy holder type.
    using namespace detail::policy_impl;
    using policy_holder = decltype(make_policy_holder(
        base_default_pair_list<
            base_default_pair<sign::base, sign::return_sign>,
            base_default_pair<trailing_zero::base, trailing_zero::remove>,
            base_default_pair<decimal_to_binary_rounding::base,
                              decimal_to_binary_rounding::nearest_to_even>,
            base_default_pair<binary_to_decimal_rounding::base,
                              binary_to_decimal_rounding::to_even>,
            base_default_pair<cache::base, cache::full>>{},
        policies...));

    using return_type = decimal_fp<typename FloatTraits::carrier_uint,
                                   policy_holder::return_has_sign,
                                   policy_holder::report_trailing_zeros>;

    return_type ret = policy_holder::delegate(
        signed_significand_bits,
        [exponent_bits, signed_significand_bits](auto interval_type_provider) {
            using format = FloatTraits::format;
            constexpr auto tag = decltype(interval_type_provider)::tag;

            auto two_fc = signed_significand_bits.remove_sign_bit_and_shift();
            auto exponent = int(exponent_bits);

            if constexpr (tag ==
                          decimal_to_binary_rounding::tag_t::to_nearest) {
                // Is the input a normal number?
                if (exponent != 0) {
                    exponent +=
                        format::exponent_bias - format::significand_bits;

                    // Shorter interval case; proceed like Schubfach.
                    // One might think this condition is wrong, since when
                    // exponent_bits == 1 and two_fc == 0, the interval is
                    // actually regular. However, it turns out that this
                    // seemingly wrong condition is actually fine, because the
                    // end result is anyway the same.
                    //
                    // [binary32]
                    // (fc-1/2) * 2^e = 1.175'494'28... * 10^-38
                    // (fc-1/4) * 2^e = 1.175'494'31... * 10^-38
                    //    fc    * 2^e = 1.175'494'35... * 10^-38
                    // (fc+1/2) * 2^e = 1.175'494'42... * 10^-38
                    //
                    // Hence, shorter_interval_case will return 1.175'494'4 *
                    // 10^-38. 1.175'494'3 * 10^-38 is also a correct shortest
                    // representation that will be rejected if we assume shorter
                    // interval, but 1.175'494'4 * 10^-38 is closer to the true
                    // value so it doesn't matter.
                    //
                    // [binary64]
                    // (fc-1/2) * 2^e = 2.225'073'858'507'201'13... * 10^-308
                    // (fc-1/4) * 2^e = 2.225'073'858'507'201'25... * 10^-308
                    //    fc    * 2^e = 2.225'073'858'507'201'38... * 10^-308
                    // (fc+1/2) * 2^e = 2.225'073'858'507'201'63... * 10^-308
                    //
                    // Hence, shorter_interval_case will
                    // return 2.225'073'858'507'201'4 * 10^-308. This is indeed
                    // of the shortest length, and it is the unique one closest
                    // to the true value among valid representations of the same
                    // length.
                    static_assert(cat::is_same<format, ieee754_binary32> ||
                                  cat::is_same<format, ieee754_binary64>);

                    if (two_fc == 0) {
                        return decltype(interval_type_provider)::
                            invoke_shorter_interval_case(
                                signed_significand_bits,
                                [exponent](auto... additional_args) {
                                    return detail::impl<Float, FloatTraits>::
                                        template compute_nearest_shorter<
                                            return_type,
                                            typename decltype(interval_type_provider)::
                                                shorter_interval_type,
                                            typename policy_holder::
                                                trailing_zero_policy,
                                            typename policy_holder::
                                                binary_to_decimal_rounding_policy,
                                            typename policy_holder::
                                                cache_policy>(
                                            exponent, additional_args...);
                                });
                    }

                    two_fc |=
                        (decltype(two_fc)(1) << (format::significand_bits + 1));
                }
                // Is the input a subnormal number?
                else {
                    exponent = format::min_exponent - format::significand_bits;
                }

                return decltype(interval_type_provider)::
                    invoke_normal_interval_case(
                        signed_significand_bits,
                        [two_fc, exponent](auto... additional_args) {
                            return detail::impl<Float, FloatTraits>::
                                template compute_nearest_normal<
                                    return_type,
                                    typename decltype(interval_type_provider)::
                                        normal_interval_type,
                                    typename policy_holder::
                                        trailing_zero_policy,
                                    typename policy_holder::
                                        binary_to_decimal_rounding_policy,
                                    typename policy_holder::cache_policy>(
                                    two_fc, exponent, additional_args...);
                        });
            } else if constexpr (tag == decimal_to_binary_rounding::tag_t::
                                            left_closed_directed) {
                // Is the input a normal number?
                if (exponent != 0) {
                    exponent +=
                        format::exponent_bias - format::significand_bits;
                    two_fc |=
                        (decltype(two_fc)(1) << (format::significand_bits + 1));
                }
                // Is the input a subnormal number?
                else {
                    exponent = format::min_exponent - format::significand_bits;
                }

                return detail::impl<Float>::
                    template compute_left_closed_directed<
                        return_type,
                        typename policy_holder::trailing_zero_policy,
                        typename policy_holder::cache_policy>(two_fc, exponent);
            } else {
                static_assert(
                    tag ==
                    decimal_to_binary_rounding::tag_t::right_closed_directed);

                bool shorter_interval = false;

                // Is the input a normal number?
                if (exponent != 0) {
                    if (two_fc == 0 && exponent != 1) {
                        shorter_interval = true;
                    }
                    exponent +=
                        format::exponent_bias - format::significand_bits;
                    two_fc |=
                        (decltype(two_fc)(1) << (format::significand_bits + 1));
                }
                // Is the input a subnormal number?
                else {
                    exponent = format::min_exponent - format::significand_bits;
                }

                return detail::impl<Float>::
                    template compute_right_closed_directed<
                        return_type,
                        typename policy_holder::trailing_zero_policy,
                        typename policy_holder::cache_policy>(two_fc, exponent,
                                                              shorter_interval);
            }
        });

    policy_holder::handle_sign(signed_significand_bits, ret);
    return ret;
}

template <class Float, class FloatTraits = default_float_traits<Float>,
          class... Policies>
auto
to_decimal(Float x, Policies... policies) noexcept {
    auto const br = float_bits<Float, FloatTraits>(x);
    auto const exponent_bits = br.extract_exponent_bits();
    auto const s = br.remove_exponent_bits(exponent_bits);
    // assert(br.is_finite());

    return to_decimal<Float, FloatTraits>(s, exponent_bits, policies...);
}
}  // namespace cat::detail::dragonbox

#undef JKJ_FORCEINLINE
#undef JKJ_SAFEBUFFERS
#undef JKJ_DRAGONBOX_HAS_BUILTIN

namespace cat::detail::dragonbox {
namespace to_chars_detail {
    template <class Float, class FloatTraits>
    extern char* to_chars(typename FloatTraits::carrier_uint significand,
                          int exponent, char* buffer) noexcept;

    // Avoid needless ABI overhead incurred by tag dispatch.
    template <class PolicyHolder, class Float, class FloatTraits>
    char*
    to_chars_n_impl(float_bits<Float, FloatTraits> br, char* buffer) noexcept {
        auto const exponent_bits = br.extract_exponent_bits();
        auto const s = br.remove_exponent_bits(exponent_bits);

        if (br.is_finite(exponent_bits)) {
            if (s.is_negative()) {
                *buffer = '-';
                ++buffer;
            }
            if (br.is_nonzero()) {
                auto result = to_decimal<Float, FloatTraits>(
                    s, exponent_bits, policy::sign::ignore,
                    policy::trailing_zero::remove,
                    typename PolicyHolder::decimal_to_binary_rounding_policy{},
                    typename PolicyHolder::binary_to_decimal_rounding_policy{},
                    typename PolicyHolder::cache_policy{});
                return to_chars_detail::to_chars<Float, FloatTraits>(
                    result.significand, result.exponent, buffer);
            }
            copy_memory_small("0E0", buffer, 3u);
            return buffer + 3;
        }
        if (s.has_all_zero_significand_bits()) {
            if (s.is_negative()) {
                *buffer = '-';
                ++buffer;
            }
            copy_memory_small("Infinity", buffer, 8u);
            return buffer + 8;
        } else {
            copy_memory_small("NaN", buffer, 3u);
            return buffer + 3;
        }
    }
}  // namespace to_chars_detail

// Returns the next-to-end position
template <class Float, class FloatTraits = default_float_traits<Float>,
          class... Policies>
char*
to_chars_n(Float x, char* buffer, Policies... policies) noexcept {
    using namespace cat::detail::dragonbox::detail::policy_impl;
    using policy_holder = decltype(make_policy_holder(
        base_default_pair_list<
            base_default_pair<decimal_to_binary_rounding::base,
                              decimal_to_binary_rounding::nearest_to_even>,
            base_default_pair<binary_to_decimal_rounding::base,
                              binary_to_decimal_rounding::to_even>,
            base_default_pair<cache::base, cache::full>>{},
        policies...));

    return to_chars_detail::to_chars_n_impl<policy_holder>(
        float_bits<Float, FloatTraits>(x), buffer);
}

// Null-terminate and bypass the return value of fp_to_chars_n
template <class Float, class FloatTraits = default_float_traits<Float>,
          class... Policies>
char*
to_chars(Float x, char* buffer, Policies... policies) noexcept {
    auto ptr = to_chars_n<Float, FloatTraits>(x, buffer, policies...);
    *ptr = '\0';
    return ptr;
}

// Maximum required buffer size (excluding null-terminator)
template <class FloatFormat>
inline constexpr uword::raw_type max_output_string_length =
    cat::is_same<FloatFormat, ieee754_binary32>
        ?
        // sign(1) + significand(9) + decimal_point(1) + exp_marker(1) +
        // exp_sign(1) + exp(2)
        (1 + 9 + 1 + 1 + 1 + 2)
        :
        // format == ieee754_format::binary64
        // sign(1) + significand(17) + decimal_point(1) + exp_marker(1) +
        // exp_sign(1) + exp(3)
        (1 + 17 + 1 + 1 + 1 + 3);
}  // namespace cat::detail::dragonbox

// NOLINTEND
