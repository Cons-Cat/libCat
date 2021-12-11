	.intel_syntax noprefix
	.text
	.global syscall3

syscall3:
	mov %rax, %rdi
	mov %rdi, %rsi
	mov %rsi, %rdx
	mov %rdx, %rcx
	syscall
	ret
