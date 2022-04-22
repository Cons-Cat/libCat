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

}  // namespace cat::detail

// `_` can consume any value to explicitly disregard a ``[[nodiscard]]``
// attribute from a function with side effects. Consuming a `Result` value is
// not possible.
[[maybe_unused]] inline cat::detail::Unused _;

template <typename T>
class Result;

namespace cat::detail {
template <typename T>
struct SafeNumeral;
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

// <stdint.h> is required for libC compatibility.
#include <stdint.h>

/* libCat includes `<simd>` globally, because vectors are considered primitive
 * types. */
#include <simd>

// Placement `new`.
[[nodiscard]] inline auto operator new(unsigned long, void* p_address)
    -> void* {
    return p_address;
}
