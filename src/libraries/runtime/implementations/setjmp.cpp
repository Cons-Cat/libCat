#include <cat/runtime>

[[gnu::naked]] auto cat::setjmp(JmpBuffer& /* jump_buffer */) -> int {
    asm volatile(R"(
        # Put the pointer to `jump_buffer` in %rdi.
        mov %rbx, (%rdi)

       # Store various registers into `jump_buffer`.
        mov %rbp,    8(%rdi)
        mov %r12,   16(%rdi)
        mov %r13,   24(%rdi)
        mov %r14,   32(%rdi)
        mov %r15,   40(%rdi)

        # Store the stack pointer into `jump_buffer`.
        lea 8(%rsp),   %rdx
        mov %rdx,   48(%rdi)
        mov (%rsp),    %rdx
        mov %rdx,   56(%rdi)

        # Return 0 here, to differentiate the `cat::setjmp()` call here from a
        # `cat::longjmp()` to the same point, which will set %rax to a
        # potentially different value.
        xor %rax, %rax
        ret)");
}
