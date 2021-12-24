// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* This file should be implicitly included in all other files. With GCC, this is
   done using the --include flag, as in --include global_includes.h */
extern "C" __attribute__((used)) void meow();

/* Many debuggers, including GDB, will enter by default at the symbol
 * .main. Because main() is not used in libCat, an alias to it is required for
 * mocking an entry point to debuggers. */
// TODO: Prevent this from being optimized out in RelWithDebInfo.
auto main() -> int{};
__attribute__((alias("main"))) auto debugger_entry_point() -> int;

template <typename T>
struct Result;

// TODO: Fix this declaration in <numerals.h>
using isize = int long long;

#include <numerals.h>

// using isize = int long long;

// Including <start.h> and <exit.h> is required to link a libCat program.
#include <exit.h>
#include <start.h>

/* <result.h> and <numerals.h> are analogous to exceptions and stdint.h in that
 * they are used throughout the library and should be accessible to a user by
 * default. */
#include <result.h>

// <stdint.h> is required for libC compatibility.
#include <stdint.h>

/* libCat includes <simd.h> globally, because vectors are considered primitive
 * types. */
#include <simd.h>

/* libCat provides a `_` variable that consumes a function's output, but cannot
 * be assigned to any variable. */
namespace std::detail {

struct __attribute__((unused)) unused {
    // Any type can be converted into `unused`, except for `Result`s.
    template <typename T>
    void operator=(T const&) requires(
        !meta::is_specialization<T, Result>::value) {
    };
    // `unused` cannot be assigned to any variable.
    template <typename T>
    operator T() = delete;
};

}  // namespace std::detail
std::detail::unused _{};
