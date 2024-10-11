#include <cat/runtime>

// TODO: Should `__builtin_setjmp_setup()`/`__builtin_setjmp()` be used here?
[[gnu::naked, gnu::returns_twice]]
auto
cat::setjmp(jmp_buffer& /* jump_point */) -> int4 {
   asm volatile(R"(
        # Put the pointer to `jump_point` in %rdi.
        mov %rbx, (%rdi)

       # Store various registers into `jump_point`.
        mov %rbp,    8(%rdi)
        mov %r12,   16(%rdi)
        mov %r13,   24(%rdi)
        mov %r14,   32(%rdi)
        mov %r15,   40(%rdi)

        # Store the stack pointer into `jump_point`.
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
