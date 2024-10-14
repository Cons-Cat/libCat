// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>

template <typename... Args>
[[gnu::no_sanitize_undefined, gnu::no_sanitize_address]]
auto
nix::process::create(cat::is_allocator auto& allocator,
                     cat::idx const initial_stack_size,
                     cat::is_invocable<Args...> auto&& function,
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

   scaredy_nix<void> on_parent = this->create_impl(
      p_stack_bottom, initial_stack_size, reinterpret_cast<void*>(function),
      reinterpret_cast<void*>(&args));

   // The child thread, if it exists, never reaches this point.
   if (!on_parent.has_value()) {
      return on_parent.error();
   }
   return cat::monostate;
}
