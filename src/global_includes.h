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
struct Result;

// TODO: Fix this declaration in `<numerals>`
using ssize = int long long;

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
namespace std::detail {

struct [[maybe_unused]] unused {
    // Any type can be converted into `unused`, except for `Result`s.
    template <typename T>
    constexpr void operator=(T const&) requires(
        !meta::is_specialization<T, Result>::value) {
    };
    // `unused` cannot be assigned to any variable.
    operator auto() = delete;
};

}  // namespace std::detail

// TODO: Ensure that this static global is zero-overhead.
[[maybe_unused]] static std::detail::unused _;
