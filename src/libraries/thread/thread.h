// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <linux>

struct Thread {
    ProcessId process_id;

    /* `joinable` is `true` if this thread can currently be joined, otherwise it
     * is `false`. */
    bool joinable = false;

    void* p_stack;
    // TODO: This could be `isize` if it being non-negative is asserted.
    usize stack_size;

    Thread() = default;
    Thread(Thread const&) = delete;

    ~Thread() {
        if (joinable) {
            exit(0);
        }
    }

    // TODO: Make this `Result` hold `void` and store the PID in a member.
    // TODO: Use a `meta::allocator` concept here when it's working.
    // TODO: Add a method for allocating guard memory.
    // Add meta::invocable concept.
    auto create(auto& allocator, usize const initial_stack_size,
                auto const& function, void* p_arguments_struct) -> Result<> {
        u4 const flags = ::CLONE_VM | ::CLONE_FS | ::CLONE_FILES |
                         ::CLONE_SIGHAND | ::CLONE_SETTLS |
                         ::CLONE_PARENT_SETTID | ::CLONE_CHILD_CLEARTID;

        this->stack_size = initial_stack_size;
        /* Allocate a stack for this thread, and get an address to the top of
         * it. */
        // TODO: There has to be a way to streamline this:
        this->p_stack = allocator.malloc(stack_size).or_is(nullptr);
        if (this->p_stack == nullptr) {
            return Failure(1);
        }
        /* We need the top because memory will be pushed to it downwards on
         * x86-64. */
        void* p_stack_top =
            static_cast<char*>(this->p_stack) + this->stack_size;
        // The ProcessId is the first data stored in a thread.
        ProcessId* p_pid = static_cast<ProcessId*>(p_stack_top);

        // prctl(ARCH_SET_GS, p_tls).or_panic();
        // __builtin_ia32_wrfsbase64(); // Write to `%fs`.
        // __builtin_ia32_rdfsbase64(); // Read from `%fs`.
        // __builtin_ia32_wrgsbase64(); // Write to `%gs`.
        // __builtin_ia32_rdgsbase64(); // Read from `%gs`.

        // `clone()` will always return a value.
        clone_asm(&function, p_stack_top, flags, p_arguments_struct, p_pid,
                  p_stack_top, p_pid);
        this->process_id = *p_pid;
        this->joinable = true;
        return okay;
    }

    auto join() -> Result<Any> {
        this->joinable = false;
        /* Wait on this thread, without storing its status, or waiting with
         * special options. */
        return waitid(
            WaitIdType::P_PID, this->process_id,
            WaitOptions::WEXITED | WaitOptions::WCLONE | WaitOptions::WNOWAIT);
        // TODO: Free the stack memory.
    }
};
