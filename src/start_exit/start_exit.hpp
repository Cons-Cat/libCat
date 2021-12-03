#pragma once

/* These are the entry and exit points of a program compiled with LibCat.
 * extern "C" puts _start and _exit symbols into the binary, so they do
 * not need to be declared explicitly here. */

void _exit(int exit_code) {
    // clang-format off
	asm(R"(
        syscall
        ret)"
		: // No outputs.
		:"D" (exit_code), "a"(60)
		: "memory");
    // clang-format on
    __builtin_unreachable();
}

void _start() {
    // clang-format off
    asm(R"(
        xor %rbp, %rbp # Zero-out the stack pointer.
        mov %esp, %edi # Setup 32-bit argc
        lea 8(%rsp), %rsi # Setup argv
        call main
        mov %eax, %edi # Forward main()'s 32-bit return to _exit.
        call _exit)");
    // clang-format on
}
