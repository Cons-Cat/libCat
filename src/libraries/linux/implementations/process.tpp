// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator_parameters>
#include <cat/linux>
#include <cat/runtime>

namespace nix {

namespace detail {

template <typename Arguments>
// Clone child runs on its own `%fs` TLS base that the ASan runtime does not
// know about.
[[noreturn, gnu::noinline, gnu::no_sanitize_address]]
void
clone_continuation(Arguments* _Nonnull p_arguments) {
   auto&& [callback, ... arguments] = *p_arguments;
   $fwd(callback)($fwd(arguments)...);
   p_arguments->~Arguments();
#if !defined(CAT_THREAD_LOCAL_SIZE) || (CAT_THREAD_LOCAL_SIZE) != 0
   cat::__cxa_thread_finalize();
#endif
   // Exit with success.
   nix::sys_exit(0);
}

}  // namespace detail

inline namespace manual {

// `process` handles an asynchronous task multitasked by the Linux kernel.
struct process {
   // `clone_flags::csignal` must carry `signal::child_stopped`, otherwise
   // `clone` leaves `exit_signal` at 0. `clone_flags::set_tls` is merged in
   // `prepare_spawn` when the executable has a `PT_TLS` image so each child
   // receives an initialized `%fs` base.
   static constexpr clone_flags default_flags =
      clone_flags::virtual_memory
      | clone_flags::file_system
      | clone_flags::file_descriptor_table
      | clone_flags::io
      | clone_flags::parent_set_tid
      | clone_flags::child_clear_tid
      | static_cast<clone_flags>(static_cast<unsigned int>(
         static_cast<unsigned char>(signal::child_stopped)
      ));

   process() = default;

   process(process const&) = delete;

   process(process&& other) {
      *this = cat::move(other);
   }

   auto
   operator=(process&& other) -> process& {
      if (this == __builtin_addressof(other)) {
         return *this;
      }

      m_id.relaxed() = other.m_id.relaxed().load();
      m_clone_child_clear_tid_for_kernel.m_value.relaxed() =
         other.m_clone_child_clear_tid_for_kernel.m_value.relaxed().load();
      m_p_stack_bottom = other.m_p_stack_bottom;
      m_stack_size = other.m_stack_size;
      m_allocation_bytes = other.m_allocation_bytes;
      m_flags = other.m_flags;

      other.m_id.relaxed() = 0u;
      other.m_clone_child_clear_tid_for_kernel.m_value.relaxed() = 0;
      other.m_p_stack_bottom = nullptr;
      other.m_stack_size = 0;
      other.m_allocation_bytes = 0;
      other.m_flags = default_flags;
      return *this;
   }

   auto
   operator=(process const&) -> process& =
      delete ("`nix::process` is non-copyable. Try to move instead!");

   [[nodiscard]]
   constexpr auto
   get_clone_flags() const -> clone_flags {
      return m_flags;
   }

   constexpr void
   set_clone_flags(clone_flags flags) {
      m_flags = flags;
   }

   constexpr void
   add_clone_flag(clone_flags extra) {
      m_flags = static_cast<clone_flags>(
         cat::to_underlying(m_flags) | cat::to_underlying(extra)
      );
   }

   template <typename... Args, cat::is_invocable<Args...> Callback>
   auto
   spawn(
      cat::is_allocator auto& allocator, cat::idx stack_size,
      Callback&& callback, Args&&... arguments
   ) -> scaredy_nix<void>;

   [[nodiscard]]
   auto
   wait() const -> scaredy_nix<process_id>;

   [[nodiscard]]
   constexpr auto
   is_empty() const -> bool {
      return m_p_stack_bottom == nullptr;
   }

   [[nodiscard]]
   constexpr auto
   id() const -> process_id {
      return m_id.relaxed().load();
   }

   // Deallocate this `process`'s stack from `allocator`. Call after `wait()`.
   template <cat::is_allocator Allocator>
   void
   free(cat::allocator_ref<Allocator> allocator);

   // Deallocate this `process`'s stack from `allocator`. Call after `wait()`.
   [[clang::reinitializes, gnu::always_inline, gnu::nodebug]]
   void
   free(cat::dyn_allocator allocator) {
      free<cat::dyn_allocator>(allocator);
   }

   // Deallocate this `process`'s stack from `allocator`. Call after `wait()`.
   template <cat::is_allocator Allocator>
   void
   cfree(cat::allocator_ref<Allocator> allocator);

   // Deallocate this `process`'s stack from `allocator`. Call after `wait()`.
   [[clang::reinitializes, gnu::always_inline, gnu::nodebug]]
   void
   cfree(cat::dyn_allocator allocator) {
      cfree<cat::dyn_allocator>(allocator);
   }

 private:
   auto
   prepare_spawn(
      cat::uintptr<void> stack, cat::idx stack_size,
      cat::uintptr<void>& stack_top, clone_flags& active_clone_flags,
      void const* _Nullable& p_clear_tid_for_clone,
      void* _Nullable& p_tls_thread_pointer
   ) -> scaredy_nix<void>;

   // The kernel publishes the child tid here for
   // `clone_flags::parent_set_tid`, concurrently with the parent's `wait()`.
   cat::atomic<cat::uint4> m_id{};
   // `clone_flags::child_clear_tid` must not use `m_id` as the clear-tid word.
   // The kernel stores the child tid here, clears it to zero at thread exit,
   // and wakes waiters with `futex_command::wake` and `futex_options::none`.
   // `wait()` uses `futex_command::wait` with `futex_options::none` on this
   // word when `clone_flags::thread` and `clone_flags::child_set_tid` are set.
   alignas(8) futex_word m_clone_child_clear_tid_for_kernel{};
   void* _Nullable m_p_stack_bottom = nullptr;
   cat::idx m_stack_size = 0;
   cat::idx m_allocation_bytes = 0;
   clone_flags m_flags = default_flags;
};

}  // namespace manual

template <typename... Args, cat::is_invocable<Args...> Callback>
auto
manual::process::spawn(
   cat::is_allocator auto& allocator, cat::idx const stack_size,
   Callback&& callback, Args&&... arguments
) -> scaredy_nix<void> {
   using arguments_type =
      decltype(cat::tuple{$fwd(callback), $fwd(arguments)...});

   cat::idx const thread_local_slab_bytes =
      detail::clone_thread_local_buffer_min_bytes();
   constexpr cat::idx arguments_padding =
      alignof(arguments_type) - 1u + sizeof(arguments_type);
   cat::span<cat::byte> memory = $prop_as(
      allocator.template align_alloc_multi<cat::byte>(
         cat::max(16u, alignof(arguments_type)),
         stack_size + thread_local_slab_bytes + arguments_padding
      ),
      linux_error::inval
   );

   // TODO: Support call operator for functors.
   // cat::tuple<Args...> args{$fwd(arguments)...};

   cat::byte* p_stack_bottom = memory.data();

   cat::byte* const p_arguments_storage = cat::align_up(
      p_stack_bottom + stack_size + thread_local_slab_bytes,
      alignof(arguments_type)
   );
   auto* const p_arguments = new (p_arguments_storage)
      arguments_type{$fwd(callback), $fwd(arguments)...};

   cat::uintptr<void> stack_top;
   clone_flags active_clone_flags;
   void const* _Nullable p_clear_tid_for_clone;
   void* _Nullable p_tls_thread_pointer;
   scaredy_nix<void> result = this->prepare_spawn(
      p_stack_bottom, stack_size, stack_top, active_clone_flags,
      p_clear_tid_for_clone, p_tls_thread_pointer
   );

   if (result.has_value()) {
      cat::iword syscall_number = 56;
      asm goto volatile(
         R"(mov %[cleartid], %%r10
            mov %[tls], %%r8
            mov %[arguments], %%r12
            syscall
            test %%rax, %%rax
            jz 1f
            mov %%eax, %[parent_eax]
            jmp %l[clone_parent]
         1:
            mov %%r12, %%rdi
            call %P[continuation]
            ud2)"
         : [parent_eax] "=m"(result), [syscall_number] "+a"(syscall_number),
           [active_clone_flags] "+D"(active_clone_flags)
         : "S"(stack_top), "d"(&(m_id)), [tls] "r"(p_tls_thread_pointer),
           [cleartid] "r"(p_clear_tid_for_clone), [arguments] "r"(p_arguments),
           [continuation] "i"(&detail::clone_continuation<arguments_type>)
         : "r8", "r10", "r12", "rcx", "r11", "cc", "memory"
         : clone_parent
      );
      __builtin_unreachable();

clone_parent:
   } else {
      p_arguments->~arguments_type();
   }

   if (result.is_empty()) {
      allocator.free_multi(memory);
      this->m_p_stack_bottom = nullptr;
      this->m_stack_size = 0;
      this->m_allocation_bytes = 0;
      return result;
   }

   this->m_allocation_bytes = memory.size();
   return result;
}

template <cat::is_allocator Allocator>
void
manual::process::free(cat::allocator_ref<Allocator> allocator) {
   if (m_p_stack_bottom != nullptr) {
      allocator.free_multi(
         cat::span<cat::byte>(
            static_cast<cat::byte*>(m_p_stack_bottom), m_allocation_bytes
         )
      );
      m_p_stack_bottom = nullptr;
      m_stack_size = 0;
      m_allocation_bytes = 0;
   }
}

template <cat::is_allocator Allocator>
void
manual::process::cfree(cat::allocator_ref<Allocator> allocator) {
   if (m_p_stack_bottom != nullptr) {
      allocator.cfree_multi(
         cat::span<cat::byte>(
            static_cast<cat::byte*>(m_p_stack_bottom), m_allocation_bytes
         )
      );
      m_p_stack_bottom = nullptr;
      m_stack_size = 0;
      m_allocation_bytes = 0;
   }
}

}  // namespace nix
