#include <cat/linux>

[[gnu::no_sanitize_address, gnu::no_sanitize_undefined]]
auto
nix::process::create_impl(cat::uintptr<void> stack, cat::idx initial_stack_size,
                          void* p_function, void* p_args_struct)
   -> scaredy_nix<void> {
   m_stack_size = initial_stack_size;
   m_p_stack = static_cast<void*>(stack);

   // We need the top because memory will be pushed to it downwards on
   // x86-64.
   cat::uintptr<void> stack_top = cat::uintptr<void>(m_p_stack) + m_stack_size;

   // Place a pointer to function arguments on the new stack:
   stack_top -= 8;
   __builtin_memcpy(static_cast<void*>(stack_top),
                    static_cast<void*>(&p_args_struct), 8);

   // Place a pointer to function on the new stack:
   // 8 is the size of a pointer, such as `p_function`.
   stack_top -= 8;
   __builtin_memcpy(static_cast<void*>(stack_top),
                    static_cast<void*>(&p_function), 8);

   // This syscall is made manually here because it's important to be careful
   // with the stack and registers.
   // cat::no_type result;
   nix::scaredy_nix<nix::process_id> result;
   asm goto volatile(
      R"(syscall
         # Branch if this is the parent process.
         mov %%rax, %[result]
         test %%rax, %%rax
         jnz %l[parent_thread]

         # Call the function pointer if this is the child process.
         pop %%rax
         pop %%rdi
         call *%%rax)"
      : /* There are no outputs. */
      : "a"(56), "D"(flags), "S"(stack_top),
        "d"(&(m_id)), [p_invocable] "r"(&p_function),
        [p_args] "r"(p_args_struct), [result] "r"(result)
      : "rcx", "r8", "r9", "r10", "r11", "memory", "cc"
      : parent_thread);

   // Exit the child thread after its entry function returns.
   cat::exit();

parent_thread:
   if (!result.has_value()) {
      return result.error();
   }
   return cat::monostate;
}

[[nodiscard]]
auto
nix::process::wait() const -> scaredy_nix<process_id> {
   scaredy_nix<process_id> result =
      sys_waitid(wait_id::process_id, m_id,
                 wait_options_flags::exited | wait_options_flags::clone
                    | wait_options_flags::no_wait);
   return result;
}
