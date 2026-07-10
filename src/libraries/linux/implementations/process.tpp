// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator_parameters>

namespace nix {

inline namespace manual {

// `process` handles an asynchronous task multitasked by the Linux kernel.
struct process {
   // `clone_flags::csignal` must carry `signal::child_stopped`, otherwise
   // `clone` leaves `exit_signal` at 0. `clone_flags::set_tls` is merged in
   // `spawn_impl` when the executable has a `PT_TLS` image so each clone child
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
   // TODO: Add a move constructor and move assignment operator.
   process(process const&) = delete;

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
   has_stack() const -> bool {
      return m_p_stack_bottom != nullptr;
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

 private:
   auto
   spawn_impl(
      cat::uintptr<void> stack, cat::idx stack_size, void* _Nonnull p_function,
      void* _Nullable p_args_struct
   ) -> scaredy_nix<void>;

   process_id m_id{0};
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
   // Allocate a stack for this process.
   // TODO: This should union allocator and linux errors.
   // TODO: Use size feedback.
   cat::idx const thread_local_slab_bytes =
      detail::clone_thread_local_buffer_min_bytes();
   cat::span<cat::byte> memory = $prop_as(
      allocator.template align_alloc_multi<cat::byte>(
         16u, stack_size + thread_local_slab_bytes
      ),
      linux_error::inval
   );

   // TODO: Support call operator for functors.
   // cat::tuple<Args...> args{$fwd(arguments)...};

   cat::byte* p_stack_bottom = memory.data();

   scaredy_nix<void> result;
   if constexpr (
      sizeof...(Args) == 0
      && (__is_pointer(Callback) || __is_function(__remove_reference_t(Callback)))
   ) {
      // If there are no arguments, and `callback` is a pointer, it can be
      // called almost directly.
      result = this->spawn_impl(
         p_stack_bottom, stack_size, reinterpret_cast<void*>(callback), nullptr
      );
   } else {
      // If there are arguments, `callback` must be wrapped in a lambda that has
      // tuple storage.
      static cat::tuple tuple_args{$fwd(callback), $fwd(arguments)...};

      // Unary + converts this lambda to function pointer.
      static auto* _Nonnull p_entry =
         +[] [[gnu::no_sanitize_address, gnu::no_sanitize("undefined")]]
          (cat::tuple<Callback, Args...>* p_arguments) {
             auto&& [fn, ... pack_args] = *p_arguments;
             $fwd(fn)($fwd(pack_args)...);
          };

      result = this->spawn_impl(
         p_stack_bottom, stack_size, reinterpret_cast<void*>(p_entry),
         reinterpret_cast<void*>(&tuple_args)
      );
   }

   if (!result.has_value()) {
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

}  // namespace nix
