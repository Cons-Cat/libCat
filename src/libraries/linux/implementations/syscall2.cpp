#include <cat/linux>

[[gnu::no_sanitize_address]]
auto
nix::syscall2(cat::iword call, cat::no_type arg1, cat::no_type arg2)
   -> cat::iword {
   cat::iword result;
   asm volatile("syscall"
                : "=a"(result)
                : "a"(call), "D"(arg1), "S"(arg2)
                // Clobbering all of these is necessary to prevent a segfault:
                : "memory", "cc", "rcx", "r11");
   return result;
}
