	.intel_syntax noprefix
	.text
	.global syscall1

syscall1:
	mov %rax, %rdi
	mov %rdi, %rsi
	syscall
	ret
