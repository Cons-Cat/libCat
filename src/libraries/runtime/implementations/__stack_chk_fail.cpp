// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <runtime>

/* `__stack_chk_fail()` is called when stack overflow occurs in programs
 * compiled without `-fno-stack-protector`. This will terminate the program with
 * exit code `1`. */
extern "C" void cat::__stack_chk_fail() {
    cat::exit(1);
}
