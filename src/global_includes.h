// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* This file should be implicitly included in all other files. With GCC, this is
 * done using the `--include` flag, as in `--include global_includes.h`. The
 * `CMakeLists.txt` in this repository's top level directory does this. */
extern "C" [[gnu::used]] void meow(
#ifdef load_argc_argv
    int argc, char* argv[]
#endif
);

/* libCat provides a `_` variable that consumes a function's output, but cannot
 * be assigned to any variable. */
namespace cat::detail {

struct [[maybe_unused]] Unused {
    // Any type can be converted into an `Unused`, except for `Result`.
    template <typename T>
    constexpr void operator=(T const&){};
    // `unused` cannot be assigned to any variable.
    operator auto() = delete;
};

// A `Monostate` is an object that can hold anything, and convert into anything
// or from anything. It has no storage or behavior.
struct Monostate {
    constexpr Monostate() = default;
    // constexpr Monostate(auto){};
    constexpr operator auto(){};
};

// `InPlace` is consumed by wrapper classes to default-initialize their storage.
struct InPlace {};

}  // namespace cat::detail

// `_` can consume any value to explicitly disregard a ``[[nodiscard]]``
// attribute from a function with side effects. Consuming a `Result` value is
// not possible.
[[maybe_unused]] inline cat::detail::Unused _;

// `in_place` is consumed by wrapper classes to default-initialize their
// storage.
inline constexpr cat::detail::InPlace in_place;

// `monostate` can be consumed by wrapper classes to represent no storage.
inline constexpr cat::detail::Monostate monostate;

namespace cat {

template <typename T, auto predicate, T sentinel>
requires(!predicate(sentinel)) struct Predicate {
    using PredicateType = T;
    static constexpr auto predicate_function = predicate;
    static constexpr T sentinel_value = sentinel;
    // `Predicate`s can only be instantiated at compile-time.
    consteval Predicate() = default;
};

template <typename T, T sentinel>
using Sentinel = Predicate<T,
                           [](T const value) {
                               return value != sentinel;
                           },
                           sentinel>;

}  // namespace cat

template <typename T>
class Result;

namespace cat::detail {
template <typename T>
struct Numeral;
}

namespace meta {
template <typename T>
constexpr auto bit_cast(auto& from_value) -> T;
}

// `<numerals>` should make basic data types global, similarly to `<stdint.h>`
// being globally available.
#include <numerals>

// Including the `<runtime>` library is required to link a libCat program.
#include <runtime>

// `Result` is used throughout the library.
#include <result>

// Placement `new`.
[[nodiscard]] inline auto operator new(unsigned long, void* p_address)
    -> void* {
    return p_address;
}
