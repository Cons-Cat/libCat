	.intel_syntax noprefix
	.text
	.global syscall5

syscall5:
	mov %rax, %rdi // Assign syscall.
	mov %rdi, %rsi // arg 1
	mov %rsi, %rdx // arg 2
	mov %rdx, %rcx // arg 3
	mov %r10, %r8  // arg 4
	mov %r8, %r9   // arg 5
	syscall
	ret
