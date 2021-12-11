	.intel_syntax noprefix
	.text
	.global syscall5

syscall5:
	mov %rax, %rdi
	mov %rdi, %rsi
	mov %rsi, %rdx
	mov %rdx, %rcx
	mov %r10, %r8
	mov %r8, %r9
	syscall
	ret
