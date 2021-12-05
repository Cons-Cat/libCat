	.intel_syntax noprefix
	.text
	.global syscall2

syscall2:
	mov %rax, %rdi
	mov %rdi, %rsi
	mov %rsi, %rdx
	syscall
	ret
