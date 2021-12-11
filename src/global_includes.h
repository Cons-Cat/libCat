#pragma once

/* This file should be implicitly included in all other files. With GCC, this is
   done using the --include flag, as in --include global_includes.h */

extern "C" __attribute__((used)) void meow();

// Including <start.h> and <exit.h> is required to link a libCat program.
#include <exit.h>
#include <start.h>

/* <result.h> and <numerals.h> are analogous to exceptions and stdint.h in that
 * they are used throughout the library and should be accessible to a user by
 * default. */
#include <numerals.h>
#include <result.h>

// libCat includes <simd.h>, because vectors are considered primitive types.
#include <simd.h>
