// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <any.h>
#include <concepts.h>
#include <mmap.h>
#include <syscall.h>

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

extern "C" auto clone_asm(isize (*function)(void*), void*, i4, void*,
                          ProcessId*, void*, ProcessId*) -> ProcessId;

enum ProcessControl
{
    // `prctl` options:
    ARCH_SET_GS = 0x1001,
    ARCH_SET_FS = 0x1002,
    ARCH_GET_FS = 0x1003,
    ARCH_GET_GS = 0x1004,
    ARCH_GET_CPUID = 0x1011,
    ARCH_SET_CPUID = 0x1012,
    ARCH_MAP_VDSO_X32 = 0x2001,
    ARCH_MAP_VDSO_32 = 0x2002,
    ARCH_MAP_VDSO_64 = 0x2003,
    // `prctl` arguments:
    PR_SET_PDEATHSIG = 1,
    PR_GET_PDEATHSIG = 2,
    PR_GET_DUMPABLE = 3,
    PR_SET_DUMPABLE = 4,
    PR_GET_UNALIGN = 5,
    PR_SET_UNALIGN = 6,
    PR_UNALIGN_NOPRINT = 1,
    PR_UNALIGN_SIGBUS = 2,
    PR_GET_KEEPCAPS = 7,
    PR_SET_KEEPCAPS = 8,
    PR_GET_FPEMU = 9,
    PR_SET_FPEMU = 10,
    PR_FPEMU_NOPRINT = 1,
    PR_FPEMU_SIGFPE = 2,
    PR_GET_FPEXC = 11,
    PR_SET_FPEXC = 12,
    PR_FP_EXC_SW_ENABLE = 0x80,
    PR_FP_EXC_DIV = 0x010000,
    PR_FP_EXC_OVF = 0x020000,
    PR_FP_EXC_UND = 0x040000,
    PR_FP_EXC_RES = 0x080000,
    PR_FP_EXC_INV = 0x100000,
    PR_FP_EXC_DISABLED = 0,
    PR_FP_EXC_NONRECOV = 1,
    PR_FP_EXC_ASYNC = 2,
    PR_FP_EXC_PRECISE = 3,
    PR_GET_TIMING = 13,
    PR_SET_TIMING = 14,
    PR_TIMING_STATISTICAL = 0,
    PR_TIMING_TIMESTAMP = 1,
    PR_SET_NAME = 15,
    PR_GET_NAME = 16,
    PR_GET_ENDIAN = 19,
    PR_SET_ENDIAN = 20,
    PR_ENDIAN_BIG = 0,
    PR_ENDIAN_LITTLE = 1,
    PR_ENDIAN_PPC_LITTLE = 2,
    PR_GET_SECCOMP = 21,
    PR_SET_SECCOMP = 22,
    PR_CAPBSET_READ = 23,
    PR_CAPBSET_DROP = 24,
    PR_GET_TSC = 25,
    PR_SET_TSC = 26,
    PR_TSC_ENABLE = 1,
    PR_TSC_SIGSEGV = 2,
    PR_GET_SECUREBITS = 27,
    PR_SET_SECUREBITS = 28,
    PR_SET_TIMERSLACK = 29,
    PR_GET_TIMERSLACK = 30,
    PR_TASK_PERF_EVENTS_DISABLE = 31,
    PR_TASK_PERF_EVENTS_ENABLE = 32,
    PR_MCE_KILL = 33,
    PR_MCE_KILL_CLEAR = 0,
    PR_MCE_KILL_SET = 1,
    PR_MCE_KILL_LATE = 0,
    PR_MCE_KILL_EARLY = 1,
    PR_MCE_KILL_DEFAULT = 2,
    PR_MCE_KILL_GET = 34,
    PR_SET_MM = 35,
    PR_SET_MM_START_CODE = 1,
    PR_SET_MM_END_CODE = 2,
    PR_SET_MM_START_DATA = 3,
    PR_SET_MM_END_DATA = 4,
    PR_SET_MM_START_STACK = 5,
    PR_SET_MM_START_BRK = 6,
    PR_SET_MM_BRK = 7,
    PR_SET_MM_ARG_START = 8,
    PR_SET_MM_ARG_END = 9,
    PR_SET_MM_ENV_START = 10,
    PR_SET_MM_ENV_END = 11,
    PR_SET_MM_AUXV = 12,
    PR_SET_MM_EXE_FILE = 13,
    PR_SET_MM_MAP = 14,
    PR_SET_MM_MAP_SIZE = 15,
    PR_SET_PTRACER = 0x59616d61,
    PR_SET_PTRACER_ANY = (-1UL),
    PR_SET_CHILD_SUBREAPER = 36,
    PR_GET_CHILD_SUBREAPER = 37,
    PR_SET_NO_NEW_PRIVS = 38,
    PR_GET_NO_NEW_PRIVS = 39,
    PR_GET_TID_ADDRESS = 40,
    PR_SET_THP_DISABLE = 41,
    PR_GET_THP_DISABLE = 42,
    PR_MPX_ENABLE_MANAGEMENT = 43,
    PR_MPX_DISABLE_MANAGEMENT = 44,
    PR_SET_FP_MODE = 45,
    PR_GET_FP_MODE = 46,
    PR_FP_MODE_FR = (1 << 0),
    PR_FP_MODE_FRE = (1 << 1),
    PR_CAP_AMBIENT = 47,
    PR_CAP_AMBIENT_IS_SET = 1,
    PR_CAP_AMBIENT_RAISE = 2,
    PR_CAP_AMBIENT_LOWER = 3,
    PR_CAP_AMBIENT_CLEAR_ALL = 4,
    PR_SVE_SET_VL = 50,
    PR_SVE_SET_VL_ONEXEC = (1 << 18),
    PR_SVE_GET_VL = 51,
    PR_SVE_VL_LEN_MASK = 0xffff,
    PR_SVE_VL_INHERIT = (1 << 17),
    PR_GET_SPECULATION_CTRL = 52,
    PR_SET_SPECULATION_CTRL = 53,
    PR_SPEC_STORE_BYPASS = 0,
    PR_SPEC_INDIRECT_BRANCH = 1,
    PR_SPEC_NOT_AFFECTED = 0,
    PR_SPEC_PRCTL = (1UL << 0),
    PR_SPEC_ENABLE = (1UL << 1),
    PR_SPEC_DISABLE = (1UL << 2),
    PR_SPEC_FORCE_DISABLE = (1UL << 3),
    PR_SPEC_DISABLE_NOEXEC = (1UL << 4),
    PR_PAC_RESET_KEYS = 54,
    PR_PAC_APIAKEY = (1UL << 0),
    PR_PAC_APIBKEY = (1UL << 1),
    PR_PAC_APDAKEY = (1UL << 2),
    PR_PAC_APDBKEY = (1UL << 3),
    PR_PAC_APGAKEY = (1UL << 4),
    PR_SET_TAGGED_ADDR_CTRL = 55,
    PR_GET_TAGGED_ADDR_CTRL = 56,
    PR_TAGGED_ADDR_ENABLE = (1UL << 0),
    PR_MTE_TCF_SHIFT = 1,
    PR_MTE_TCF_NONE = (0UL << 1),
    PR_MTE_TCF_SYNC = (1UL << 1),
    PR_MTE_TCF_ASYNC = (2UL << 1),
    PR_MTE_TCF_MASK = (3UL << 1),
    PR_MTE_TAG_SHIFT = 3,
    PR_MTE_TAG_MASK = (0xffffUL << 3),
    PR_SET_IO_FLUSHER = 57,
    PR_GET_IO_FLUSHER = 58,
};

template <typename... T>
auto prctl(ProcessControl option, T... arguments) -> Result<>
requires(sizeof...(arguments) <= 4) {
    // TODO: Implement failure handling when syscalls are factored.
    syscall<void>(157u, option, arguments...).or_panic();
    return okay;
}

// auto clone(isize (*function)(void*), void* p_stack, i4 flags,
//            auto function_arguments, ProcessId* p_parent_thread_id = nullptr,
//            void* p_tls = nullptr, ProcessId* p_child_thread_id = nullptr)
//     -> ProcessId {
//     // register ProcessId result asm("rax");
//     // TODO: Replace this with inline asm.
//     // This is just Musl code.
//     clone_asm(function, p_stack, flags, function_arguments,
//     p_parent_thread_id,
//               p_tls, p_child_thread_id);
//     // TODO: Failure handling.
//     // return result;
//     return 0;
// }

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
                auto const& function, void* p_arguments_struct)
        -> Result<ProcessId> {
        u4 const flags = ::CLONE_VM | ::CLONE_FS | ::CLONE_FILES |
                         ::CLONE_SIGHAND | ::CLONE_THREAD | ::CLONE_SYSVSEM |
                         ::CLONE_SETTLS | ::CLONE_PARENT_SETTID |
                         ::CLONE_CHILD_CLEARTID | ::CLONE_DETACHED;

        this->stack_size = initial_stack_size;
        /* Allocate a stack for this thread, and get an address to the top of
         * it. We need the top because memory will be pushed to it downwards on
         * x86-64. */
        this->p_stack =
            allocator.malloc(stack_size).or_it_is(nullptr) + this->stack_size;
        if (this->p_stack == nullptr) {
            return Failure(1);
        }

        void* p_tls;
        prctl(ARCH_SET_GS, p_tls).or_panic();

        // TODO: This does not compile:
        // `clone()` will always return a value.
        this->process_id =
            clone_asm(&function, this->p_stack, flags, p_arguments_struct,
                      nullptr, p_tls, nullptr);
        // joinable = (exit_code == 0);
        return this->process_id;
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
