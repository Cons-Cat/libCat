#pragma once

#include <concepts.h>

/* Safe arithmetic does compile with no known issues. However, clangd 12 and 13
 * keel over after a single glance at this metaprogramming, so I am currently
 * hiding this behind a feature flag. */
#ifdef SAFE_ARITHMETIC

// TODO: Rename to decay_safe_value;
/* Get the value of a primitive number, or the value held by a
 * safe_integral_t<>. */
constexpr auto decay_integral(std::int_or_float auto from) {
    return from;
}
constexpr auto decay_integral(auto from) {
    return from.data;
}

namespace std::detail {

/* safe_integral_t<> is the most interesting part of <stdint.h>. libCat uses
 * this struct to represent all of its scalar numerical data types.
 * Operations between two specializations of a safe_integral_t<> that would
 * cause a narrowing conversion such as loss of precision, change in
 * signed-ness, or an implicit float to int conversion, *do not compile*, and
 * the code to ensure this is very simple compared to many other mechanisms.
 *
 * This works because all of its implicit conversion operators, arithmetic
 * operators, and constructors are generic with constraints. In other words,
 * these methods do not exist until they are actually called somewhere.
 *
 * At a method's call site, the compiler will attempt to find an overload of the
 * generic method that satisfies the inputs you provided. Because these methods
 * are constrained by concepts, it is possible that the compiler will fail to
 * find any satisfying overload, and thus not compile.
 *
 * So, we can provide constraints such as "the operand must have a size that is
 * less than or equal to my own size", and this will guarantee that the compiler
 * cannot generate a method that could cause a narrowing conversion between an
 * i64 and an i32 type. This solution is very concise because that same
 * constraint broadly applies to all possible type conversions. */

// TODO: Rename to safe_value_t
template <typename T>
struct safe_integral_t {
    // TODO: Rename to value
    T data;  // Uninitialized by default.

    constexpr safe_integral_t() = default;
    // Any number that is smaller than this can safely cast into it.
    constexpr safe_integral_t(auto from) requires(
        std::is_signed_v<T> ==
            std::is_signed_v<decltype(decay_integral(from))> &&
        sizeof(T) >= sizeof(decltype(decay_integral(from))) &&
        std::is_floating_point_v<T> ==
            std::is_floating_point_v<decltype(decay_integral(from))>) {
        this->data = decay_integral(from);
    }
    // TODO: Make a concept that limits signed-ness and size.
    constexpr operator std::int_or_float auto() const {
        return this->data;
    };

        /* TODO: Putting concepts or constexpr functions in these requires
         * clauses doesn't work, for some reason. */
#define REQUIRES_HELPER                                                        \
    std::is_signed_v<T> == std::is_signed_v<decltype(decay_integral(from))> && \
        sizeof(T) >= sizeof(decltype(decay_integral(from))) &&                 \
        std::is_floating_point_v<T> ==                                         \
            std::is_floating_point_v<decltype(decay_integral(from))>

    auto operator=(auto from) -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = decay_integral(from);
        return *this;
    }
    auto operator==(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data == decay_integral(from);
    }

    auto operator>(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data > decay_integral(from);
    }
    auto operator>=(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data >= decay_integral(from);
    }

    auto operator<(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data < decay_integral(from);
    }
    auto operator<=(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data <= decay_integral(from);
    }

    auto operator+(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data + decay_integral(from);
    }
    auto operator+=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data + decay_integral(from);
        return *this;
    }

    auto operator++() -> safe_integral_t<T> {
        return ++data;
    }
    auto operator++(int) -> safe_integral_t<T> {
        return data++;
    }

    auto operator-(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data - decay_integral(from);
    }
    auto operator-=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this - from;
        return *this;
    }

    auto operator--() -> safe_integral_t<T> {
        return --data;
    }
    auto operator--(int) -> safe_integral_t<T> {
        return data--;
    }

    auto operator*(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data * decay_integral(from);
    }
    auto operator*=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data * from.data;
        return *this;
    }

    auto operator/(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data / decay_integral(from);
    }
    auto operator/=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data / from.data;
        return *this;
    }

    auto operator%(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data % decay_integral(from);
    }
    auto operator%=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data % from.data;
        return *this;
    }

    auto operator&(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data & decay_integral(from);
    }
    auto operator&=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data & from.data;
        return *this;
    }

    auto operator|(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data | decay_integral(from);
    }
    auto operator|=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data | from.data;
        return *this;
    }

    auto operator<<(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data << decay_integral(from);
    }
    auto operator<<=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data << from.data;
        return *this;
    }

    auto operator>>(auto from) -> safe_integral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data >> decay_integral(from);
    }
    auto operator>>=(auto from)
        -> safe_integral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data >> from.data;
        return *this;
    }
};

}  // namespace std::detail

/* These must be type aliases, because derived structs cannot inherit operator
 * overloading. */
using i8 = std::detail::safe_integral_t<signed char>;
using u8 = std::detail::safe_integral_t<unsigned char>;
using i16 = std::detail::safe_integral_t<signed short>;
using u16 = std::detail::safe_integral_t<unsigned short>;
using i32 = std::detail::safe_integral_t<signed int>;
using u32 = std::detail::safe_integral_t<unsigned int>;
using i64 = std::detail::safe_integral_t<signed long>;
using u64 = std::detail::safe_integral_t<unsigned long>;
using i128 = std::detail::safe_integral_t<int128_t>;
using u128 = std::detail::safe_integral_t<uint128_t>;

using usize = u64;
using isize = i64;

// These are GCC built-in types:
// using f16 = _Float16;
using f32 = std::detail::safe_integral_t<float>;
using f64 = std::detail::safe_integral_t<double>;
using f128 = __float128;
using x128 = float __attribute__((mode(TC))) _Complex;
#else

constexpr auto decay_numeral(auto from) {
    return from;
}

// These macros are defined by the GCC compiler.
using i8 = __INT8_TYPE__;
using u8 = __UINT8_TYPE__;
using i16 = __INT16_TYPE__;
using u16 = __UINT16_TYPE__;
using i32 = __INT32_TYPE__;
using u32 = __UINT32_TYPE__;
using i64 = __INT64_TYPE__;
using u64 = __UINT64_TYPE__;
using i128 = __int128;
using u128 = unsigned __int128;
using usize = u64;
using isize = i64;
using f32 = float;
using f64 = double;
// These are GCC built-in types:
using f128 = __float128;
using x128 = float __attribute__((mode(TC))) _Complex;
#endif

using bool8 = u8;
using bool16 = u16;
using bool32 = u32;
