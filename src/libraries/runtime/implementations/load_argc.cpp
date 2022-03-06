// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <runtime>

auto load_argc() -> int4 {
    int4 argc;
    asm("mov 0(%%rbp), %[argc]" : [argc] "=r"(argc));
    return argc;
}
