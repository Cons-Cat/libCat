#include <cat/linux>

[[gnu::no_sanitize_address]]
auto
nix::syscall6(cat::iword call, cat::no_type arg1, cat::no_type arg2,
              cat::no_type arg3, cat::no_type const arg4,
              cat::no_type const arg5, cat::no_type const arg6) -> cat::iword {
   // `arg4`, `arg5`, and `arg6` must be `const-qualified` for GCC to compile.
   register cat::no_type r10 asm("r10") = arg4;
   register cat::no_type r8 asm("r8") = arg5;
   register cat::no_type r9 asm("r9") = arg6;

   cat::iword result;
   asm volatile("syscall"
                : "=a"(result)
                : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8),
                  "r"(r9), [a6] "re"(arg6)
                : "rcx", "r11", "memory", "cc");
   return result;
}
