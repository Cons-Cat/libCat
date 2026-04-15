#include <cat/linux>

// The child thread exits with a false-positive from asan.
[[gnu::no_sanitize_address]]
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
   stack_top = cat::align_down(stack_top, 32u);

   // Place a pointer to function arguments on the new stack:
   stack_top -= 8;
   __builtin_memcpy(stack_top.get(), &p_args_struct, 8);

   // Place a pointer to function on the new stack:
   // 8 is the size of a pointer, such as `p_function`.
   stack_top -= 8;
   __builtin_memcpy(stack_top.get(), &p_function, 8);

   // This syscall is made manually here because it's important to be careful
   // with the stack and registers and not introduce a new stack frame.
   // Parent and child share `clone_flags::virtual_memory`, so they must not both spill `%rax`
   // through one C variable. That would race. Only the parent executes the `mov` into `parent_rax_after_clone` (`asm goto`); the child jumps away
   // before that store.
   cat::iword parent_rax_after_clone = 0;
   asm goto volatile(
      R"(mov %[tls], %%r8
         xor %%r10, %%r10
         syscall
         test %%rax, %%rax
         jz %l[clone_child]
         mov %%rax, %[parent_rax]
         jmp %l[clone_parent])"
      : [parent_rax] "=m"(parent_rax_after_clone)
      : "a"(56), "D"(m_flags), "S"(stack_top), "d"(&(m_id)), [tls] "r"(tls_buffer)
      : "rcx", "r11", "memory"
      : clone_child, clone_parent);

clone_child:
   asm volatile(
      R"(pop %%rax
         pop %%rdi
         call *%%rax
         mov $60, %%eax
         xor %%edi, %%edi
         syscall)"
      :
      :
      : "rax", "rdi", "rsp", "rcx", "r11", "cc", "memory");
   __builtin_unreachable();

clone_parent:
   if (parent_rax_after_clone < 0) {
      return static_cast<nix::linux_error>(parent_rax_after_clone);
   }
   return cat::monostate;
}

[[nodiscard]]
auto
nix::process::wait() const -> scaredy_nix<process_id> {
   // Spin until the kernel publishes the child tid via `CLONE_PARENT_SETTID`.
   // Use an acquire load so this synchronizes with that store; `pause` hints
   // the spin loop on x86.
   while (__atomic_load_n(const_cast<cat::iword::raw_type*>(&m_id.value.raw),
                          __ATOMIC_ACQUIRE) == 0) {
#if defined(__x86_64__) || defined(__i386__)
      __builtin_ia32_pause();
#endif
   }

   for (;;) {
      // `__WCLONE` pairs with clone children that omit `SIGCHLD`; libCat sets
      // `SIGCHLD` in `clone` flags, so only `WEXITED` is correct here.
      scaredy_nix<process_id> result =
         sys_waitid(wait_id::process_id, m_id, wait_options_flags::exited);
      if (result.has_value()) {
         return result;
      }
      if (result.error() == nix::linux_error::intr) {
         continue;
      }
      return result;
   }
}
