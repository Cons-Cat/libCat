// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <start.h>

/* Advance the `%rsp` stack-pointer register to the nearest 16-byte aligned
 * address. */
void align_stack_pointer_16() {
    asm("and $-16, %rsp");
}
