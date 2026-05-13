#include <cat/linux>

[[gnu::no_sanitize_address]]
auto
nix::syscall0_volatile(cat::iword call) -> cat::iword {
   cat::iword result;
   asm volatile("syscall"
                : "=a"(result)
                : "a"(call)
                // `memory` is clobbered because the kernel may write through
                // any pointer the caller passed.
                // `cc` is clobbered because the kernel may set the carry flag.
                : "memory", "cc", "rcx", "r11");
   return result;
}
