	.intel_syntax noprefix
	.text
	.globl _start, syscall5

_start:
	xor %rbp, %rbp // zero out the stack frame.
	pop %rdi // %rdi = argc
	mov %rsi, %rsp // Rest of the stack is an array of char.
	// zero the last 4 bits of %rsp, aligning it to 16 bytes.
	and %rsp,-16
	call main
	mov %rdi, %rax // Set exit argument to the return of main().
	mov %rax, 60 // _Exit
	syscall
	ret

syscall5:
	mov %rax, %rdi
	mov %rdi, %rsi
	mov %rsi, %rdx
	mov %rdx, %rcx
	mov %r10, %r8
	mov %r8, %r9
	syscall
	ret
