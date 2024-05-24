#include <cat/linux>

[[gnu::no_sanitize_address, gnu::no_sanitize_undefined]]
auto
nix::process::create_impl(cat::uintptr<void> stack, cat::idx initial_stack_size,
                          void* p_function,
                          void* p_args_struct) -> scaredy_nix<bool> {
    m_stack_size = initial_stack_size;
    m_p_stack = static_cast<void*>(stack);

    // We need the top because memory will be pushed to it downwards on
    // x86-64.
    cat::uintptr<void> stack_top =
        cat::uintptr<void>(m_p_stack) + m_stack_size - 8;

    // Place a pointer to function arguments on the new stack:
    // TODO: Generate a `cat::tuple` for this.
    __builtin_memcpy(static_cast<void*>(stack_top + 8), &p_args_struct, 8);

    // Place a pointer to function on the new stack:
    // `8` is the size of a pointer, such as `p_function`.
    __builtin_memcpy(static_cast<void*>(stack_top), &p_function, 8);

    scaredy_nix<nix::process_id> result = nix::sys_clone(
        flags, static_cast<void*>(stack_top - 8),
        // TODO: This nullptr is where the child thread's ID can be stored.
        &m_id, nullptr, nullptr);

    if (result.value().value == 0) {
        // The result is 0 iff this is the child thread.
        return false;
    }

    // Stay on parent if this is either a PID or an error.
    if (!result.has_value()) {
        return result.error();
    }
    return true;
}

[[nodiscard]]
auto
nix::process::wait() const -> scaredy_nix<process_id> {
    scaredy_nix<process_id> result =
        sys_waitid(wait_id::process_id, m_id,
                   wait_options_flags::exited | wait_options_flags::clone |
                       wait_options_flags::no_wait);
    return result;
}
