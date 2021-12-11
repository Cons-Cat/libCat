	.intel_syntax noprefix
	.text
	.global syscall0, syscall1, syscall2, syscall3, syscall4, syscall5

syscall4:
	mov %rax, %rdi
	mov %rdi, %rsi
	mov %rsi, %rdx
	mov %rdx, %rcx
	mov %r10, %r8
	syscall
	ret
