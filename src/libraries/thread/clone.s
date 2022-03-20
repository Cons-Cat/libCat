.text
.global clone_asm
.type   clone_asm,@function
clone_asm:
    mov $56,%rax

    mov %rcx,-8(%rsi)
    mov %rdi,%rsi
	
    mov %rdx,%rdi
    mov %rdi,%r11
    mov %r8,%rdx
    mov %r9,%r8
    mov 8(%rsp),%r10
    syscall
    
	popq    %rax
	popq    %rdi
	call    *%rax
    ret
