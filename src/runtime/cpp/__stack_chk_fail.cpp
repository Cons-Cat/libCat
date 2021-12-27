// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <start.h>

/* `__stack_chk_fail()` is called when stack overflow occurs in programs
 * compiled without `-fno-stack-protector`. This will terminate the program with
 * exit code `1`. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
