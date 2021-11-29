	.intel_syntax noprefix
	.text
	.global _start, syscall5

_start:	
	xor %rbp, %rbp // zero out the stack frame.
	pop %rdi // %rdi = argc
	mov %rsi, %rsp // The rest of the stack is an array of chars.
	// Zero the last 4 bits of %rsp, aligning it to 16 bytes.
	and %rsp, -16
	call main
	
	mov %rdi, %rax // Set arg1 to the return of main().
	mov %rax, 60 // _exit
	syscall
	ret

syscall5:
	mov %rax, %rdi // Assign syscall.
	mov %rdi, %rsi // arg 1
	mov %rsi, %rdx // arg 2
	mov %rdx, %rcx // arg 3
	mov %r10, %r8  // arg 4
	mov %r8, %r9   // arg 5
	syscall
	ret
