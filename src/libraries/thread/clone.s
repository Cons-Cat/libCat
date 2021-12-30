.text
.global clone_asm
.type   clone_asm,@function
clone_asm:
    xor %eax,%eax
    mov $56,%al
    mov %r11,%r9

    and $-16,%rsi
    mov %rcx,-8(%rsi)
    sub $16,%rsi
    mov %rdi,0(%rsi)
	
    mov %rdx,%rdi
    mov %rdi,%r11
    mov %r8,%rdx
    mov %r9,%r8
    mov 8(%rsp),%r10
    syscall
    
    xorl    %ebp, %ebp
	popq    %rax
	popq    %rdi
	call    *%rax
    ret
