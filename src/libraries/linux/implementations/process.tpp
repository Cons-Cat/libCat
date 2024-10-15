// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>

template <typename... Args, cat::is_invocable<Args...> F>
[[gnu::no_sanitize_undefined, gnu::no_sanitize_address]]
auto
nix::process::spawn(cat::is_allocator auto& allocator,
                    cat::idx const initial_stack_size, F&& function,
                    Args&&... arguments) -> scaredy_nix<void> {
   // Allocate a stack for this thread.
   // TODO: This stack memory should not be owned by the `process`, to
   // enable simpler memory management patterns.
   // TODO: This should union allocator and linux errors.
   // TODO: Use size feedback.
   auto maybe_memory =
      allocator.template align_alloc_multi<cat::byte>(16u, initial_stack_size);
   if (!maybe_memory.has_value()) {
      return nix::linux_error::inval;
   }

   // TODO: A `prop_as` macro or something can simplify this.
   // TODO: Support call operator for functors.
   cat::tuple<Args...> args{fwd(arguments)...};

   auto* p_stack_bottom = maybe_memory.value().data();
   scaredy_nix<void> on_parent;

   if constexpr (sizeof...(arguments) == 0) {
      // If there are no arguments, `function` can be called almost directly.
      on_parent = this->spawn_impl(p_stack_bottom, initial_stack_size,
                                   reinterpret_cast<void*>(function), nullptr);
   } else {
      // If there are arguments, `function` must be wrapped in a lambda that has
      // tuple storage.
      cat::tuple tuple_args{function, fwd(arguments)...};

      // Unary `+` converts this lambda to function pointer.
      static auto* p_entry = +[](cat::tuple<F, Args...>* p_arguments) {
         // TODO: When supported, try:
         // auto&& [fn, pack_args...] = *p_arguments;

         auto&& [fn] = *p_arguments;
         fwd(fn)();
      };

      on_parent = this->spawn_impl(p_stack_bottom, initial_stack_size,
                                   reinterpret_cast<void*>(p_entry),
                                   reinterpret_cast<void*>(&tuple_args));
   }

   // The child thread, if it exists, never reaches this point.
   if (!on_parent.has_value()) {
      return on_parent.error();
   }
   return cat::monostate;
}
