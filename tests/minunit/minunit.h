// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* Most unit-testing frameworks depend on libC or STL features which are not
 * implemented in libCat, so MinUnit is used due to its simplicity. */
// https://jera.com/techinfo/jtns/jtn002

#define mu_assert(test, message) \
    do {                         \
        if (!(test)) exit(1);    \
    } while (0)

#define mu_run_test(test)                \
    do {                                 \
        char8_t const* message = test(); \
        tests_run++;                     \
        if (message) exit(1);            \
    } while (0)

extern i4 tests_run;
