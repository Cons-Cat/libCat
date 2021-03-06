// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// This file should be implicitly included in all other files. With GCC, this is
// done using the `--include` flag, as in `--include global_includes.h`. The
// `CMakeLists.txt` in this repository's top level directory does this.

// libCat provides a `_` variable that consumes a function's output, but
// cannot be assigned to any variable.
namespace cat {
namespace detail {
    struct [[maybe_unused]] Unused {
        template <typename T>
        constexpr void operator=(T const&){};
        // `Unused` cannot be assigned to any variable.
        operator auto() = delete;
        // `Unused` cannot be assigned to itself, i.e. `_ = _`.
        auto operator=(Unused) = delete;
    };

    // `InPlace` is consumed by wrapper classes to default-initialize their
    // storage.
    struct InPlace {};
}  // namespace detail

// A `Monostate` is an object that can hold anything, and convert into
// anything or from anything. It has no storage or behavior.
struct Monostate {
    constexpr Monostate() = default;
    // constexpr Monostate(auto){};
    constexpr operator auto(){};
};

template <typename T, T state>
struct MonostateStorage {
    T storage;

    constexpr MonostateStorage() = default;
    constexpr MonostateStorage(Monostate&) : storage(state){};
    constexpr MonostateStorage(Monostate const&) : storage(state){};
    constexpr MonostateStorage(T input) : storage(input){};

    constexpr operator auto() const {
        return this->storage;
    };

    constexpr auto operator=(Monostate) -> MonostateStorage<T, state>& {
        return *this;
    }

    // TODO: Support all non-assignment operators that `T` has.
    constexpr auto operator==(auto operand) -> bool {
        return state == operand;
    }
    constexpr auto operator!=(auto operand) -> bool {
        return state != operand;
    }
    constexpr auto operator<(auto operand) -> bool {
        return state < operand;
    }
    constexpr auto operator<=(auto operand) -> bool {
        return state <= operand;
    }
    constexpr auto operator>(auto operand) -> bool {
        return state > operand;
    }
    constexpr auto operator>=(auto operand) -> bool {
        return state >= operand;
    }
};

template <typename T, auto predicate, T sentinel>
    requires(!predicate(sentinel))
struct Compact {
    using Type = T;
    static constexpr auto predicate_function = predicate;
    static constexpr Type sentinel_value = Type{sentinel};
    // `Compact`s can only be instantiated at compile-time.
    consteval Compact() = default;
};

namespace detail {
    // This is a function instead of a lambda to fix clangd crashes.
    template <typename T, T sentinel>
    constexpr auto sentinel_predicate(T value) -> bool {
        return value != sentinel;
    }
}  // namespace detail

template <typename T, T sentinel>
using Sentinel = Compact<T, detail::sentinel_predicate<T, sentinel>, sentinel>;

namespace detail {
    template <typename T>
    struct Numeral;
}

}  // namespace cat

// `_` can consume any value to explicitly disregard a ``[[nodiscard]]``
// attribute from a function with side effects. Consuming a `Result` value is
// not possible.
[[maybe_unused]] inline cat::detail::Unused _;

// `in_place` is consumed by wrapper classes to default-initialize their
// storage.
inline constexpr cat::detail::InPlace in_place;

// `monostate` can be consumed by wrapper classes to represent no storage.
inline constexpr cat::Monostate monostate;

template <typename T>
class Result;

// Including the `<cat/runtime>` library is required to link a libCat program,
// because it contains the `_start` symbol.
#include <cat/runtime>

// TODO: Remove this.
// `Result` is used throughout the library.
#include <result>

// Placement `new`.
[[nodiscard]] constexpr auto operator new(unsigned long, void* p_address)
    -> void* {
    return p_address;
}
