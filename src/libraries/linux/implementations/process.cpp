#include <cat/linux>

auto
nix::process::spawn_impl(cat::uintptr<void> stack, cat::idx initial_stack_size,
                         cat::idx thread_local_buffer_size, void* p_function,
                         void* p_args_struct) -> scaredy_nix<void> {
   m_stack_size = initial_stack_size;
   m_p_stack_bottom = stack.get();

   // We need the top because memory will be pushed to it downwards on
   // x86-64.
   cat::uintptr<void> stack_top =
      stack + m_stack_size + thread_local_buffer_size;

   cat::uintptr<void> tls_buffer = stack_top;
   stack_top -= thread_local_buffer_size;

   // 32 byte alignment is required for AVX2 support.
   // stack_top = cat::align_down(stack_top - 16, 32u);

   // Place a pointer to function arguments on the new stack:
   stack_top -= 8;
   __builtin_memcpy(stack_top.get(), &p_args_struct, 8);

   // Place a pointer to function on the new stack:
   // 8 is the size of a pointer, such as `p_function`.
   stack_top -= 8;
   __builtin_memcpy(stack_top.get(), &p_function, 8);

   // This syscall is made manually here because it's important to be careful
   // with the stack and registers and not introduce a new stack frame.
   nix::scaredy_nix<nix::process_id> result;
   asm goto volatile(
      R"(mov %[tls], %%r8
         xor %%r10, %%r10
         syscall
         # Branch if this is the parent process.
         test %%rax, %%rax
         jnz %l[parent_thread]

         # Call the function pointer if this is the child process.
         pop %%rax
         pop %%rdi # Pass the arguments pointer to the first function parameter.
         call *%%rax)"
      : /* There are no outputs. */
      : "a"(56), "D"(m_flags), "S"(stack_top),
        "d"(&(m_id)), [tls] "r"(tls_buffer)
      // TODO: These clobbers can't all be necessary. If they are, explain why.
      : "rcx", "r8", "r9", "r10", "r11", "memory", "cc"
      : parent_thread);

   // Exit the child thread after its entry function returns.
   // TODO: Support propagating an exit code, like `main()` does.
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
   // Spin to ensure that `m_id` is initialized before waiting on it.
   while (m_id.value == 0) {
   }

   scaredy_nix<process_id> result =
      sys_waitid(wait_id::process_id, m_id,
                 wait_options_flags::exited | wait_options_flags::clone
                    | wait_options_flags::no_wait);
   return result;
}
