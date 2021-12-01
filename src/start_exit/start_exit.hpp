#pragma once

// NOLINTNEXTLINE
__attribute__((optimize(0))) void _exit(i32 const& exit_code) {
    // clang-format off
	asm(R"(syscall
           ret)"
		   : // No outputs.
		   :"D" (exit_code), "a"(60)
		   : "memory");
    // clang-format on
    __builtin_unreachable();
}

// NOLINTNEXTLINE
__attribute__((optimize(0))) void _start() {
    // clang-format off
    asm(R"(.global _start
           _start:
           xor %rbp, %rbp # Zero-out the stack pointer.
           mov %rsp, %rdi # Setup argc
           lea 8(%rsp), %rsi # Setup argv
           call main
           mov %rax, %rdi # Forward main()'s return to _exit.
           call _exit)");
    // clang-format on
}
