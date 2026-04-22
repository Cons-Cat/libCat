#include <cat/linux>

namespace {

[[nodiscard]]
auto
wait_clone_child_through_waitid(nix::process_id child_id)
   -> nix::scaredy_nix<nix::process_id> {
   for (;;) {
      // `process::default_flags` carries `signal::child_stopped` in
      // `clone_flags::csignal`, so `sys_waitid` only needs
      // `wait_options_flags::exited`.
      nix::scaredy_nix<nix::process_id> const result = nix::sys_waitid(
         nix::wait_id::process_id, child_id, nix::wait_options_flags::exited);
      if (result.has_value()) {
         return result;
      }
      if (result.error() == nix::linux_error::intr) {
         continue;
      }
      return result;
   }
}

// `cat::thread` enables `clone_flags::child_set_tid` with
// `clone_flags::child_clear_tid`. Thread exit clears this word and the kernel
// issues `futex_command::wake` with `futex_options::none`, so join must use
// `futex_command::wait` with `futex_options::none`.
[[nodiscard]]
auto
wait_clone_thread_through_cleartid_futex(nix::process_id child_id,
                                         nix::futex_word* p_clear_tid)
   -> nix::scaredy_nix<nix::process_id> {
   nix::process_id const thread_group_id = nix::sys_getpid();
   cat::uword spins = 0u;
   for (;;) {
      cat::uint4 const published =
         p_clear_tid->m_value.load(cat::memory_order::acquire);
      if (published != 0u) {
         for (;;) {
            cat::uint4 const word =
               p_clear_tid->m_value.load(cat::memory_order::acquire);
            if (word == 0u) {
               return nix::scaredy_nix<nix::process_id>(child_id);
            }
            nix::scaredy_nix<cat::idx> const slept =
               nix::sys_futex(p_clear_tid,
                              nix::make_futex_op(nix::futex_command::wait,
                                                 nix::futex_options::none),
                              word, nullptr, nullptr, cat::uint4{});
            if (slept.has_value()) {
               continue;
            }
            if (slept.error() == nix::linux_error::again
                || slept.error() == nix::linux_error::intr) {
               continue;
            }
            return nix::scaredy_nix<nix::process_id>(slept.error());
         }
      }
      if ((spins & 127u) == 0u) {
         nix::scaredy_nix<void> const poke = nix::sys_tgkill(
            thread_group_id, child_id, static_cast<nix::signal>(0));
         if (!poke.has_value()) {
            if (poke.error() == nix::linux_error::srch) {
               return nix::scaredy_nix<nix::process_id>(child_id);
            }
            return nix::scaredy_nix<nix::process_id>(poke.error());
         }
      } else {
         __builtin_ia32_pause();
      }
      ++spins;
      if ((spins & 1'023u) == 0u) {
         auto _ = nix::sys_sched_yield();
      }
   }
}

}  // namespace

auto
nix::process::spawn_impl(cat::uintptr<void> stack, cat::idx initial_stack_size,
                         void* p_function, void* p_args_struct)
   -> scaredy_nix<void> {
   m_stack_size = initial_stack_size;
   m_p_stack_bottom = stack.get();

   cat::idx const thread_local_slab_bytes =
      nix::detail::clone_thread_local_slab_min_bytes();

   cat::uintptr<void> const stack_and_thread_local_buffer_end_exclusive =
      stack + m_stack_size + thread_local_slab_bytes;

   cat::idx const tls_memory_size = nix::detail::executable_tls_memory_bytes();

   cat::uintptr<void> tls_thread_pointer =
      stack_and_thread_local_buffer_end_exclusive;

   if (tls_memory_size > 0u) {
      cat::uword align_want = nix::detail::executable_tls_alignment_bytes();
      if (align_want < 32u) {
         align_want = 32u;
      }
      // The buffer end (exclusive) can be a multiple of the requested
      // alignment. Rounding that end down with `align_down` can return the same
      // address. The TCB self slot at that address is then one past the span.
      // Take the last in-bounds address and round that down.
      cat::uintptr<void> const stack_region_last_inclusive =
         stack_and_thread_local_buffer_end_exclusive - cat::uword{1u};
      tls_thread_pointer =
         cat::align_down(stack_region_last_inclusive, align_want);
      cat::uintptr<void> const tls_slab_low = stack + m_stack_size;
      if (tls_thread_pointer - tls_memory_size < tls_slab_low) {
         return nix::linux_error::inval;
      }
      nix::detail::install_executable_tls_image_at_thread_pointer(
         tls_thread_pointer);

      // Local-exec TLS lowering loads the thread pointer from `%fs:0`. The
      // copied executable TLS image only covers `.tdata/.tbss` strictly below
      // `tls_tp`; the word at `tls_tp` is the TCB self slot and must equal
      // `tls_tp` after `clone_flags::set_tls` installs that base in `%fs`.
      *reinterpret_cast<void**>(tls_thread_pointer.get()) =
         tls_thread_pointer.get();
   }

   cat::uintptr<void> stack_top = tls_thread_pointer;
   stack_top -= thread_local_slab_bytes;

   // 32 byte alignment is required for AVX2 support.
   stack_top = cat::align_down(stack_top, 32u);

   // Place a pointer to function arguments on the new stack:
   stack_top -= 8;
   __builtin_memcpy(stack_top.get(), &p_args_struct, 8);  // NOLINT

   // Place a pointer to function on the new stack:
   // 8 is the size of a pointer, such as `p_function`.
   stack_top -= 8;
   __builtin_memcpy(stack_top.get(), &p_function, 8);  // NOLINT

   // This syscall is made manually here because it's important to be careful
   // with the stack and registers and not introduce a new stack frame.
   // Parent and child share `clone_flags::virtual_memory`, so they must not
   // both spill `%rax` through one C variable. That would race. Only the parent
   // executes the `mov` into `clone_result` (`asm goto`); the child
   // jumps away before that store.
   nix::clone_flags active_clone_flags = m_flags;
   if (tls_memory_size > 0u) {
      active_clone_flags |= nix::clone_flags::set_tls;
   }

   void const* p_clear_tid_for_clone = nullptr;
   if (cat::to_underlying(m_flags & nix::clone_flags::child_set_tid) != 0u) {
      p_clear_tid_for_clone =
         static_cast<void*>(&m_clone_child_clear_tid_for_kernel.m_value);
   }

   nix::scaredy_nix<void> clone_result;
   asm goto volatile(
      R"(mov %[cleartid], %%r10
         mov %[tls], %%r8
         syscall
         test %%rax, %%rax
         jz %l[clone_child]
         mov %%rax, %[parent_rax]
         jmp %l[clone_parent])"
      : [parent_rax] "=m"(clone_result)
      : "a"(56), "D"(active_clone_flags), "S"(stack_top),
        "d"(&(m_id)), [tls] "r"(tls_thread_pointer.get()),
        [cleartid] "r"(p_clear_tid_for_clone)
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
   return clone_result;
}

[[nodiscard]]
auto
nix::process::wait() const -> scaredy_nix<process_id> {
   // Spin until the kernel publishes the child tid for
   // `clone_flags::parent_set_tid`. Use an acquire load so this synchronizes
   // with that store; `pause` hints the spin loop on x86.
   // TODO: Implement a high level spin-lock.
   while (__atomic_load_n(&m_id.value.raw, cat::memory_order::acquire) == 0) {
      __builtin_ia32_pause();
   }

   nix::clone_flags const flags = m_flags;
   nix::clone_flags const thread_cleartid_join =
      nix::clone_flags::thread | nix::clone_flags::child_set_tid;
   if ((flags & thread_cleartid_join) != thread_cleartid_join) {
      return wait_clone_child_through_waitid(m_id);
   }
   return wait_clone_thread_through_cleartid_futex(
      m_id, const_cast<nix::futex_word*>(&m_clone_child_clear_tid_for_kernel));
}
