	.intel_syntax noprefix
	.text
	.global syscall0
		
syscall0:
	mov %rax, %rdi
	syscall
	ret

