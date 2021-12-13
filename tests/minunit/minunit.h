// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* Most unit-testing frameworks depend on libC or STL features which are not
 * implemented in libCat, so MinUnit is used due to its simplicity. */
// https://jera.com/techinfo/jtns/jtn002

#define mu_assert(message, test)     \
    do {                             \
        if (!(test)) return message; \
    } while (0)

#define mu_run_test(test)             \
    do {                              \
        char const* message = test(); \
        tests_run++;                  \
        if (message) return message;  \
    } while (0)

extern i32 tests_run;
