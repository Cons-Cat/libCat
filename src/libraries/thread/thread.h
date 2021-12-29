// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <concepts.h>
#include <mmap.h>
#include <syscall.h>

#include "unistd.h"

struct Thread;

auto get_p_thread() -> Thread* {
    Thread* p_thread;
    asm("mov %%fs:0,%0" : "=r"(p_thread));
    return p_thread;
}

enum ThreadFlags : u4
{
    CSIGNAL = 0x000000ff,
    CLONE_NEWTIME = 0x00000080,
    CLONE_VM = 0x00000100,
    CLONE_FS = 0x00000200,
    CLONE_FILES = 0x00000400,
    CLONE_SIGHAND = 0x00000800,
    CLONE_PIDFD = 0x00001000,
    CLONE_PTRACE = 0x00002000,
    CLONE_VFORK = 0x00004000,
    CLONE_PARENT = 0x00008000,
    CLONE_THREAD = 0x00010000,
    CLONE_NEWNS = 0x00020000,
    CLONE_SYSVSEM = 0x00040000,
    CLONE_SETTLS = 0x00080000,
    CLONE_PARENT_SETTID = 0x00100000,
    CLONE_CHILD_CLEARTID = 0x00200000,
    CLONE_DETACHED = 0x00400000,
    CLONE_UNTRACED = 0x00800000,
    CLONE_CHILD_SETTID = 0x01000000,
    CLONE_NEWCGROUP = 0x02000000,
    CLONE_NEWUTS = 0x04000000,
    CLONE_NEWIPC = 0x08000000,
    CLONE_NEWUSER = 0x10000000,
    CLONE_NEWPID = 0x20000000,
    CLONE_NEWNET = 0x40000000,
    CLONE_IO = 0x80000000,
};

using UserId = u4;
using GroupId = u4;
using ProcessId = i4;

struct CloneArguments {
    u8 flags;
    FileDescriptor* process_id_file_descriptor;
    ProcessId* child_thread_id;
    ProcessId* parent_thread_id;
    i8 exit_code;
    void* p_stack;
    usize stack_size;
    // TODO: Deal with these later:
    void* p_tls;
    ProcessId* set_tid;
    usize set_tid_size;
    u8 cgroup;
};

extern "C" void clone_asm(isize (*function)(void*), void*, i4, auto*,
                          ProcessId*, void*, ProcessId*);

auto clone(isize (*function)(void*), void* p_stack, i4 flags,
           auto function_arguments, ProcessId* p_parent_thread_id = nullptr,
           void* p_tls = nullptr, ProcessId* p_child_thread_id = nullptr)
    -> ProcessId {
    // register ProcessId result asm("rax");
    // TODO: Replace this with inline asm.
    // This is just Musl code.
    clone_asm(function, p_stack, flags, function_arguments, p_parent_thread_id,
              p_tls, p_child_thread_id);
    // TODO: Failure handling.
    // return result;
    return 0;
}

// TODO: Replace the esoteric Linux names.
struct ResourceUsage {
    // struct timeval ru_utime;
    // struct timeval ru_stime;
    i8 ru_maxrss;
    i8 ru_ixrss;
    i8 ru_idrss;
    i8 ru_isrss;
    i8 ru_minflt;
    i8 ru_majflt;
    i8 ru_nswap;
    i8 ru_inblock;
    i8 ru_oublock;
    i8 ru_msgsnd;
    i8 ru_msgrcv;
    i8 ru_nsignals;
    i8 ru_nvcsw;
    i8 ru_nivcsw;
};

// TODO: Return a `ProcessId`
// TODO: Replace this with a more general `wait4` syscall.
auto waitpid(ProcessId child_process, i4 options) -> Result<> {
    // TODO: Use `p_termination_status` for failure-handling.
    i4* p_termination_status;
    return syscall4(61, child_process, p_termination_status, options, nullptr);
}

struct Thread {
    ProcessId process_id;

    /* `joinable` is `true` if this thread can currently be joined, otherwise it
     * is `false`. */
    bool joinable = false;

    void* p_stack;
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
                auto const& function, auto* p_arguments_struct)
        -> Result<ProcessId> {
        // Similar to `pthread_create`.
        u4 const flags = ::CLONE_VM | ::CLONE_FS | ::CLONE_FILES |
                         ::CLONE_SIGHAND | ::CLONE_THREAD | ::CLONE_SYSVSEM |
                         ::CLONE_SETTLS | ::CLONE_PARENT_SETTID |
                         ::CLONE_CHILD_CLEARTID | ::CLONE_DETACHED;
        // Thread* self = get_p_thread();

        // void* p_stack = mmap(0, stack_size, ::PROT_READ | ::PROT_WRITE,
        //                      ::MAP_PRIVATE | ::MAP_ANONYMOUS, -1, 0)
        //                     .or_panic();
        this->stack_size = initial_stack_size;
        this->p_stack = allocator.malloc(stack_size).or_it_is(nullptr);
        if (this->p_stack == nullptr) {
            return Failure(1);
        }

        // TODO: This does not compile:
        // `clone()` will always return a value.
        // isize exit_code =
        clone_asm(&function, this->p_stack, flags, p_arguments_struct, nullptr,
                  nullptr, nullptr);
        // joinable = (exit_code == 0);
        // return this->process_id;
    }

    auto join() -> Result<> {
        this->joinable = false;
        return waitpid(this->process_id, 0);
        // TODO: Free the stack memory.
    }

    auto detach() -> Result<> {
        // return okay;
    }
};
