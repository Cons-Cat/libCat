// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>

[[gnu::no_sanitize_undefined, gnu::noinline]]
auto
nix::process::create(
    cat::is_allocator auto& allocator, cat::idx const initial_stack_size,
    auto const& function, void* p_args_struct,
    // TODO: These flags should largely be encoded into the type.
    clone_flags const flags) -> scaredy_nix<void> {
    // Allocate a stack for this thread, and get an address to the top of
    // it.
    // TODO: This stack memory should not be owned by the `process`, to
    // enable simpler memory management patterns.
    // TODO: A `prop_as` macro or something can simplify this.
    auto maybe_memory =
        allocator.template alloc_multi<cat::byte>(initial_stack_size);
    if (!maybe_memory.has_value()) {
        return nix::linux_error::inval;
    }

    // TODO: Support call operator for functors.
    return this->create_impl(maybe_memory.value().data(), initial_stack_size,
                             reinterpret_cast<void*>(&function), p_args_struct,
                             flags);
}
