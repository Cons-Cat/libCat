// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <runtime>

/* Get a pointer to an array of UTF-8 encoded command-line arguments. This array
 * is 1 KiB long. */
auto load_argv() -> char** {
    char** argv;
    asm("lea 8(%%rbp), %[argv]" : [argv] "=r"(argv));
    return argv;
}
