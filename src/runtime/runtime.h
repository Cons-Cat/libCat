// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <concepts.h>
#include <type_traits.h>

/* `meow()` is the user's entry point in a libCat program, rather than `main()`.
 * But hear meowt! This allows libCat to explore interesting ideas, such as
 * enabling the user to express explicit stack initialization, and giving them
 * generally more control over their program than the standard `crt0` or `crt1`
 * do.
 *
 * That control is very generally useful. Do you need to support `.init` and
 * `.fini`, or do you only use `.ctor` or `.dtor` instead? Or both? Or neither?
 * POSIX and ELF require libC support all four, technically, but there is
 * usually no tangible reason to do so.
 *
 * Does your program need to allocate a stack at all, or can you just use the
 * space you're already given in `argv[]`? Do you have to populate `argp[]`?
 * Does your program need to create a main `pthread`, or not? Should you align
 * the stack, or is that unnecessary? There are myriad creative liberties
 * available once you rethink basic assumptions about what C++ actually does.
 *
 * As far as the name change from `main()` goes, GCC simply won't compile when
 * it thinks the `main()` definition is ill-formed, so this change *is*
 * necessary for fundamentally different behavior, such as not returning `int`.
 * Meow is four letters, it starts with 'm', and it sounds cute, so it seems as
 * good as anything else. */

/* TODO: #ifdef __SANITIZE_ADDRESS__
 * to handle address sanitizers, which do not work. */

/* in libCat, `_start()` does almost nothing. It is necessary to store the
 * base stack pointer for future use, because that is impossible after memory is
 * pushed to the stack, but otherwise all initialization logic shall occur in
 * `meow()`.
 *
 * an `_exit` syscall is also not made in `_start()`, because GCC cannot inline
 * `call` assembly instructions, and a more sophisticated exit strategy may be
 * desired by a user anyways. For instance, stack unwinding or zero-ing out
 * memory. In libCat, program termination is completely explicit. */
extern "C" [[gnu::used]] void _start();

// TODO: Use `isize` here:
/* In libCat, the `exit()` function is provided globally.
 * This streamlines out the existence of `_exit()`. */
extern "C" [[gnu::used]] void exit(i8 exit_code);

auto load_base_stack_pointer() -> void*;
auto load_argc() -> i4;
auto load_argv() -> char8_t**;
void align_stack_pointer_16();
void align_stack_pointer_32();
void dont_optimize_out(auto& variable);
extern "C" void __stack_chk_fail();
