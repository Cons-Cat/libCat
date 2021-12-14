#pragma once

#include <concepts.h>

/* Safe arithmetic does compile with no known issues. However, clangd 12 and 13
 * keel over after a single glance at this metaprogramming, so I am currently
 * hiding this behind a feature flag. */
#ifdef SAFE_ARITHMETIC

/* Get the value of a primitive number, or the value held by a
 * safe_numeral_t<>. */
constexpr auto decay_numeral(int_or_float auto from) {
    return from;
}
constexpr auto decay_numeral(auto from) {
    return from.data;
}

namespace std::detail {

/* safe_numeral_t<> is the most interesting part of <stdint.h>. libCat uses
 * this struct to represent all of its scalar numerical data types.
 * Operations between two specializations of a safe_numeral_t<> that would
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

template <typename T>
struct safe_numeral_t {
    // TODO: Rename to value
    T data;  // Uninitialized by default.

    constexpr safe_numeral_t() = default;
    // Any number that is smaller than this can safely cast into it.
    constexpr safe_numeral_t(auto from) requires(
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

    auto operator=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
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

    auto operator+(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data + decay_numeral(from);
    }
    auto operator+=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data + decay_numeral(from);
        return *this;
    }

    auto operator++() -> safe_numeral_t<T> {
        return ++data;
    }
    auto operator++(int) -> safe_numeral_t<T> {
        return data++;
    }

    auto operator-(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data - decay_numeral(from);
    }
    auto operator-=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this - from;
        return *this;
    }

    auto operator--() -> safe_numeral_t<T> {
        return --data;
    }
    auto operator--(int) -> safe_numeral_t<T> {
        return data--;
    }

    auto operator*(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data * decay_numeral(from);
    }
    auto operator*=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data * from.data;
        return *this;
    }

    auto operator/(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data / decay_numeral(from);
    }
    auto operator/=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data / from.data;
        return *this;
    }

    auto operator%(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data % decay_numeral(from);
    }
    auto operator%=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data % from.data;
        return *this;
    }

    auto operator&(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data & decay_numeral(from);
    }
    auto operator&=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data & from.data;
        return *this;
    }

    auto operator|(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data | decay_numeral(from);
    }
    auto operator|=(auto from) -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data | from.data;
        return *this;
    }

    auto operator<<(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data << decay_numeral(from);
    }
    auto operator<<=(auto from)
        -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data << from.data;
        return *this;
    }

    auto operator>>(auto from) -> safe_numeral_t<T>
    requires(REQUIRES_HELPER) {
        return this->data >> decay_numeral(from);
    }
    auto operator>>=(auto from)
        -> safe_numeral_t<T>& requires(REQUIRES_HELPER) {
        this->data = this->data >> from.data;
        return *this;
    }
};

}  // namespace std::detail

// These macros are defined by the GCC compiler.
using i8 = std::detail::safe_numeral_t<__INT8_TYPE__>;
using u8 = std::detail::safe_numeral_t<__UINT8_TYPE__>;
using i16 = std::detail::safe_numeral_t<__INT16_TYPE__>;
using u16 = std::detail::safe_numeral_t<__UINT16_TYPE__>;
using i32 = std::detail::safe_numeral_t<__INT32_TYPE__>;
using u32 = std::detail::safe_numeral_t<__UINT32_TYPE__>;
using i64 = std::detail::safe_numeral_t<__INT64_TYPE__>;
using u64 = std::detail::safe_numeral_t<__UINT64_TYPE__>;
// using i128 = std::detail::safe_numeral_t<int128_t>;
// using u128 = std::detail::safe_numeral_t<uint128_t>;

using usize = u64;
using isize = i64;

// These are GCC built-in types:
// using f16 = _Float16;
using f32 = std::detail::safe_numeral_t<float>;
using f64 = std::detail::safe_numeral_t<double>;
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
