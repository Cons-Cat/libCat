// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <unistd.h>

auto std::detail::syscall6(Any call, Any arg1, Any arg2, Any arg3, Any arg4,
                           Any arg5, Any arg6) -> Result<Any> {
    register Any r10 asm("r10") = arg4;
    register Any r8 asm("r8") = arg5;
    register Any r9 asm("r9") = arg6;

    Any result;
    asm("syscall"
        : "=a"(result)
        : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8),
          "r"(r9),
          // TODO: Fix this segfaulting with static_cast<void*>():
          [a6] "re"(meta::bit_cast<void*>(arg6))
        : "rcx", "r11", "memory", "cc");
    if (meta::bit_cast<i8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<i8>(result));
    }
    return result;
}
