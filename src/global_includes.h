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

template <typename T>
class Result;

// TODO: Fix this declaration in `<numerals>`
using ssize = signed long long;

#include <numerals>

// Including the `<runtime>` library is required to link a libCat program.
#include <runtime>

/* `<result>` and `<numerals>` are analogous to exceptions and `<stdint.h>` in
 * that they are used throughout the library and should be accessible to a user
 * by default. */
#include <result>

// <stdint.h> is required for libC compatibility.
#include <stdint.h>

/* libCat includes `<simd>` globally, because vectors are considered primitive
 * types. */
#include <simd>

/* libCat provides a `_` variable that consumes a function's output, but cannot
 * be assigned to any variable. */
namespace cat::detail {

struct [[maybe_unused]] Unused {
    // Any type can be converted into an `Unused`, except for `Result`.
    template <typename T>
    constexpr void operator=(T const&) requires(
        !meta::is_specialization<T, Result>::value) {
    };
    // `unused` cannot be assigned to any variable.
    operator auto() = delete;
};

}  // namespace cat::detail

// `_` can consume any value to explicitly disregard a ``[[nodiscard]]``
// attribute from a function with side effects. Consuming a `Result` value is
// not possible.
[[maybe_unused]] inline cat::detail::Unused _;
