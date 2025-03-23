// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>

template <typename... Args, cat::is_invocable<Args...> F>
auto
nix::process::spawn(cat::is_allocator auto& allocator,
                    cat::idx const initial_stack_size,
                    cat::idx const thread_local_buffer_size, F&& function,
                    Args&&... arguments) -> scaredy_nix<void> {
   // Allocate a stack for this thread.
   // TODO: This stack memory should not be owned by the `process`, to
   // enable simpler memory management patterns.
   // TODO: This should union allocator and linux errors.
   // TODO: Use size feedback.
   cat::span<cat::byte> memory =
      prop_as(allocator.template align_alloc_multi<cat::byte>(
                 16u, initial_stack_size + thread_local_buffer_size),
              nix::linux_error::inval);

   // TODO: Support call operator for functors.
   // cat::tuple<Args...> args{fwd(arguments)...};

   cat::byte* p_stack_bottom = memory.data();

   if constexpr (sizeof...(arguments) == 0 && __is_pointer(F)) {
      // If there are no arguments, and `function` is a pointer, it can be
      // called almost directly.
      return this->spawn_impl(p_stack_bottom, initial_stack_size,
                              thread_local_buffer_size,
                              reinterpret_cast<void*>(function), nullptr);
   } else {
      // If there are arguments, `function` must be wrapped in a lambda that has
      // tuple storage.
      cat::tuple tuple_args{fwd(function), fwd(arguments)...};

      // Unary `+` converts this lambda to function pointer.
      static auto* p_entry = +[](cat::tuple<F, Args...>* p_arguments) {
         auto&& [fn, ... pack_args] = *p_arguments;
         fwd(fn)(fwd(pack_args)...);
      };

      return this->spawn_impl(p_stack_bottom, initial_stack_size,
                              thread_local_buffer_size,
                              reinterpret_cast<void*>(p_entry),
                              reinterpret_cast<void*>(&tuple_args));
   }
}
