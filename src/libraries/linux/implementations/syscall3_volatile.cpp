#include <cat/linux>

[[gnu::no_sanitize_address]]
auto
nix::syscall3_volatile(cat::iword call, cat::no_type arg1, cat::no_type arg2,
                       cat::no_type arg3) -> cat::iword {
   cat::iword result;
   asm volatile("syscall"
                : "=a"(result)
                : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3)
                // `memory` is clobbered because the kernel may write through
                // any pointer the caller passed.
                // `cc` is clobbered because the kernel may set the carry flag.
                : "memory", "cc", "rcx", "r11");
   return result;
}
