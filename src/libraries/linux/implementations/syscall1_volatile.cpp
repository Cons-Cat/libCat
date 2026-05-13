#include <cat/linux>

[[gnu::no_sanitize_address]]
auto
nix::syscall1_volatile(cat::iword call, cat::no_type arg) -> cat::iword {
   cat::iword result;
   asm volatile("syscall"
                : "=a"(result)
                : "a"(call), "D"(arg)
                // `memory` is clobbered because the kernel may write through
                // any pointer the caller passed.
                // `cc` is clobbered because the kernel may set the carry flag.
                : "memory", "cc", "rcx", "r11");
   return result;
}
