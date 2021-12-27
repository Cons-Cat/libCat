// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <concepts.h>

/* Safe arithmetic does compile with no known issues. However, clangd 12 and 13
 * keel over after a single glance at this metaprogramming, so I am currently
 * hiding this behind a feature flag. It likely doesn't work anymore, because I
 * have been developing libCat mostly without safe arithmetic due to clangd. */
#ifdef SAFE_ARITHMETIC

namespace meta {

/* Get the value of a primitive number, or the value held by a
 * SafeNumeral<>. */
constexpr auto decay_numeral(int_or_float auto from) {
    return from;
}
constexpr auto decay_numeral(auto from) {
    return from.data;
}

}  // namespace meta

namespace std::detail {

/* SafeNumeral<> is the most interesting part of <stdint.h>. libCat uses
 * this struct to represent all of its scalar numerical data types.
 * Operations between two specializations of a SafeNumeral<> that would
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
 * i8 and an i4 type. This solution is very concise because that same
 * constraint broadly applies to all possible type conversions. */

template <typename T>
struct SafeNumeral {
    // TODO: Rename to value
    T data;  // Uninitialized by default.

    constexpr SafeNumeral() = default;
    // Any number that is smaller than this can safely cast into it.
    constexpr SafeNumeral(auto from) requires(
        std::is_signed_v<T> ==
            std::is_signed_v<decltype(decay_numeral(from))> &&
        sizeof(T) >= sizeof(decltype(decay_numeral(from))) &&
        std::is_floating_point_v<T> ==
            std::is_floating_point_v<decltype(decay_numeral(from))>) {
        this->data = decay_numeral(from);
    }
    // TODO: Make a concept that limits signed-ness and size.
    constexpr operator std::int_or_float auto() const {
        return this->data;
    };

        /* TODO: Putting concepts or constexpr functions in these requires
         * clauses doesn't work, for some reason. */
#define REQUIRES_HELPER                                                       \
    std::is_signed_v<T> == std::is_signed_v<decltype(decay_numeral(from))> && \
        sizeof(T) >= sizeof(decltype(decay_numeral(from))) &&                 \
        std::is_floating_point_v<T> ==                                        \
            std::is_floating_point_v<decltype(decay_numeral(from))>

    auto operator=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = decay_numeral(from);
        return *this;
    }
    auto operator==(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data == decay_numeral(from);
    }

    auto operator>(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data > decay_numeral(from);
    }
    auto operator>=(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data >= decay_numeral(from);
    }

    auto operator<(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data < decay_numeral(from);
    }
    auto operator<=(auto from) -> bool requires(REQUIRES_HELPER) {
        return this->data <= decay_numeral(from);
    }

    auto operator+(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data + decay_numeral(from);
    }
    auto operator+=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data + decay_numeral(from);
        return *this;
    }

    auto operator++() -> SafeNumeral<T> {
        return ++data;
    }
    auto operator++(int) -> SafeNumeral<T> {
        return data++;
    }

    auto operator-(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data - decay_numeral(from);
    }
    auto operator-=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this - from;
        return *this;
    }

    auto operator--() -> SafeNumeral<T> {
        return --data;
    }
    auto operator--(int) -> SafeNumeral<T> {
        return data--;
    }

    auto operator*(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data * decay_numeral(from);
    }
    auto operator*=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data * from.data;
        return *this;
    }

    auto operator/(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data / decay_numeral(from);
    }
    auto operator/=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data / from.data;
        return *this;
    }

    auto operator%(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data % decay_numeral(from);
    }
    auto operator%=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data % from.data;
        return *this;
    }

    auto operator&(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data & decay_numeral(from);
    }
    auto operator&=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data & from.data;
        return *this;
    }

    auto operator|(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data | decay_numeral(from);
    }
    auto operator|=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data | from.data;
        return *this;
    }

    auto operator<<(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data << decay_numeral(from);
    }
    auto operator<<=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data << from.data;
        return *this;
    }

    auto operator>>(auto from) -> SafeNumeral<T>
    requires(REQUIRES_HELPER) {
        return this->data >> decay_numeral(from);
    }
    auto operator>>=(auto from) -> SafeNumeral<T>& requires(REQUIRES_HELPER) {
        this->data = this->data >> from.data;
        return *this;
    }
};

}  // namespace std::detail

// These macros are defined by the GCC compiler.
using i1 = std::detail::SafeNumeral<__INT8_TYPE__>;
using u1 = std::detail::SafeNumeral<__UINT8_TYPE__>;
using i2 = std::detail::SafeNumeral<__INT16_TYPE__>;
using u2 = std::detail::SafeNumeral<__UINT16_TYPE__>;
using i4 = std::detail::SafeNumeral<__INT32_TYPE__>;
using u4 = std::detail::SafeNumeral<__UINT32_TYPE__>;
using i8 = std::detail::SafeNumeral<__INT64_TYPE__>;
using u8 = std::detail::SafeNumeral<__UINT64_TYPE__>;
// using i128 = std::detail::SafeNumeral<int128_t>;
// using u128 = std::detail::SafeNumeral<uint128_t>;

// These are GCC built-in types:
using f4 = std::detail::SafeNumeral<float>;
using f8 = std::detail::SafeNumeral<double>;
#else

namespace meta {

constexpr auto decay_numeral(auto from) {
    return from;
}

}  // namespace meta

// TODO: Add and test 16-byte types and complex numbers.
// These macros are defined by the GCC compiler.
using i1 = __INT8_TYPE__;
using u1 = __UINT8_TYPE__;
using i2 = __INT16_TYPE__;
using u2 = __UINT16_TYPE__;
using i4 = __INT32_TYPE__;
using u4 = __UINT32_TYPE__;
using i8 = __INT64_TYPE__;
using u8 = __UINT64_TYPE__;
using f4 = float;
using f8 = double;
#endif

using usize = u8;
// TODO: Solve circular dependency:
// using isize = i8;

using bool1 = u1;
using bool2 = u2;
using bool4 = u4;
