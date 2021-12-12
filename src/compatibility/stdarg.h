// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* In C++, va_list is essentially useless. It produces inferior codegen to
 * analogous procedures using variadic templates due to redundant copies and
 * making function inlining unreliable. It is also less flexible. Moreover,
 * it can only be fully implemented with preprocessor macros. This header is
 * provided only for backwards compatibility with some libC-based code. */

using va_list = __builtin_va_list;

#define va_start(values, length) __builtin_va_start(values, length)
#define va_end(values) __builtin_va_end(values)
#define va_arg(values, length) __builtin_va_arg(values, length)
#define va_copy(destination, source) __builtin_va_copy(destination, source)
