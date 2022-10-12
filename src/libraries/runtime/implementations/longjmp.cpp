#include <cat/runtime>

// TODO: Should `__builtin_setjmp_reciever()` be used here?
[[noreturn, gnu::naked]] void cat::longjmp(JmpBuffer& /* jump_buffer */,
                                           int8 /* return_value */) {
    asm volatile(R"(
        # Increment %rax if `return_value` is non-zero.
        # This code is equivalent to:
        #     %rax = return_value + (!return_value);
        xor %rax,     %rax
        cmp $1,       %rsi
        adc %rsi,     %rax

        # Load registers from `jump_buffer`.
        mov (%rdi),   %rbx
        mov 8(%rdi),  %rbp
        mov 16(%rdi), %r12
        mov 24(%rdi), %r13
        mov 32(%rdi), %r14
        mov 40(%rdi), %r15

        # Return to the stack pointer saved in `jump_buffer`.
        mov 48(%rdi), %rsp
        jmp *56(%rdi))");
}
