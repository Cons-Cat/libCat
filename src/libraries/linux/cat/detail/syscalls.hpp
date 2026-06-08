// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// This file forward declares Linux syscalls and provides their
// corresponding structs and enums.

#include <cat/linux>
#include <cat/string>

namespace nix {

enum class memory_protection_flags : unsigned char {
   none = 0b000,        // Data cannot be accessed at all.
   read = 0b001,        // Data is readable.
   write = 0b010,       // Data is writable.
   read_write = 0b011,  // Data is writable and readable.
   execute = 0b100,     // Data can be executed.
};

enum class memory_flags : unsigned int {
   shared = 0b1,           // Writes change the underlying object.
   privately = 0b10,       // Writes only change the calling process.
   fixed = 0b1'0000,       // Map to precisely this address, rather than virtual
                           // memory. This may fail.
   anonymous = 0b10'0000,  // Not backed by a file descriptor.
   // TODO: Make binary/hexa format consistent.
   grows_down = 0x00100,    // Stack-like segment.
   denywrite = 0x00800,     // ETXTBSY.
   executable = 0x01000,    // Mark it as an executable.
   locked = 0x02000,        // Lock the mapping.
   no_reserve = 0x04000,    // Don't check for reservations.
   populate = 0x08000,      // Populate (prefault) pagetables.
   non_blocking = 0x10000,  // Do not block on IO.
   stack = 0x20000,         // allocation_type is for a stack.
   hugetlb = 0x40000,       // Create huge page mapping.
   sync = 0x80000,          // Perform synchronous page faults for the mapping.
   fixed_noreplace = 0x100000,  // `mmap_memory_flags::fixed` but do not unmap
                                // underlying mapping.
};

enum class wait_id : unsigned char {
   all = 0,
   process_id = 1,
   process_group = 2,
   file_descriptor = 3,
};

enum class wait_options_flags : unsigned int {
   no_hang = 1,
   untraced = 2,
   stopped = 2,
   exited = 4,
   continued = 8,
   no_wait = 0x1000000,
   no_thread = 0x20000000,
   wait_all = 0x40000000,
   clone = 0x80000000,
};

// Futex command in the low bits of a futex `op` argument.
enum class futex_command : unsigned char {
   wait = 0,
   wake = 1,
   fd = 2,
   requeue = 3,
   compare_requeue = 4,
   wake_op = 5,
   lock_pi = 6,
   unlock_pi = 7,
   trylock_pi = 8,
   wait_bitset = 9,
   wake_bitset = 10,
   wait_requeue_pi = 11,
   compare_requeue_pi = 12,
   lock_pi2 = 13,
};

// Bit-or this with `futex_command` to build the `op` argument to `sys_futex()`.
enum class futex_options : unsigned int {
   none = 0,
   private_process = 128,
   clock_realtime = 256,
};

// Per-wait-entry flags in `struct futex_waitv::flags` (`linux/futex.h`,
// futex2). Underlying type is `unsigned int` so values like `clock_realtime`
// (256) fit and `struct futex_waitv` exposes a full `__u32`-sized flags word to
// the kernel.
enum class futex_wait_flags : unsigned int {
   word_32 = 2,
   word_64 = 3,
   private_process = 128,
   clock_realtime = 256,
};

// `flags` argument to `sys_futex_waitv()` (syscall `futex_waitv`,
// `man 2 futex_waitv`).
enum class futex_waitv_call_flags : unsigned char {
   none = 0,
};

// Robust priority-inheritance futex fields use these bits in the 32-bit word.
inline constexpr cat::uint4 futex_waiters_flag{0x80000000u};
inline constexpr cat::uint4 futex_owner_died_flag{0x40000000u};
inline constexpr cat::uint4 futex_tid_mask{0x3fffffffu};
inline constexpr cat::uint4 futex_bitset_match_any{0xffffffffu};

struct futex_timespec {
   cat::int8 seconds;
   cat::int8 nanoseconds;
};

// Kernel `sys_futex()` only cares about the address of this 32-bit word.
struct futex_word {
   cat::atomic<cat::uint4> m_value;
};

// Selects `basic_futex` behavior. `futex_plain` is an ordinary user-chosen
// futex word. `futex_robust` marks the same layout as used with the userspace
// robust list (`set_robust_list`/`get_robust_list`) and owner/waiter bits.
struct futex_plain {};

struct futex_robust {};

template <class Kind = futex_plain>
class basic_futex;

using futex = basic_futex<futex_plain>;
using robust_futex = basic_futex<futex_robust>;

template <class Kind>
class [[clang::preferred_name(futex), clang::preferred_name(robust_futex)]]
basic_futex : public futex_word {
 public:
   // Block only if `value` still equals `expected` (`futex_command::wait`).
   // Otherwise returns `linux_error::again` without sleeping.
   [[nodiscard]]
   auto
   wait(cat::uint4 expected,
        futex_timespec const* _Nullable p_timeout = nullptr)
      -> scaredy_nix<void>;

   // Wake at most one waiter. Returns the number of waiting threads woken.
   [[nodiscard]]
   auto
   wake() -> scaredy_nix<cat::idx>;

   // Wake as many waiters as possible. Returns the number of waiting threads
   // woken.
   [[nodiscard]]
   auto
   wake_all() -> scaredy_nix<cat::idx>;

   // TODO: Once timers are implemented, add blocking+timeout variants of
   // `wake()`.

   // Wake at most `wake_limit` waiters at the source and requeues up to
   // `requeue_limit` others to `target` only if the source word equals
   // `expected_source_value`. Otherwise, returns `linux_error::again`.
   [[nodiscard]]
   auto
   compare_requeue(cat::uint4 wake_limit, cat::uint4 requeue_limit,
                   futex_word& target, cat::uint4 expected_source_value)
      -> scaredy_nix<cat::idx>;

   // Robust mutex word layout (`futex_waiters_flag`, `futex_owner_died_flag`).
   [[nodiscard]]
   static constexpr auto
   unlocked() -> cat::uint4
      requires(cat::is_same<Kind, futex_robust>)
   {
      return 0u;
   }

   [[nodiscard]]
   static constexpr auto
   decode_owner_thread_id(cat::uint4 lock_word) -> cat::uint4
      requires(cat::is_same<Kind, futex_robust>)
   {
      return lock_word & futex_tid_mask;
   }

   [[nodiscard]]
   static constexpr auto
   has_waiters(cat::uint4 lock_word) -> bool
      requires(cat::is_same<Kind, futex_robust>)
   {
      return (lock_word & futex_waiters_flag) != 0u;
   }

   [[nodiscard]]
   static constexpr auto
   owner_exited_abnormally(cat::uint4 lock_word) -> bool
      requires(cat::is_same<Kind, futex_robust>)
   {
      return (lock_word & futex_owner_died_flag) != 0u;
   }
};

static_assert(sizeof(futex_word) == sizeof(cat::uint4));
static_assert(alignof(futex_word) == alignof(cat::uint4));
static_assert(sizeof(futex) == sizeof(cat::uint4));
static_assert(alignof(futex) == alignof(cat::uint4));
static_assert(sizeof(robust_futex) == sizeof(cat::uint4));

// Whence argument to `sys_lseek()`. The `data` and `hole` values are Linux
// extensions for sparse-file traversal.
enum class seek_whence : unsigned char {
   beginning = 0,
   current = 1,
   end = 2,
   data = 3,
   hole = 4,
};

// Flags argument to `sys_dup3()`. Mirrors `open_flags::close_exec`.
enum class dup3_flags : unsigned int {
   none = 0,
   close_exec = 02000000,
};

// Flags argument to `sys_pipe2()`. Same encoding as the matching
// `open_flags` bits.
enum class pipe2_flags : unsigned int {
   none = 0,
   nonblocking = 04000,
   close_exec = 02000000,
   direct = 040000,
   notification = 0x00800000,
};

// Flags argument to `sys_getrandom()`.
enum class getrandom_flags : unsigned int {
   none = 0,
   nonblocking = 0x0001,
   random = 0x0002,
   insecure = 0x0004,
};

// `mode` argument to `sys_access()`. `exists` (zero) tests for presence.
// The `readable`/`writable`/`executable` bits may be combined.
enum class access_mode : unsigned int {
   exists = 0,
   executable = 1,
   writable = 2,
   readable = 4,
};

// `sa_flags` argument to `sys_rt_sigaction()`. The kernel ABI names
// `unsigned long`, so this enum's underlying type is 8 bytes on x86_64.
enum class signal_action_flags : unsigned long {
   none = 0,
   no_child_stop = 0x00000001,  // SA_NOCLDSTOP
   no_child_wait = 0x00000002,  // SA_NOCLDWAIT
   siginfo = 0x00000004,        // SA_SIGINFO
   unsupported = 0x00000008,    // SA_UNSUPPORTED
   exposed_taglatching = 0x00000010,
   restorer = 0x04000000,         // SA_RESTORER (required on x86_64)
   on_stack = 0x08000000,         // SA_ONSTACK
   restart = 0x10000000,          // SA_RESTART
   no_defer = 0x40000000,         // SA_NODEFER
   reset_handler = 0x80000000ul,  // SA_RESETHAND
};

// `ss_flags` field of `signal_stack` and the corresponding sigaltstack
// argument bits.
enum class signal_stack_flags : unsigned int {
   none = 0,
   on_stack = 1,            // SS_ONSTACK
   disable = 2,             // SS_DISABLE
   auto_disarm = 1u << 31,  // SS_AUTODISARM (Linux extension)
};

// `how` argument to `sys_shutdown()`. Selects which half of a connected
// socket to tear down.
enum class shutdown_how : unsigned int {
   read = 0,        // SHUT_RD
   write = 1,       // SHUT_WR
   read_write = 2,  // SHUT_RDWR
};

// `flags` argument to `sys_accept4()`. Same encoding as the matching
// `open_flags` and `pipe2_flags` bits.
enum class accept4_flags : unsigned int {
   none = 0,
   nonblocking = 04000,
   close_exec = 02000000,
};

// File-mode permission bits. Matches POSIX `mode_t` octal layout.
enum class file_permissions : unsigned int {
   none = 0,
   other_execute = 01,
   other_write = 02,
   other_read = 04,
   group_execute = 010,
   group_write = 020,
   group_read = 040,
   user_execute = 0100,
   user_write = 0200,
   user_read = 0400,
   sticky = 01000,
   set_group_id = 02000,
   set_user_id = 04000,
};

// `cmd` argument to `sys_fcntl()`. Only the most commonly used commands are
// listed. The kernel ABI accepts arbitrary `int` values here.
enum class fcntl_command : int {
   duplicate_fd = 0,                 // F_DUPFD
   get_fd_flags = 1,                 // F_GETFD
   set_fd_flags = 2,                 // F_SETFD
   get_status_flags = 3,             // F_GETFL
   set_status_flags = 4,             // F_SETFL
   get_lock = 5,                     // F_GETLK
   set_lock = 6,                     // F_SETLK
   set_lock_wait = 7,                // F_SETLKW
   set_owner = 8,                    // F_SETOWN
   get_owner = 9,                    // F_GETOWN
   set_signal = 10,                  // F_SETSIG
   get_signal = 11,                  // F_GETSIG
   duplicate_fd_close_exec = 1'030,  // F_DUPFD_CLOEXEC
   set_pipe_size = 1'031,            // F_SETPIPE_SZ
   get_pipe_size = 1'032,            // F_GETPIPE_SZ
};

// `operation` argument to `sys_flock()`.
enum class flock_op : int {
   shared = 1,       // LOCK_SH
   exclusive = 2,    // LOCK_EX
   nonblocking = 4,  // LOCK_NB
   unlock = 8,       // LOCK_UN
};

// `flags` argument to `*at()`-family syscalls (`sys_openat`,
// `sys_unlinkat`, `sys_fchmodat`, etc.). Not every flag is meaningful for
// every syscall.
enum class atfile_flags : int {
   none = 0,
   no_follow = 0x100,          // AT_SYMLINK_NOFOLLOW
   remove_directory = 0x200,   // AT_REMOVEDIR
   eaccess = 0x200,            // AT_EACCESS (faccessat only)
   follow = 0x400,             // AT_SYMLINK_FOLLOW
   no_auto_mount = 0x800,      // AT_NO_AUTOMOUNT
   empty_path = 0x1000,        // AT_EMPTY_PATH
   statx_force_sync = 0x2000,  // AT_STATX_FORCE_SYNC (statx only)
   statx_dont_sync = 0x4000,   // AT_STATX_DONT_SYNC (statx only)
};

// Special `dirfd` value meaning "resolve `path` against the current working
// directory" for any `*at()` syscall.
inline constexpr file_descriptor at_fdcwd = {-100};

// `mode` argument to `sys_fallocate()`.
enum class fallocate_flags : int {
   none = 0,
   keep_size = 0x01,       // FALLOC_FL_KEEP_SIZE
   punch_hole = 0x02,      // FALLOC_FL_PUNCH_HOLE
   no_hide_stale = 0x04,   // FALLOC_FL_NO_HIDE_STALE
   collapse_range = 0x08,  // FALLOC_FL_COLLAPSE_RANGE
   zero_range = 0x10,      // FALLOC_FL_ZERO_RANGE
   insert_range = 0x20,    // FALLOC_FL_INSERT_RANGE
   unshare_range = 0x40,   // FALLOC_FL_UNSHARE_RANGE
};

// `flags` argument to `sys_renameat2()`.
enum class renameat2_flags : unsigned int {
   none = 0,
   no_replace = 1,  // RENAME_NOREPLACE
   exchange = 2,    // RENAME_EXCHANGE
   whiteout = 4,    // RENAME_WHITEOUT
};

// `flags` argument to `sys_mlockall()`. At least one of `current` or
// `future` must be set. `on_fault` is a Linux 4.4+ extension that defers
// locking until the page is first faulted in.
enum class mlockall_flags : unsigned int {
   none = 0,
   current = 1,   // MCL_CURRENT
   future = 2,    // MCL_FUTURE
   on_fault = 4,  // MCL_ONFAULT
};

// `flags` argument to `sys_mlock2()`.
enum class mlock2_flags : unsigned int {
   none = 0,
   on_fault = 1,  // MLOCK_ONFAULT
};

// `flags` field of `io_uring_params` (the third argument to
// `sys_io_uring_setup()`). Mirrors the kernel's `IORING_SETUP_*` constants.
enum class io_uring_setup_flags : unsigned int {
   none = 0,
   io_poll = 1u << 0,              // IORING_SETUP_IOPOLL
   sq_poll = 1u << 1,              // IORING_SETUP_SQPOLL
   sq_affinity = 1u << 2,          // IORING_SETUP_SQ_AFF
   completion_size = 1u << 3,      // IORING_SETUP_CQSIZE
   clamp = 1u << 4,                // IORING_SETUP_CLAMP
   attach_wq = 1u << 5,            // IORING_SETUP_ATTACH_WQ
   ring_disabled = 1u << 6,        // IORING_SETUP_R_DISABLED
   submit_all = 1u << 7,           // IORING_SETUP_SUBMIT_ALL
   cooperative_taskrun = 1u << 8,  // IORING_SETUP_COOP_TASKRUN
   taskrun_flag = 1u << 9,         // IORING_SETUP_TASKRUN_FLAG
   sqe_128 = 1u << 10,             // IORING_SETUP_SQE128
   cqe_32 = 1u << 11,              // IORING_SETUP_CQE32
   single_issuer = 1u << 12,       // IORING_SETUP_SINGLE_ISSUER
   defer_taskrun = 1u << 13,       // IORING_SETUP_DEFER_TASKRUN
   no_mmap = 1u << 14,             // IORING_SETUP_NO_MMAP
   registered_fd_only = 1u << 15,  // IORING_SETUP_REGISTERED_FD_ONLY
   no_sq_array = 1u << 16,         // IORING_SETUP_NO_SQARRAY
};

// `flags` argument to `sys_io_uring_enter()`. Mirrors the kernel's
// `IORING_ENTER_*` constants.
enum class io_uring_enter_flags : unsigned int {
   none = 0,
   get_events = 1u << 0,       // IORING_ENTER_GETEVENTS
   sq_wakeup = 1u << 1,        // IORING_ENTER_SQ_WAKEUP
   sq_wait = 1u << 2,          // IORING_ENTER_SQ_WAIT
   ext_arg = 1u << 3,          // IORING_ENTER_EXT_ARG
   registered_ring = 1u << 4,  // IORING_ENTER_REGISTERED_RING
};

// `op` argument to `sys_io_uring_register()`. Mirrors `IORING_REGISTER_*` /
// `IORING_UNREGISTER_*`. Newer codes can be passed via `cat::uint4` cast.
enum class io_uring_register_op : unsigned int {
   register_buffers = 0,            // IORING_REGISTER_BUFFERS
   unregister_buffers = 1,          // IORING_UNREGISTER_BUFFERS
   register_files = 2,              // IORING_REGISTER_FILES
   unregister_files = 3,            // IORING_UNREGISTER_FILES
   register_eventfd = 4,            // IORING_REGISTER_EVENTFD
   unregister_eventfd = 5,          // IORING_UNREGISTER_EVENTFD
   register_files_update = 6,       // IORING_REGISTER_FILES_UPDATE
   register_eventfd_async = 7,      // IORING_REGISTER_EVENTFD_ASYNC
   register_probe = 8,              // IORING_REGISTER_PROBE
   register_personality = 9,        // IORING_REGISTER_PERSONALITY
   unregister_personality = 10,     // IORING_UNREGISTER_PERSONALITY
   register_restrictions = 11,      // IORING_REGISTER_RESTRICTIONS
   register_enable_rings = 12,      // IORING_REGISTER_ENABLE_RINGS
   register_files2 = 13,            // IORING_REGISTER_FILES2
   register_files_update2 = 14,     // IORING_REGISTER_FILES_UPDATE2
   register_buffers2 = 15,          // IORING_REGISTER_BUFFERS2
   register_buffers_update = 16,    // IORING_REGISTER_BUFFERS_UPDATE
   register_iowq_aff = 17,          // IORING_REGISTER_IOWQ_AFF
   unregister_iowq_aff = 18,        // IORING_UNREGISTER_IOWQ_AFF
   register_iowq_max_workers = 19,  // IORING_REGISTER_IOWQ_MAX_WORKERS
   register_ring_fds = 20,          // IORING_REGISTER_RING_FDS
   unregister_ring_fds = 21,        // IORING_UNREGISTER_RING_FDS
   register_pbuf_ring = 22,         // IORING_REGISTER_PBUF_RING
   unregister_pbuf_ring = 23,       // IORING_UNREGISTER_PBUF_RING
   register_sync_cancel = 24,       // IORING_REGISTER_SYNC_CANCEL
   register_file_alloc_range = 25,  // IORING_REGISTER_FILE_ALLOC_RANGE
   register_pbuf_status = 26,       // IORING_REGISTER_PBUF_STATUS
   register_napi = 27,              // IORING_REGISTER_NAPI
   unregister_napi = 28,            // IORING_UNREGISTER_NAPI
   register_clock = 29,             // IORING_REGISTER_CLOCK
   register_clone_buffers = 30,     // IORING_REGISTER_CLONE_BUFFERS
};

// `mask` bits for `sys_statx()` selecting which fields the kernel must fill
// in the output `statx_data`.
enum class statx_mask : unsigned int {
   type = 0x0001,
   mode = 0x0002,
   nlink = 0x0004,
   uid = 0x0008,
   gid = 0x0010,
   atime = 0x0020,
   mtime = 0x0040,
   ctime = 0x0080,
   ino = 0x0100,
   size = 0x0200,
   blocks = 0x0400,
   basic_stats = 0x07ff,
   btime = 0x0800,
   mount_id = 0x1000,
   dio_align = 0x2000,
   subvol = 0x8000,
};

// `utimbuf` is the kernel ABI for `sys_utime()`.
struct utimbuf {
   cat::iword access_time;
   cat::iword modification_time;
};

// `open_how` is the kernel ABI for `sys_openat2()`. `resolve` is a bitmask of
// `RESOLVE_*` flags (e.g. `RESOLVE_NO_SYMLINKS`). See `man 2 openat2`.
struct open_how {
   cat::uint8 flags;
   cat::uint8 mode;
   cat::uint8 resolve;
};

// `statx_timestamp` and `statx_data` together match the kernel ABI for
// `sys_statx()` (`struct statx`). Total size is 256 bytes.
struct statx_timestamp {
   cat::int8 seconds;
   cat::uint4 nanoseconds;

 private:
   [[maybe_unused]]
   cat::int4 m_padding;
};

static_assert(sizeof(statx_timestamp) == 16);

struct statx_data {
   cat::uint4 mask;
   cat::uint4 block_size;
   cat::uint8 attributes;
   cat::uint4 hard_link_count;
   cat::uint4 user;
   cat::uint4 group;
   cat::uint2 protections_mode;

 private:
   [[maybe_unused]]
   cat::uint2 m_padding_0[1];

 public:
   cat::uint8 inode;
   cat::uint8 file_size;
   cat::uint8 blocks_count;
   cat::uint8 attributes_mask;

   statx_timestamp last_access_time;
   statx_timestamp creation_time;
   statx_timestamp last_change_time;
   statx_timestamp last_modification_time;

   cat::uint4 rdev_major;
   cat::uint4 rdev_minor;
   cat::uint4 device_major;
   cat::uint4 device_minor;
   cat::uint8 mount_id;
   cat::uint8 dio_memory_alignment;
   cat::uint8 dio_offset_alignment;
   cat::uint8 subvolume;
   cat::uint4 atomic_write_unit_min;
   cat::uint4 atomic_write_unit_max;
   cat::uint4 atomic_write_segments_max;

 private:
   [[maybe_unused]]
   cat::uint4 m_padding_1[1];
   [[maybe_unused]]
   cat::uint8 m_padding_2[8];
};

static_assert(sizeof(statx_data) == 256);

// `statfs_data` matches the kernel ABI for `sys_statfs()` /
// `sys_fstatfs()`. Total size is 120 bytes on x86-64.
struct statfs_data {
   cat::iword type;
   cat::iword block_size;
   cat::uword total_blocks;
   cat::uword free_blocks;
   cat::uword available_blocks;
   cat::uword total_files;
   cat::uword free_files;
   cat::int4 file_system_id[2];
   cat::iword name_max;
   cat::iword fragment_size;
   cat::iword flags;

 private:
   [[maybe_unused]]
   cat::iword m_spare[4];
};

static_assert(sizeof(statfs_data) == 120);

// `linux_dirent64` matches the kernel ABI for `sys_getdents64()` (the
// preferred form on 64-bit). Records are variable-length, advance by
// `record_length` to reach the next entry.
enum class file_type : unsigned char {
   unknown = 0,
   fifo = 1,
   character_device = 2,
   directory = 4,
   block_device = 6,
   regular = 8,
   symbolic_link = 10,
   socket = 12,
};

struct linux_dirent64 {
   cat::uint8 inode;
   cat::int8 next_offset;
   cat::uint2 record_length;
   file_type type;
   char name[];  // NUL-terminated, length implicit in record_length.
};

// `linux_dirent` matches the kernel ABI for `sys_getdents()` (the legacy
// form). Records are variable-length and do not carry a `file_type`. See
// `linux_dirent64` for the preferred form.
struct linux_dirent {
   cat::uword inode;
   cat::iword next_offset;
   cat::uint2 record_length;
   char name[];  // NUL-terminated, length implicit in record_length.
};

// `kernel_version` is the parsed `major.minor` pair from `utsname::release`.
struct kernel_version {
   cat::int4 major;
   cat::int4 minor;

   constexpr auto
   operator<=>(kernel_version const&) const = default;
};

// Parse the host's kernel version from `sys_uname()`.
[[nodiscard]]
auto
get_kernel_version() -> kernel_version;

namespace detail {
// Per-check probe results, populated once in `_start()`.
// Each check is stored in a separate variable for dead-store elimination.
#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_cachestat_cache = false;  // NOLINT

#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_fchmodat2_cache = false;  // NOLINT

#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_map_shadow_stack_cache = false;  // NOLINT

#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_futex_wake_cache = false;  // NOLINT

#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_futex_wait_cache = false;  // NOLINT

#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_futex_requeue_cache = false;  // NOLINT

#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_mseal_cache = false;  // NOLINT

#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline bool has_sys_io_uring_cache = false;  // NOLINT

}  // namespace detail

// `utsname` is the Linux ABI for `sys_uname()`. Each field is 65 bytes
// (`__NEW_UTS_LEN + 1`) and the kernel zero-fills the unused tail, so
// every slot is always NUL-terminated.
struct utsname {
   cat::zstr_inplace<65> sysname;
   cat::zstr_inplace<65> nodename;
   cat::zstr_inplace<65> release;
   cat::zstr_inplace<65> version;
   cat::zstr_inplace<65> machine;
   cat::zstr_inplace<65> domainname;
};

// Pin the kernel ABI. `zstr_inplace<65>` collapses its empty CRTP bases
// via EBO, leaving a flat `char[65]` payload, so the struct must occupy
// exactly 6 * 65 bytes with byte alignment.
static_assert(sizeof(utsname) == 6u * 65u);
static_assert(alignof(utsname) == 1u);

enum class open_mode : unsigned char {
   read_only = 00,
   write_only = 01,
   // This flag cannot be use on a FIFO.
   read_write = 02,
};

enum class open_flags : unsigned int {
   // This will create a new file. If that file already exists, it is no-op
   // unless combined with `open_flags::exclusive`.
   create = 0100,
   // This flag can only be used in combination with `open_flags::create`. This
   // will make a syscall fail if the file already exists.
   exclusive = 0200,
   no_control_tty = 0400,
   truncate = 01000,
   append_file = 02000,
   nonblocking = 04000,
   // Write I/O operations on the `file_descriptor` shall complete as defined by
   // synchronized I/O data integrity completion.
   dsync = 010000,
   sync = 04010000,
   read_sync = 04010000,
   directory = 0200000,
   nofollow = 0400000,
   // Close the `file_descriptor` automatically when finished with this
   // operation.
   close_exec = 02000000,

   async = 020000,
   direct = 040000,
   largefile = 0100000,
   noatime = 01000000,
   path = 010000000,
   temporary_file = 020200000,
   // This flag is used by `open_file()` implicitly. It specifies that the
   // offset value is 8-bytes.
   large_file = 0100000,
};

// TODO: Comment wtf these mean.
enum class io_requests : unsigned int {
   none = 0,
   write = 1,
   read = 2,
   tcgets = 0x5401,
   tcsets = 0x5402,
   tcsetsw = 0x5403,
   tcsetsf = 0x5404,
   tcgeta = 0x5405,
   tcseta = 0x5406,
   tcsetaw = 0x5407,
   tcsetaf = 0x5408,
   tcsbrk = 0x5409,
   tcxonc = 0x540A,
   tcflsh = 0x540B,
   tiocexcl = 0x540C,
   tiocnxcl = 0x540D,
   tiocsctty = 0x540E,
   tiocgpgrp = 0x540F,
   tiocspgrp = 0x5410,
   tiocoutq = 0x5411,
   tiocsti = 0x5412,
   tiocgwinsz = 0x5413,
   tiocswinsz = 0x5414,
   tiocmget = 0x5415,
   tiocmbis = 0x5416,
   tiocmbic = 0x5417,
   tiocmset = 0x5418,
   tiocgsoftcar = 0x5419,
   tiocssoftcar = 0x541A,
   fionread = 0x541B,
   tiocinq = fionread,
   tioclinux = 0x541C,
   tioccons = 0x541D,
   tiocgserial = 0x541E,
   tiocsserial = 0x541F,
   tiocpkt = 0x5420,
   fionbio = 0x5421,
   tiocnotty = 0x5422,
   tiocsetd = 0x5423,
   tiocgetd = 0x5424,
   tcsbrkp = 0x5425,
   tiocsbrk = 0x5427,
   tioccbrk = 0x5428,
   tiocgsid = 0x5429,
   tiocgrs485 = 0x542E,
   tiocsrs485 = 0x542F,
   tiocgptn = 0x80045430,
   tiocsptlck = 0x40045431,
   tiocgdev = 0x80045432,
   tcgetx = 0x5432,
   tcsetx = 0x5433,
   tcsetxf = 0x5434,
   tcsetxw = 0x5435,
   tiocsig = 0x40045436,
   tiocvhangup = 0x5437,
   tiocgpkt = 0x80045438,
   tiocgptlck = 0x80045439,
   tiocgexcl = 0x80045440,
   tiocgptpeer = 0x5441,
   tiocgiso7816 = 0x80285442,
   tiocsiso7816 = 0xc0285443,
   fionclex = 0x5450,
   fioclex = 0x5451,
   fioasync = 0x5452,
   tiocserconfig = 0x5453,
   tiocsergwild = 0x5454,
   tiocserswild = 0x5455,
   tiocglcktrmios = 0x5456,
   tiocslcktrmios = 0x5457,
   tiocsergstruct = 0x5458,
   tiocsergetlsr = 0x5459,
   tiocsergetmulti = 0x545A,
   tiocsersetmulti = 0x545B,
   tiocmiwait = 0x545C,
   tiocgicount = 0x545D,
   fioqsize = 0x5460,
   tiocm_le = 0x001,
   tiocm_dtr = 0x002,
   tiocm_rts = 0x004,
   tiocm_st = 0x008,
   tiocm_sr = 0x010,
   tiocm_cts = 0x020,
   tiocm_car = 0x040,
   tiocm_rng = 0x080,
   tiocm_dsr = 0x100,
   tiocm_cd = tiocm_car,
   tiocm_ri = tiocm_rng,
   tiocm_out1 = 0x2000,
   tiocm_out2 = 0x4000,
   tiocm_loop = 0x8000,
   fiosetown = 0x8901,
   siocspgrp = 0x8902,
   fiogetown = 0x8903,
   siocgpgrp = 0x8904,
   siocatmark = 0x8905,
};

namespace detail {
consteval auto
compose_io_request(io_requests a, io_requests b, io_requests c, io_requests d)
   -> io_requests {
   return io_requests{(cat::to_underlying(a) << 30)
                      | (cat::to_underlying(b) << 8)
                      | cat::to_underlying(c)
                      | (cat::to_underlying(d) << 16)};
}
}  // namespace detail

consteval auto
io_request(io_requests a, io_requests b) -> io_requests {
   return detail::compose_io_request(io_requests::none, a, b,
                                     io_requests::none);
}

consteval auto
io_request_write(io_requests a, io_requests b, io_requests c) -> io_requests {
   return detail::compose_io_request(io_requests::write, a, b,
                                     io_requests(sizeof(c)));
}

consteval auto
io_request_read(io_requests a, io_requests b, io_requests c) -> io_requests {
   return detail::compose_io_request(io_requests::read, a, b,
                                     io_requests(sizeof(c)));
}

consteval auto
io_request_readwrite(io_requests a, io_requests b, io_requests c)
   -> io_requests {
   return detail::compose_io_request(
      io_requests{cat::to_underlying(io_requests::read)
                  | cat::to_underlying(io_requests::write)},
      a, b, io_requests(sizeof(c)));
}

// TODO: Comment wtf these mean.
// NOLINTNEXTLINE The ABI requires this by 4 bytes.
enum class tty_configuration_flags : unsigned int {
   vintr = 0,
   vquit = 1,
   verase = 2,
   vkill = 3,
   veof = 4,
   vtime = 5,
   vmin = 6,
   vswtc = 7,
   vstart = 8,
   vstop = 9,
   vsusp = 10,
   veol = 11,
   vreprint = 12,
   vdiscard = 13,
   vwerase = 14,
   vlnext = 15,
   veol2 = 16,
   ignbrk = 0000001,
   brkint = 0000002,
   ignpar = 0000004,
   parmrk = 0000010,
   inpck = 0000020,
   istrip = 0000040,
   inlcr = 0000100,
   igncr = 0000200,
   icrnl = 0000400,
   iuclc = 0001000,
   ixon = 0002000,
   ixany = 0004000,
   ixoff = 0010000,
   imaxbel = 0020000,
   iutf8 = 0040000,
   opost = 0000001,
   olcuc = 0000002,
   onlcr = 0000004,
   ocrnl = 0000010,
   onocr = 0000020,
   onlret = 0000040,
   ofill = 0000100,
   ofdel = 0000200,
   nldly = 0000400,
   nl0 = 0000000,
   nl1 = 0000400,
   crdly = 0003000,
   cr0 = 0000000,
   cr1 = 0001000,
   cr2 = 0002000,
   cr3 = 0003000,
   tabdly = 0014000,
   tab0 = 0000000,
   tab1 = 0004000,
   tab2 = 0010000,
   tab3 = 0014000,
   bsdly = 0020000,
   bs0 = 0000000,
   bs1 = 0020000,
   ffdly = 0100000,
   ff0 = 0000000,
   ff1 = 0100000,
   vtdly = 0040000,
   vt0 = 0000000,
   vt1 = 0040000,
   b0 = 0000000,
   b50 = 0000001,
   b75 = 0000002,
   b110 = 0000003,
   b134 = 0000004,
   b150 = 0000005,
   b200 = 0000006,
   b300 = 0000007,
   b600 = 0000010,
   b1200 = 0000011,
   b1800 = 0000012,
   b2400 = 0000013,
   b4800 = 0000014,
   b9600 = 0000015,
   b19200 = 0000016,
   b38400 = 0000017,
   b57600 = 0010001,
   b115200 = 0010002,
   b230400 = 0010003,
   b460800 = 0010004,
   b500000 = 0010005,
   b576000 = 0010006,
   b921600 = 0010007,
   b1000000 = 0010010,
   b1152000 = 0010011,
   b1500000 = 0010012,
   b2000000 = 0010013,
   b2500000 = 0010014,
   b3000000 = 0010015,
   b3500000 = 0010016,
   b4000000 = 0010017,
   csize = 0000060,
   cs5 = 0000000,
   cs6 = 0000020,
   cs7 = 0000040,
   cs8 = 0000060,
   cstopb = 0000100,
   cread = 0000200,
   parenb = 0000400,
   parodd = 0001000,
   hupcl = 0002000,
   clocal = 0004000,
   isig = 0000001,
   icanon = 0000002,
   echo = 0000010,
   echoe = 0000020,
   echok = 0000040,
   echonl = 0000100,
   noflsh = 0000200,
   tostop = 0000400,
   iexten = 0100000,
   ooff = 0,
   oon = 1,
   ioff = 2,
   ion = 3,
   iflush = 0,
   oflush = 1,
   ioflush = 2,
   sanow = 0,
   sadrain = 1,
   saflush = 2,
};

}  // namespace nix

// Enable using these `enum class`es as bit-flags.
template <>
struct cat::enum_flag_trait<nix::memory_protection_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::memory_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::open_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::wait_options_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::futex_options> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::futex_wait_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::futex_waitv_call_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_requests> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::tty_configuration_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::dup3_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::pipe2_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::getrandom_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::access_mode> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::signal_action_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::signal_stack_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::accept4_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::file_permissions> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::flock_op> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::atfile_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::fallocate_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::renameat2_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::statx_mask> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::mlockall_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::mlock2_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_setup_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_enter_flags> : cat::true_trait {};

namespace nix {

// TODO: These can return `idx` rather than `iword`.

// Syscall 0.
auto
sys_read(file_descriptor file_descriptor, char* _Nonnull p_string_buffer,
         cat::iword length) -> scaredy_nix<cat::iword>;

// Syscall 1.
auto
sys_write(file_descriptor file_descriptor, char const* _Nonnull p_string_buffer,
          cat::iword length) -> scaredy_nix<cat::idx>;

auto
sys_write(file_descriptor file_descriptor, cat::str_view string)
   -> scaredy_nix<cat::idx>;

// Syscall 2.
auto
sys_open(char const* _Nonnull p_file_path, open_mode file_mode,
         open_flags flags = open_flags(0)) -> scaredy_nix<file_descriptor>;

// Syscall 3.
auto
sys_close(file_descriptor descriptor) -> scaredy_nix<void>;

// TODO: Flesh this out more.
// TODO: Extract this to an implementation file.
struct file_handle {
   cat::int4 flags;  // TODO: Make this strongly typed.
   char* _Nullable p_read_position;
   char* _Nullable p_read_end;

   cat::int4 (*_Nullable close)(file_handle* _Nonnull);

   char* _Nullable p_write_end;
   char* _Nullable p_write_position;
   unsigned char* _Nullable mustbezero_1;
   char* _Nullable p_write_base;

   cat::uword (*_Nullable read)(file_handle* _Nonnull, char* _Nonnull,
                                cat::uword);
   cat::uword (*_Nullable write)(file_handle* _Nonnull, char const* _Nonnull,
                                 cat::uword);
   cat::uword (*_Nullable seek)(file_handle* _Nonnull, cat::uword, cat::int4);
   char* _Nullable buf;

   cat::uword buf_size;
   file_handle* _Nullable prev;
   file_handle* _Nullable next;
   cat::int4 file_descriptor;
   cat::int4 pipe_pid;
   long lockcount;
   cat::int4 mode;
   cat::int4 volatile lock;
   cat::int4 lbf;
   void* _Nullable cookie;
   cat::uword off;
   char* _Nullable getln_buf;
   void* _Nullable mustbezero_2;
   char* _Nullable shend;
   cat::uword shlim, shcnt;
   file_handle* _Nullable p_prev_locked;
   file_handle* _Nullable p_next_locked;
   // TODO: Implement locale.

 private:
   [[maybe_unused]] void* _Nullable m_p_locale;
   // Locale* locale;
};

// Syscall 4. Fills `out` with the status of the file at `file_path`.
auto
sys_stat(cat::str_view file_path, file_status& out) -> scaredy_nix<void>;

// Syscall 5.
auto
sys_fstat(file_descriptor file_descriptor)
   -> cat::scaredy<file_status, linux_error>;

// Syscall 6. `lstat()` does not follow symbolic links. Fills `out`.
auto
sys_lstat(cat::str_view file_path, file_status& out) -> scaredy_nix<void>;

// TODO: `sys_poll()` (syscall 7).

// Syscall 8. Returns the resulting absolute offset.
auto
sys_lseek(file_descriptor file_descriptor, cat::iword offset,
          seek_whence whence) -> scaredy_nix<cat::iword>;

// Syscall 9. `p_start_address` is a hint; passing `nullptr` lets the
// kernel choose the mapping address.
auto
sys_mmap(void* _Nullable p_start_address, cat::uword bytes_size,
         memory_protection_flags protections, memory_flags flags,
         file_descriptor file_descriptor, cat::uword page_offset)
   -> scaredy_nix<void* _Nullable>;

// Syscall 10. Change the protection of the page-aligned range
// [`p_address`, `p_address + length`).
auto
sys_mprotect(void* _Nonnull p_address, cat::uword length,
             memory_protection_flags protections) -> scaredy_nix<void>;

// Syscall 11.
auto
sys_munmap(void const* _Nonnull p_memory, cat::uword length)
   -> scaredy_nix<void>;

// Syscall 12. `brk(0)` queries the current program break. A non-null
// argument adjusts it. Returns the resulting break address.
auto
sys_brk(void* _Nullable p_address) -> scaredy_nix<void* _Nullable>;

struct signals_mask_set {
   // TODO: A `cat::bitset` could simplify this.
   cat::array<unsigned char, 8u> bytes;
};

inline constexpr signals_mask_set all_signals_mask = {
   .bytes = cat::make_array_filled<8u>(static_cast<unsigned char>(-1)),
};

// `signal_info` matches the kernel's `siginfo_t`. The first three fields are
// always populated. The remaining 116 bytes form a tagged union whose active
// member depends on `code`. Layout matches the Linux x86-64 ABI exactly.
struct signal_info {
   cat::int4 signal_number;
   cat::int4 errno_code;
   cat::int4 code;

 private:
   [[maybe_unused]]
   cat::int4 m_padding;
   [[maybe_unused]]
   cat::uint8 m_fields[14];
};

static_assert(sizeof(signal_info) == 128);

// `sigaction` is the kernel ABI for `sys_rt_sigaction()`. On x86-64 the
// `restorer` slot must be populated and `signal_action_flags::restorer` set,
// otherwise the kernel returns to a stale return address after the handler.
struct sigaction {
   using handler_fn = void (*_Nullable)(cat::int4);
   using sigaction_fn = void (*_Nullable)(cat::int4, signal_info* _Nonnull,
                                          void* _Nullable);

   union {
      handler_fn handler;
      sigaction_fn sigaction_handler;
   };

   signal_action_flags flags;
   void (*_Nullable restorer)();
   signals_mask_set mask;
};

static_assert(sizeof(sigaction) == 32);

// `signal_stack` matches `stack_t`. Used by `sys_sigaltstack()` to install or
// query the alternate stack delivered to handlers with
// `signal_action_flags::on_stack` set.
struct signal_stack {
   void* _Nullable p_stack;
   signal_stack_flags flags;
   cat::uword size;
};

static_assert(sizeof(signal_stack) == 24);

// Syscall 13. Install a handler for `s`. Pass `nullptr` for `p_act` to query
// the current handler. Pass `nullptr` for `p_old_act` to discard it.
auto
sys_rt_sigaction(signal s, sigaction const* _Nullable __restrict p_act,
                 sigaction* _Nullable __restrict p_old_act)
   -> scaredy_nix<void>;

// Syscall 14.
auto
// Pass `nullptr` for `p_other_set` to query without modifying, or for
// `p_current_set` to discard the previous mask.
sys_rt_sigprocmask(signal_action action,
                   signals_mask_set const* _Nullable __restrict p_other_set,
                   signals_mask_set* _Nullable __restrict p_current_set)
   -> scaredy_nix<void>;

// Syscall 15 (`rt_sigreturn`) is the kernel-internal signal-handler trampoline
// and is not callable directly from user code.

// Syscall 16.
auto
sys_ioctl(file_descriptor io_descriptor, io_requests request)
   -> scaredy_nix<void>;

auto
sys_ioctl(file_descriptor io_descriptor, io_requests request,
          cat::no_type_ptr p_argument) -> scaredy_nix<void>;

// Syscall 17. Read up to `length` bytes from `file_descriptor` at `offset`.
auto
sys_pread64(file_descriptor file_descriptor, void* _Nonnull p_buffer,
            cat::iword length, cat::iword offset) -> scaredy_nix<cat::iword>;

// Syscall 18.
auto
sys_pwrite64(file_descriptor file_descriptor, void const* _Nonnull p_buffer,
             cat::iword length, cat::iword offset) -> scaredy_nix<cat::idx>;

struct io_vector : cat::span<cat::byte> {
   constexpr io_vector() = default;
   io_vector(io_vector const&) = default;
   constexpr io_vector(io_vector&&) = default;
   ~io_vector() = default;

   // Construct from any reasonable pointer type.
   constexpr io_vector(cat::byte* _Nonnull p_in_data, cat::idx in_length)
       : cat::span<cat::byte>(p_in_data, in_length) {
   }

   // Construct from any reasonable pointer type.
   constexpr io_vector(cat::byte* _Nonnull p_start, cat::byte* _Nonnull p_end)
       : cat::span<cat::byte>(p_start,
                              cat::idx(cat::uintptr<cat::byte>(p_end)
                                       - cat::uintptr<cat::byte>(p_start))) {
   }

   constexpr auto
   operator=(io_vector const& io_vector) -> struct io_vector& {
      m_p_data = io_vector.m_p_data;
      m_size = io_vector.m_size;
      return *this;
   }

   constexpr auto
   operator=(io_vector&& io_vector) -> struct io_vector& {
      m_p_data = io_vector.m_p_data;
      m_size = cat::move(io_vector).m_size;
      return *this;
   }
};

// Syscall 19.
auto
sys_readv(file_descriptor file_descriptor, cat::span<io_vector> const& vectors)
   -> scaredy_nix<cat::iword>;

// Syscall 20.
auto
sys_writev(file_descriptor file_descriptor, cat::span<io_vector> const& vectors)
   -> scaredy_nix<cat::idx>;

// Syscall 21. Check accessibility of `file_path` for the calling process's
// real user/group id. `mode` is `access_mode::exists` to test for presence,
// or any combination of the `readable`/`writable`/`executable` bits.
auto
sys_access(char const* _Nonnull p_file_path, access_mode mode)
   -> scaredy_nix<void>;

// Syscall 22. `pipefd[0]` becomes the read end and `pipefd[1]` the write end.
auto sys_pipe(cat::int4 (&pipefd)[2]) -> scaredy_nix<void>;

// Syscall 24.
auto
sys_sched_yield() -> scaredy_nix<void>;

// Syscall 32. Duplicate a file descriptor onto the lowest available number.
auto
sys_dup(file_descriptor fd) -> scaredy_nix<file_descriptor>;

// Syscall 33. Duplicate `oldfd` onto exactly `newfd`, atomically closing
// any descriptor previously at `newfd`.
auto
sys_dup2(file_descriptor oldfd, file_descriptor newfd)
   -> scaredy_nix<file_descriptor>;

// Syscall 39.
auto
sys_getpid() -> process_id;

// Syscall 41.
auto
sys_socket(cat::int8 protocol_family, cat::int8 type, cat::int8 protocol)
   -> scaredy_nix<file_descriptor>;

// Syscall 42.
auto
sys_connect(file_descriptor socket_descriptor, void const* _Nonnull p_socket,
            cat::iword socket_size) -> scaredy_nix<void>;

// Syscall 43. Pass `nullptr` for `p_socket` and `p_addr_len` to accept
// without retrieving the peer address.
auto
sys_accept(file_descriptor socket_descriptor,
           void* _Nullable __restrict p_socket = nullptr,
           cat::iword const* _Nullable __restrict p_addr_len = nullptr)
   -> scaredy_nix<file_descriptor>;

// Syscall 44. Pass `nullptr` for `p_destination_socket` when sending on a
// connected socket.
auto
sys_sendto(file_descriptor socket_descriptor,
           void const* _Nonnull p_message_buffer, cat::iword buffer_length,
           cat::int8 flags,
           cat::Socket const* _Nullable p_destination_socket = nullptr,
           cat::iword addr_length = 0) -> scaredy_nix<cat::iword>;

// Syscall 45. Linux names this `recvfrom`. The libCat name follows the BSD
// `recv` family for backward source compatibility with earlier libCat code.
auto
sys_recv(file_descriptor socket_descriptor, void* _Nonnull p_message_buffer,
         cat::iword buffer_length,
         cat::Socket const* _Nullable __restrict p_addr = nullptr,
         cat::iword const* _Nullable __restrict p_addr_length = nullptr)
   -> scaredy_nix<cat::iword>;

// `msg_header` is the kernel ABI for `sys_sendmsg()` and `sys_recvmsg()`.
// Total size is 56 bytes on x86-64.
struct msg_header {
   void* _Nullable p_name;
   cat::uint4 name_length;

 private:
   [[maybe_unused]]
   cat::uint4 m_padding_0;

 public:
   io_vector* _Nullable p_io_vectors;
   cat::uword io_vectors_count;
   void* _Nullable p_control;
   cat::uword control_length;
   cat::int4 flags;

 private:
   [[maybe_unused]]
   cat::int4 m_padding_1;
};

static_assert(sizeof(msg_header) == 56);

// Syscall 46. Returns the number of bytes sent.
auto
sys_sendmsg(file_descriptor socket_descriptor, msg_header const& message,
            cat::int4 flags) -> scaredy_nix<cat::iword>;

// Syscall 47. Returns the number of bytes received.
auto
sys_recvmsg(file_descriptor socket_descriptor, msg_header& message,
            cat::int4 flags) -> scaredy_nix<cat::iword>;

// Syscall 48. Tear down half or all of a connected socket.
auto
sys_shutdown(file_descriptor socket_descriptor, shutdown_how how)
   -> scaredy_nix<void>;

// Syscall 49.
auto
sys_bind(file_descriptor socket_descriptor, void const* _Nonnull p_socket,
         cat::iword p_addr_len) -> scaredy_nix<void>;

// Syscall 50.
auto
sys_listen(file_descriptor socket_descriptor, cat::int8 backlog)
   -> scaredy_nix<void>;

// Syscall 51. Fill `out_socket` with the local address `socket_descriptor` is
// bound to. `inout_addr_length` is in/out. On input it bounds the buffer, on
// output it holds the actual size.
auto
sys_getsockname(file_descriptor socket_descriptor, cat::Socket& out_socket,
                cat::iword& inout_addr_length) -> scaredy_nix<void>;

// Syscall 52. Fill `out_socket` with the address of the connected peer.
auto
sys_getpeername(file_descriptor socket_descriptor, cat::Socket& out_socket,
                cat::iword& inout_addr_length) -> scaredy_nix<void>;

// Syscall 53. Create a connected pair of sockets and write the file
// descriptors into `socket_vector[0]` and `socket_vector[1]`.
auto
sys_socketpair(cat::int8 protocol_family, cat::int8 type, cat::int8 protocol,
               cat::int4 (&socket_vector)[2]) -> scaredy_nix<void>;

// Syscall 54.
auto
sys_setsockopt(file_descriptor socket_descriptor, cat::int4 level,
               cat::int4 option_name, void const* _Nonnull p_option_value,
               cat::int4 option_length) -> scaredy_nix<void>;

// Syscall 55. `inout_option_length` is in/out. On input it bounds the buffer,
// on output it holds the actual size written to `p_option_value`.
auto
sys_getsockopt(file_descriptor socket_descriptor, cat::int4 level,
               cat::int4 option_name, void* _Nonnull p_option_value,
               cat::int4& inout_option_length) -> scaredy_nix<void>;

// Syscall 56 (`sys_clone()`) is not provided because it is highly sensitive to
// function inlining. Instead, the `nix::process` abstraction should be used.

// Syscall 60. Terminate the calling thread with `status` as the exit code.
// Use `sys_exit_group()` to terminate the entire thread group.
[[noreturn]]
void
sys_exit(cat::int4 status);

// Syscall 61. Pass `nullptr` for `p_status_output` to discard status, or for
// `p_resource_usage` to discard resource usage.
auto
sys_wait4(process_id waiting_on_id, cat::int4* _Nullable p_status_output,
          wait_options_flags options, void* _Nullable p_resource_usage)
   -> scaredy_nix<process_id>;

// Syscall 63.
auto
sys_uname(utsname& out) -> scaredy_nix<void>;

// Syscall 72. The third argument is command-specific. Pass `nullptr` when
// the command does not consume one.
auto
sys_fcntl(file_descriptor file_descriptor, fcntl_command command,
          cat::no_type_ptr p_argument = nullptr) -> scaredy_nix<cat::iword>;

// Syscall 73.
auto
sys_flock(file_descriptor file_descriptor, flock_op operation)
   -> scaredy_nix<void>;

// Syscall 74. Flush all dirty data and metadata for `file_descriptor`.
auto
sys_fsync(file_descriptor file_descriptor) -> scaredy_nix<void>;

// Syscall 75. Flush only dirty data (skip non-essential metadata).
auto
sys_fdatasync(file_descriptor file_descriptor) -> scaredy_nix<void>;

// Syscall 76.
auto
sys_truncate(char const* _Nonnull p_file_path, cat::iword length)
   -> scaredy_nix<void>;

// Syscall 77.
auto
sys_ftruncate(file_descriptor file_descriptor, cat::iword length)
   -> scaredy_nix<void>;

// Syscall 78. Read directory entries from `file_descriptor` into `p_buffer`.
// Returns the total number of bytes filled. `linux_dirent` records are
// variable-length, advance by `record_length` to reach the next entry. The
// `linux_dirent64` form (`sys_getdents64()`) is preferred on 64-bit.
auto
sys_getdents(file_descriptor file_descriptor, linux_dirent* _Nonnull p_buffer,
             cat::uword length) -> scaredy_nix<cat::idx>;

// Syscall 79. Fill `p_buffer` with the calling process's working directory.
// Returns the number of bytes written, including the terminating NUL.
auto
sys_getcwd(char* _Nonnull p_buffer, cat::uword length) -> scaredy_nix<cat::idx>;

// Syscall 80.
auto
sys_chdir(char const* _Nonnull p_file_path) -> scaredy_nix<void>;

// Syscall 81.
auto
sys_fchdir(file_descriptor file_descriptor) -> scaredy_nix<void>;

// Syscall 82.
auto
sys_rename(char const* _Nonnull __restrict p_old_path,
           char const* _Nonnull __restrict p_new_path) -> scaredy_nix<void>;

// Syscall 83.
auto
sys_mkdir(char const* _Nonnull p_file_path, file_permissions mode)
   -> scaredy_nix<void>;

// Syscall 84.
auto
sys_rmdir(char const* _Nonnull p_file_path) -> scaredy_nix<void>;

// Syscall 85.
auto
sys_creat(char const* _Nonnull p_file_path, open_mode file_mode)
   -> scaredy_nix<file_descriptor>;

// Syscall 86.
auto
sys_link(char const* _Nonnull __restrict p_existing_path,
         char const* _Nonnull __restrict p_new_path) -> scaredy_nix<void>;

// Syscall 87.
auto
sys_unlink(char const* _Nonnull p_path_name) -> scaredy_nix<void>;

// Syscall 88.
auto
sys_symlink(char const* _Nonnull __restrict p_target_path,
            char const* _Nonnull __restrict p_link_path) -> scaredy_nix<void>;

// Syscall 89. Returns the number of bytes written to `p_buffer`. The kernel
// does not append a terminating NUL.
auto
sys_readlink(char const* _Nonnull __restrict p_file_path,
             char* _Nonnull __restrict p_buffer, cat::uword buffer_length)
   -> scaredy_nix<cat::idx>;

// Syscall 90.
auto
sys_chmod(char const* _Nonnull p_file_path, file_permissions mode)
   -> scaredy_nix<void>;

// Syscall 91.
auto
sys_fchmod(file_descriptor file_descriptor, file_permissions mode)
   -> scaredy_nix<void>;

// Syscall 92.
auto
sys_chown(char const* _Nonnull p_file_path, user_id user, group_id group)
   -> scaredy_nix<void>;

// Syscall 93.
auto
sys_fchown(file_descriptor file_descriptor, user_id user, group_id group)
   -> scaredy_nix<void>;

// Syscall 94. `lchown()` does not follow symbolic links.
auto
sys_lchown(char const* _Nonnull p_file_path, user_id user, group_id group)
   -> scaredy_nix<void>;

// Syscall 95. Replace the file mode creation mask with `mask` and return the
// previous mask.
auto
sys_umask(file_permissions mask) -> scaredy_nix<file_permissions>;

// Syscall 97.
enum class rlimit_resource : unsigned int {
   // CPU time in seconds.
   cpu_time_seconds = 0,
   // Maximum file size.
   max_file_size = 1,
   // Maximum data segment size.
   max_data_segment_size = 2,
   // Maximum stack size.
   max_stack_size = 3,
   // Maximum core dump file size.
   max_core_file_size = 4,
   // Maximum resident set size.
   max_resident_set_size = 5,
   // Maximum processes and threads for the real user ID.
   max_processes_and_threads_per_real_user = 6,
   // Maximum open file descriptors.
   max_open_files = 7,
   // Maximum locked-in-memory bytes.
   max_locked_memory = 8,
   // Maximum address space.
   max_address_space = 9,
   // Maximum file locks held.
   max_file_locks = 10,
   // Maximum pending signals.
   max_pending_signals = 11,
   // Maximum bytes in POSIX message queues.
   max_posix_message_queue_bytes = 12,
   // Maximum nice priority raise.
   max_nice_priority_ceiling = 13,
   // Maximum realtime scheduling priority.
   max_realtime_priority = 14,
   // Maximum realtime CPU time before SIGXCPU, microseconds.
   max_realtime_cpu_microseconds = 15,
};

struct rlimit {
   cat::uint8 soft;
   cat::uint8 hard;
};

auto
sys_getrlimit(rlimit_resource resource, rlimit& out_limits)
   -> scaredy_nix<void>;

// Syscall 102.
auto
sys_getuid() -> user_id;

// Syscall 104.
auto
sys_getgid() -> group_id;

// Syscall 105.
auto
sys_setuid(user_id user) -> scaredy_nix<void>;

// Syscall 106.
auto
sys_setgid(group_id group) -> scaredy_nix<void>;

// Syscall 107.
auto
sys_geteuid() -> user_id;

// Syscall 108.
auto
sys_getegid() -> group_id;

// Syscall 109. Set the process group of `pid` to `process_group`. Pass
// `process_id{0}` for `pid` to mean "the calling process".
auto
sys_setpgid(process_id pid, process_id process_group) -> scaredy_nix<void>;

// Syscall 110.
auto
sys_getppid() -> process_id;

// Syscall 111.
auto
sys_getpgrp() -> process_id;

// Syscall 112. Create a new session and process group with the calling
// thread as the leader. Returns the new session id.
auto
sys_setsid() -> scaredy_nix<process_id>;

// Syscall 113.
auto
sys_setreuid(user_id real, user_id effective) -> scaredy_nix<void>;

// Syscall 114.
auto
sys_setregid(group_id real, group_id effective) -> scaredy_nix<void>;

// Syscall 115. Fill `list` with the calling process's supplementary group
// ids and return the number written. If `list` is empty, the count is
// returned without writing anything.
auto
sys_getgroups(cat::span<group_id> list) -> scaredy_nix<cat::idx>;

// Syscall 116. Replace the calling process's supplementary group list
// with `list`.
auto
sys_setgroups(cat::span<group_id const> list) -> scaredy_nix<void>;

// Syscall 117.
auto
sys_setresuid(user_id real, user_id effective, user_id saved)
   -> scaredy_nix<void>;

// Syscall 118. All three output slots are required.
auto
sys_getresuid(user_id& real, user_id& effective, user_id& saved)
   -> scaredy_nix<void>;

// Syscall 119.
auto
sys_setresgid(group_id real, group_id effective, group_id saved)
   -> scaredy_nix<void>;

// Syscall 120. All three output slots are required.
auto
sys_getresgid(group_id& real, group_id& effective, group_id& saved)
   -> scaredy_nix<void>;

// Syscall 121. Pass `process_id{0}` to query the calling process.
auto
sys_getpgid(process_id pid) -> scaredy_nix<process_id>;

// Syscall 122. Set the filesystem user id used for filesystem checks and
// return the previous value. Pass `user_id{-1}` to query without changing.
auto
sys_setfsuid(user_id user) -> user_id;

// Syscall 123. Set the filesystem group id used for filesystem checks and
// return the previous value. Pass `group_id{-1}` to query without changing.
auto
sys_setfsgid(group_id group) -> group_id;

// Syscall 124. Pass `process_id{0}` to query the calling process.
auto
sys_getsid(process_id pid) -> scaredy_nix<process_id>;

// Syscall 127. Fill `out` with the set of signals pending delivery to the
// calling thread.
auto
sys_rt_sigpending(signals_mask_set& out) -> scaredy_nix<void>;

// Syscall 128. Block until one of the signals in `set` is pending, then
// dequeue it and return its `signal_number`. `p_timeout == nullptr` waits
// indefinitely. `p_info == nullptr` discards the queued payload.
auto
sys_rt_sigtimedwait(signals_mask_set const& set, signal_info* _Nullable p_info,
                    futex_timespec const* _Nullable p_timeout)
   -> scaredy_nix<signal>;

// Syscall 129. Send `s` to `pid` along with a queued `info` payload.
auto
sys_rt_sigqueueinfo(process_id pid, signal s, signal_info const& info)
   -> scaredy_nix<void>;

// Syscall 130. Atomically replace the signal mask with `new_mask` and wait
// for an unmasked signal. Always returns `linux_error::intr` on resume.
auto
sys_rt_sigsuspend(signals_mask_set const& new_mask) -> scaredy_nix<void>;

// Syscall 131. Install a new alternate signal stack and/or query the
// current one. Either pointer may be `nullptr`.
auto
sys_sigaltstack(signal_stack const* _Nullable __restrict p_new_stack,
                signal_stack* _Nullable __restrict p_old_stack)
   -> scaredy_nix<void>;

// Syscall 132. Update the access and modification times of `p_file_path`.
// Pass `nullptr` to set both timestamps to the current time.
auto
sys_utime(char const* _Nonnull p_file_path, utimbuf const* _Nullable p_times)
   -> scaredy_nix<void>;

// Syscall 137. Fill `out` with statistics for the file system that
// contains `p_file_path`.
auto
sys_statfs(char const* _Nonnull p_file_path, statfs_data& out)
   -> scaredy_nix<void>;

// Syscall 138.
auto
sys_fstatfs(file_descriptor file_descriptor, statfs_data& out)
   -> scaredy_nix<void>;

// Syscall 149. Lock the page-aligned range
// [`p_address`, `p_address + length`) into RAM so the kernel may not page
// it out.
auto
sys_mlock(void const* _Nonnull p_address, cat::uword length)
   -> scaredy_nix<void>;

// Syscall 150.
auto
sys_munlock(void const* _Nonnull p_address, cat::uword length)
   -> scaredy_nix<void>;

// Syscall 151. Lock all pages currently mapped (`mlockall_flags::current`),
// all future mappings (`mlockall_flags::future`), or both. `on_fault` is a
// Linux 4.4+ extension.
auto
sys_mlockall(mlockall_flags flags) -> scaredy_nix<void>;

// Syscall 152. Reverse the effect of `sys_mlockall()`.
auto
sys_munlockall() -> scaredy_nix<void>;

// Syscall 160.
auto
sys_setrlimit(rlimit_resource resource, rlimit const& limits)
   -> scaredy_nix<void>;

// Syscall 186.
auto
sys_gettid() -> process_id;

// Syscall 200.
auto
sys_tkill(process_id pid, signal signal) -> scaredy_nix<void>;

struct futex_waitv {
   cat::uint8 expected_value;
   futex_word* _Nonnull user_space_address;
   futex_wait_flags wait_flags;
   cat::uint4 kernel_reserved;
};

// Matches Linux `struct robust_list`/`struct robust_list_head`.
struct robust_list {
   robust_list* _Nullable p_next;
};

struct robust_list_head {
   robust_list head;
   cat::iword futex_offset;
   robust_list* _Nullable p_list_op_pending;
};

inline constexpr cat::idx futex_waitv_max_count = 128u;

struct futex_op;

// Syscall 202. `p_second_futex` may be `nullptr` for ops that don't use it
// (the `wake`/`wait` ops). The primary `futex` operand is always required.
auto
// `p_timeout == nullptr` waits forever. `p_second_futex` is only consulted
// by ops that touch a second word (`requeue`, `cmp_requeue`, `wake_op`).
sys_futex(futex_word& futex, futex_op operation, cat::uint4 value,
          futex_timespec const* _Nullable p_timeout,
          futex_word* _Nullable p_second_futex, cat::uint4 value3)
   -> scaredy_nix<cat::idx>;

auto
sys_futex(futex_word& futex, futex_op operation, cat::uint4 value,
          cat::uint4 value2, futex_word* _Nullable p_second_futex,
          cat::uint4 value3) -> scaredy_nix<cat::idx>;

// Syscall 203. Pin thread `pid` (or the calling thread when `pid` is
// `process_id{0}`) to the CPUs whose bits are set in `mask`. The kernel
// reads `mask.size_bytes()` bytes. Round it up to whatever the host's
// configured CPU count requires.
auto
sys_sched_setaffinity(process_id pid, cat::span<cat::uword const> mask)
   -> scaredy_nix<void>;

// Syscall 204. Fill `mask` with the CPUs thread `pid` (or the caller when
// `pid` is `process_id{0}`) is permitted to run on. Returns the number of
// bytes the kernel actually wrote.
auto
sys_sched_getaffinity(process_id pid, cat::span<cat::uword> mask)
   -> scaredy_nix<cat::idx>;

// Syscall 217. The 64-bit form of `sys_getdents()`. Each `linux_dirent64`
// record carries a `file_type`, unlike the legacy form. Returns the total
// number of bytes filled into `p_buffer`.
auto
sys_getdents64(file_descriptor file_descriptor,
               linux_dirent64* _Nonnull p_buffer, cat::uword length)
   -> scaredy_nix<cat::idx>;

// Syscall 218. Stores the current thread id into `tid` and arranges for the
// kernel to clear it at thread exit. Returns the calling thread's id.
auto
sys_set_tid_address(cat::int4& tid) -> process_id;

// Syscall 231. Terminate every thread in the calling thread group with
// `status` as the exit code.
[[noreturn]]
void
sys_exit_group(cat::int4 status);

// Syscall 234.
auto
sys_tgkill(process_id thread_group_id, process_id target, signal delivered)
   -> scaredy_nix<void>;

// Syscall 247.
auto
sys_waitid(wait_id type, process_id id, wait_options_flags options)
   -> scaredy_nix<process_id>;

// Syscall 257. `dirfd` may be `at_fdcwd` or any other directory descriptor.
// `mode` is honored only when `flags` includes `open_flags::create`.
auto
sys_openat(file_descriptor dirfd, char const* _Nonnull p_file_path,
           open_mode mode, open_flags flags = open_flags(0),
           file_permissions permissions = file_permissions::none)
   -> scaredy_nix<file_descriptor>;

// Syscall 258.
auto
sys_mkdirat(file_descriptor dirfd, char const* _Nonnull p_file_path,
            file_permissions mode) -> scaredy_nix<void>;

// Syscall 260. `flags` accepts `atfile_flags::no_follow` and
// `atfile_flags::empty_path`.
auto
sys_fchownat(file_descriptor dirfd, char const* _Nonnull p_file_path,
             user_id user, group_id group,
             atfile_flags flags = atfile_flags::none) -> scaredy_nix<void>;

// Syscall 262. The kernel name is `newfstatat`. Resolves `p_file_path`
// relative to `dirfd` (or `at_fdcwd`) and fills `out`.
auto
sys_newfstatat(file_descriptor dirfd, char const* _Nonnull p_file_path,
               file_status& out, atfile_flags flags = atfile_flags::none)
   -> scaredy_nix<void>;

// Syscall 263. `flags` accepts `atfile_flags::remove_directory` to behave
// like `sys_rmdir()`.
auto
sys_unlinkat(file_descriptor dirfd, char const* _Nonnull p_file_path,
             atfile_flags flags = atfile_flags::none) -> scaredy_nix<void>;

// Syscall 264.
auto
sys_renameat(file_descriptor old_dirfd,
             char const* _Nonnull __restrict p_old_path,
             file_descriptor new_dirfd,
             char const* _Nonnull __restrict p_new_path) -> scaredy_nix<void>;

// Syscall 265.
auto
sys_linkat(file_descriptor old_dirfd,
           char const* _Nonnull __restrict p_existing_path,
           file_descriptor new_dirfd,
           char const* _Nonnull __restrict p_new_path,
           atfile_flags flags = atfile_flags::none) -> scaredy_nix<void>;

// Syscall 266.
auto
sys_symlinkat(char const* _Nonnull __restrict p_target_path,
              file_descriptor new_dirfd,
              char const* _Nonnull __restrict p_link_path) -> scaredy_nix<void>;

// Syscall 267.
auto
sys_readlinkat(file_descriptor dirfd,
               char const* _Nonnull __restrict p_file_path,
               char* _Nonnull __restrict p_buffer, cat::uword buffer_length)
   -> scaredy_nix<cat::idx>;

// Syscall 268.
auto
sys_fchmodat(file_descriptor dirfd, char const* _Nonnull p_file_path,
             file_permissions mode, atfile_flags flags = atfile_flags::none)
   -> scaredy_nix<void>;

// Syscall 269. `flags` accepts `atfile_flags::eaccess` (effective uid/gid
// instead of real) and `atfile_flags::no_follow`.
auto
sys_faccessat(file_descriptor dirfd, char const* _Nonnull p_file_path,
              access_mode mode, atfile_flags flags = atfile_flags::none)
   -> scaredy_nix<void>;

// Syscall 273.
auto
sys_set_robust_list(robust_list_head& head, cat::uword length_bytes)
   -> scaredy_nix<void>;

// Syscall 274. The output slots are required. The returned robust-list head
// pointer itself may be `nullptr` when the target has no robust list.
auto
sys_get_robust_list(process_id target, robust_list_head* _Nullable& out_head,
                    cat::idx& out_length_bytes) -> scaredy_nix<void>;

// Syscall 280. Update access and modification times. `p_times` is a 2-element
// array (`{access, modification}`) or `nullptr` to set both to the current
// time. `p_file_path` may be `nullptr` if `dirfd` itself is the target.
auto
sys_utimensat(file_descriptor dirfd, char const* _Nullable p_file_path,
              futex_timespec const (*_Nullable p_times)[2],
              atfile_flags flags = atfile_flags::none) -> scaredy_nix<void>;

// Syscall 285. Manipulate the on-disk allocation of `file_descriptor` over
// the byte range [`offset`, `offset + length`).
auto
sys_fallocate(file_descriptor file_descriptor, fallocate_flags mode,
              cat::iword offset, cat::iword length) -> scaredy_nix<void>;

// Syscall 288. `sys_accept()` plus `accept4_flags::nonblocking` /
// `accept4_flags::close_exec`. Pass `nullptr` for `p_socket` and
// `p_addr_len` to accept without retrieving the peer address.
auto
sys_accept4(file_descriptor socket_descriptor,
            cat::Socket* _Nullable __restrict p_socket,
            cat::iword* _Nullable __restrict p_addr_len, accept4_flags flags)
   -> scaredy_nix<file_descriptor>;

// Syscall 292. `sys_dup2()` plus `dup3_flags::close_exec`.
auto
sys_dup3(file_descriptor oldfd, file_descriptor newfd, dup3_flags flags)
   -> scaredy_nix<file_descriptor>;

// Syscall 293.
auto sys_pipe2(cat::int4 (&pipefd)[2], pipe2_flags flags) -> scaredy_nix<void>;

// Syscall 297. Send `s` to thread `tid` inside thread group `tgid` along
// with a queued `info` payload.
auto
sys_rt_tgsigqueueinfo(process_id tgid, process_id tid, signal s,
                      signal_info const& info) -> scaredy_nix<void>;

// Syscall 316. `sys_renameat()` plus `renameat2_flags`.
auto
sys_renameat2(file_descriptor old_dirfd,
              char const* _Nonnull __restrict p_old_path,
              file_descriptor new_dirfd,
              char const* _Nonnull __restrict p_new_path, renameat2_flags flags)
   -> scaredy_nix<void>;

// Syscall 318. Fill `length` bytes of `p_buffer` with kernel-supplied
// randomness. Returns the number of bytes actually written.
auto
sys_getrandom(void* _Nonnull p_buffer, cat::iword length,
              getrandom_flags flags = getrandom_flags::none)
   -> scaredy_nix<cat::iword>;

// Syscall 325. `sys_mlock()` plus `mlock2_flags::on_fault`.
auto
sys_mlock2(void const* _Nonnull p_address, cat::uword length,
           mlock2_flags flags) -> scaredy_nix<void>;

// Syscall 332. Extended `stat`. `mask` selects which fields the kernel must
// fill in `out`. `flags` may include `atfile_flags::no_follow` and the
// statx-only sync controls.
auto
sys_statx(file_descriptor dirfd, char const* _Nonnull p_file_path,
          atfile_flags flags, statx_mask mask, statx_data& out)
   -> scaredy_nix<void>;

// `io_sqring_offsets` is the kernel ABI for the submission-queue offsets
// that `sys_io_uring_setup()` fills into `io_uring_params::sq_off`.
struct io_sqring_offsets {
   cat::uint4 head;
   cat::uint4 tail;
   cat::uint4 ring_mask;
   cat::uint4 ring_entries;
   cat::uint4 flags;
   cat::uint4 dropped;
   cat::uint4 array;
   cat::uint4 reserved_1;
   cat::uint8 user_address;
};

static_assert(sizeof(io_sqring_offsets) == 40);

// `io_cqring_offsets` is the kernel ABI for the completion-queue offsets
// that `sys_io_uring_setup()` fills into `io_uring_params::cq_off`.
struct io_cqring_offsets {
   cat::uint4 head;
   cat::uint4 tail;
   cat::uint4 ring_mask;
   cat::uint4 ring_entries;
   cat::uint4 overflow;
   cat::uint4 cqes;
   cat::uint4 flags;
   cat::uint4 reserved_1;
   cat::uint8 user_address;
};

static_assert(sizeof(io_cqring_offsets) == 40);

// `io_uring_params` is the kernel ABI for `sys_io_uring_setup()`. The caller
// fills `flags`, `sq_thread_*`, and (when `completion_size` is set in
// `flags`) `cq_entries`. The kernel returns the actual ring sizing,
// `features`, and the two offset blocks.
struct io_uring_params {
   cat::uint4 sq_entries;
   cat::uint4 cq_entries;
   io_uring_setup_flags flags;
   cat::uint4 sq_thread_cpu;
   cat::uint4 sq_thread_idle;
   cat::uint4 features;
   cat::uint4 wq_fd;

 private:
   [[maybe_unused]]
   cat::uint4 m_reserved[3];

 public:
   io_sqring_offsets sq_off;
   io_cqring_offsets cq_off;
};

static_assert(sizeof(io_uring_params) == 120);

// Syscall 425 (Linux 5.1). Set up an io_uring instance with `entries` SQEs
// and the parameters in `params`. Returns the ring file descriptor on
// success. The io_uring family can be runtime-disabled via
// `/proc/sys/kernel/io_uring_disabled`. Query `has_sys_io_uring()` before
// relying on it.
auto
sys_io_uring_setup(cat::uint4 entries, io_uring_params& params)
   -> scaredy_nix<file_descriptor>;

// Syscall 426 (Linux 5.1). Submit `to_submit` SQEs and/or wait for
// `min_complete` CQEs on `ring`. When `flags` includes
// `io_uring_enter_flags::ext_arg`, `p_extended_argument` points at an
// `io_uring_getevents_arg`-shaped struct of `extended_argument_size`
// bytes. Otherwise `p_extended_argument` is interpreted as a `sigset_t`.
// Passing `nullptr` means no extended argument or signal mask is supplied.
auto
sys_io_uring_enter(file_descriptor ring, cat::uint4 to_submit,
                   cat::uint4 min_complete, io_uring_enter_flags flags,
                   void const* _Nullable p_extended_argument = nullptr,
                   cat::uword extended_argument_size = 0u)
   -> scaredy_nix<cat::idx>;

// Syscall 427 (Linux 5.1). Register or unregister resources with `ring`.
// `op` selects the operation. `p_arg` and `nr_args` are op-specific. Some
// operations ignore `p_arg`, so `nullptr` is forwarded unchanged.
auto
sys_io_uring_register(file_descriptor ring, io_uring_register_op op,
                      void* _Nullable p_arg, cat::uint4 nr_args)
   -> scaredy_nix<cat::idx>;

// Runtime probe for the whole io_uring family (setup / enter / register).
// The kernel enables and disables them as a unit, so one query suffices.
[[nodiscard, gnu::always_inline]]
inline auto
has_sys_io_uring() -> bool {
   return detail::has_sys_io_uring_cache;
}

// Syscall 437. Extended `openat`. The kernel ABI takes a `sizeof(open_how)`
// argument so it can be extended with new fields on a kernel-version basis.
auto
sys_openat2(file_descriptor dirfd, char const* _Nonnull p_file_path,
            open_how const& how) -> scaredy_nix<file_descriptor>;

// Syscall 449.
auto
sys_futex_waitv(cat::span<futex_waitv const> waiters,
                futex_waitv_call_flags call_flags,
                futex_timespec const* _Nullable p_timeout, cat::int4 clock_id)
   -> scaredy_nix<cat::iword>;

// `cachestat_range` describes the [`offset`, `offset + length`) byte range
// queried by `sys_cachestat()`.
struct cachestat_range {
   cat::uint8 offset;
   cat::uint8 length;
};

// `cachestat_stats` is filled by `sys_cachestat()` with page counts for the
// queried range.
struct cachestat_stats {
   cat::uint8 nr_cache;
   cat::uint8 nr_dirty;
   cat::uint8 nr_writeback;
   cat::uint8 nr_evicted;
   cat::uint8 nr_recently_evicted;
};

// Syscall 451 (Linux 6.5).
auto
sys_cachestat(file_descriptor file_descriptor, cachestat_range const& range,
              cachestat_stats& out, cat::uint4 flags = 0) -> scaredy_nix<void>;

[[nodiscard, gnu::always_inline]]
inline auto
has_sys_cachestat() -> bool {
   return detail::has_sys_cachestat_cache;
}

// Syscall 452 (Linux 6.6).
auto
sys_fchmodat2(file_descriptor dirfd, char const* _Nonnull p_path,
              cat::uint4 mode, cat::int4 flags) -> scaredy_nix<void>;

[[nodiscard, gnu::always_inline]]
inline auto
has_sys_fchmodat2() -> bool {
   return detail::has_sys_fchmodat2_cache;
}

// Syscall 453 (Linux 6.6).
auto
sys_map_shadow_stack(cat::uword address, cat::uword size, cat::uint4 flags)
   -> scaredy_nix<void* _Nullable>;

[[nodiscard, gnu::always_inline]]
inline auto
has_sys_map_shadow_stack() -> bool {
   return detail::has_sys_map_shadow_stack_cache;
}

// Syscall 454 (Linux 6.7). Wakes up to `nr` waiters parked on `uaddr` whose
// bitset intersects `mask`. Returns the number of waiters woken.
auto
sys_futex_wake(futex_word& uaddr, cat::uword mask, cat::int4 nr,
               cat::uint4 flags) -> scaredy_nix<cat::idx>;

[[nodiscard, gnu::always_inline]]
inline auto
has_sys_futex_wake() -> bool {
   return detail::has_sys_futex_wake_cache;
}

// Syscall 455 (Linux 6.7). Park on `uaddr` while it equals `value` and the
// caller's bitset intersects `mask`. `p_timeout == nullptr` waits forever.
auto
sys_futex_wait(futex_word& uaddr, cat::uword value, cat::uword mask,
               cat::uint4 flags, futex_timespec const* _Nullable p_timeout,
               cat::int4 clock_id) -> scaredy_nix<void>;

[[nodiscard, gnu::always_inline]]
inline auto
has_sys_futex_wait() -> bool {
   return detail::has_sys_futex_wait_cache;
}

// Syscall 456 (Linux 6.7). Wake up to `nr_wake` waiters on the first
// waitv-described futex and requeue up to `nr_requeue` of the remaining
// waiters onto the second.
auto
sys_futex_requeue(cat::span<futex_waitv const> waiters, cat::uint4 flags,
                  cat::int4 nr_wake, cat::int4 nr_requeue)
   -> scaredy_nix<cat::idx>;

[[nodiscard, gnu::always_inline]]
inline auto
has_sys_futex_requeue() -> bool {
   return detail::has_sys_futex_requeue_cache;
}

// Syscall 462 (Linux 6.10). Seal the page-aligned range
// [`p_address`, `p_address + length`) so its mapping cannot be modified or
// unmapped.
auto
sys_mseal(void* _Nonnull p_address, cat::uword length, cat::uword flags = 0u)
   -> scaredy_nix<void>;

[[nodiscard, gnu::always_inline]]
inline auto
has_sys_mseal() -> bool {
   return detail::has_sys_mseal_cache;
}

auto
wait_pid(process_id pid, file_status* _Nonnull p_file_status,
         wait_options_flags options) -> scaredy_nix<process_id>;

// Create and return a `cat::SocketLocal` (also known as Unix socket).
auto
create_socket_local(cat::int8 type, cat::int8 protocol)
   -> scaredy_nix<file_descriptor>;

// `tty_io_serial` handles a configuration of tty IO, often referred to as
// termios.
struct tty_io_serial {
   tty_configuration_flags input_flags;
   tty_configuration_flags output_flags;
   tty_configuration_flags control_flags;
   tty_configuration_flags local_flags;

 private:
   // This is for backwards compatibilty with non-serial termio syscalls.
   [[maybe_unused]]
   char m_line_characters;

 public:
   char special_characters[32];

 private:
   // This are for backwards compatibilty with non-serial termio syscalls.
   [[maybe_unused]]
   unsigned int m_input_speed;
   [[maybe_unused]]
   unsigned int m_output_speed;
};

struct tty_window_size {
   cat::uint2 rows, columns, width_pixels, height_pixels;
};

auto
is_a_tty(file_descriptor file_descriptor) -> scaredy_nix<void>;

auto is_a_tty(tty_descriptor) -> scaredy_nix<void>;

auto
tty_get_attributes(file_descriptor tty)
   -> cat::scaredy<tty_io_serial, linux_error>;

enum class tty_set_mode : unsigned short {
   // Update the tty immediately.
   now = 0x5402,
   // Update the tty once all currently written data has been transmitted.
   wait_write = 0x5403,
   // Update the tty once all currently written data has been transmitted, and
   // all recieved data has been read.
   wait_recieve = 0x5404,
};

auto
tty_set_attributes(file_descriptor tty, tty_set_mode tty_mode,
                   tty_io_serial const& configuration) -> scaredy_nix<void>;

auto
read_char() -> scaredy_nix<char>;

// TODO: Extract this to an implementation file.
struct file_status {
 public:
   [[nodiscard]]
   auto
   is_regular() const -> bool {
      return (this->protections_mode & 0170000u) == 0100000u;
   }

   static_assert(!cat::enum_flag_trait<cat::uint4>::value);

   [[nodiscard]]
   auto
   is_directory() const -> bool {
      return (this->protections_mode & 0170000u) == 0040000u;
   }

   [[nodiscard]]
   auto
   is_character_device() const -> bool {
      return (this->protections_mode & 0170000u) == 0020000u;
   }

   [[nodiscard]]
   auto
   is_block_device() const -> bool {
      return (this->protections_mode & 0170000u) == 0060000u;
   }

   [[nodiscard]]
   auto
   is_fifo() const -> bool {
      return (this->protections_mode & 0170000u) == 0010000u;
   }

   [[nodiscard]]
   auto
   is_symbolic_link() const -> bool {
      return (this->protections_mode & 0170000u) == 0120000u;
   }

   [[nodiscard]]
   auto
   is_socket() const -> bool {
      return (this->protections_mode & 0170000u) == 0140000u;
   }

   cat::uint8 device_id;
   // The index node for this file.
   cat::uint8 inode;
   cat::uword hard_links_count;

   cat::uint4 protections_mode;
   user_id user;
   group_id groud;

 private:
   [[maybe_unused]]
   cat::uint4 _;

 public:
   cat::uint8 rdev;
   cat::idx file_size;
   cat::idx block_size;
   cat::idx blocks_count;

   using time_spec = struct {
      cat::iword seconds;
      cat::iword nanoseconds;
   };

   time_spec last_access_time;
   time_spec last_modification_time;
   time_spec creation_time;

 private:
   // The kernel's trailing `long __unused[3]` (24 bytes on x86-64), not 4-byte
   // ints. A 12-byte deficit here lets `stat`/`fstat` overrun callers' stacks.
   [[maybe_unused]]
   cat::iword _[3];
};

static_assert(sizeof(file_status) == 144);

// `process` handles an asynchronous task multitasked by the Linux kernel.
// TODO: Extract this to an implementation file.
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
         static_cast<unsigned char>(signal::child_stopped)));

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
      m_flags = static_cast<clone_flags>(cat::to_underlying(m_flags)
                                         | cat::to_underlying(extra));
   }

   template <typename... Args, cat::is_invocable<Args...> Callback>
   auto
   spawn(cat::is_allocator auto& allocator, cat::idx initial_stack_size,
         Callback&& callback, Args&&... arguments) -> scaredy_nix<void>;

   [[nodiscard]]
   auto
   wait() const -> scaredy_nix<process_id>;

 private:
   auto
   spawn_impl(cat::uintptr<void> stack, cat::idx initial_stack_size,
              void* _Nonnull p_function, void* _Nonnull p_args_struct)
      -> scaredy_nix<void>;

   process_id m_id{0};
   // `clone_flags::child_clear_tid` must not use `m_id` as the clear-tid word.
   // The kernel stores the child tid here, clears it to zero at thread exit,
   // and wakes waiters with `futex_command::wake` and `futex_options::none`.
   // `wait()` uses `futex_command::wait` with `futex_options::none` on this
   // word when `clone_flags::thread` and `clone_flags::child_set_tid` are set.
   alignas(8) futex_word m_clone_child_clear_tid_for_kernel{};
   // TODO: Free this later.
   void* _Nullable m_p_stack_bottom;
   cat::idx m_stack_size;
   clone_flags m_flags = default_flags;
};

auto
block_all_signals() -> signals_mask_set;

auto
raise(signal signal, process_id pid) -> scaredy_nix<void>;

auto
raise_here(signal signal) -> scaredy_nix<void>;

// `arch_prctl_request::set_fs_base`/`get_fs_base` for x86-64. A custom
// main-thread TLS block can be installed by copying the executable TLS image
// with `detail::install_executable_tls_image_at_thread_pointer` then calling
// `sys_arch_prctl(arch_prctl_request::set_fs_base, thread_pointer.get())`.
enum class arch_prctl_request : int {
   set_fs_base = 0x1002,
   get_fs_base = 0x1003,
};

namespace detail {
// Minimum bytes reserved above the stack for `clone_flags::set_tls` TLS setup.
// Covers `p_memsz`, thread-pointer alignment, and a 64-byte line round-up.
[[nodiscard]]
auto
clone_thread_local_buffer_min_bytes() -> cat::idx;

void
install_executable_tls_image_at_thread_pointer(
   cat::uintptr<void> p_process_address);

void
init_parent_process_tls();
}  // namespace detail

[[nodiscard]]
auto
sys_arch_prctl(arch_prctl_request request, void* _Nonnull p_address)
   -> scaredy_nix<void>;

// Fields of `/proc/self/statm` in page units.
struct self_statm {
   cat::uword total_pages;
   cat::uword resident_pages;
   cat::uword shared_pages;
   cat::uword text_pages;
   cat::uword _;  // Unused since Linux 2.6.
   cat::uword data_pages;
};

// Read every field of `/proc/self/statm`. Propagates the originating
// `linux_error` if the file cannot be opened or read. The result is
// process-wide and is not idempotent: consecutive calls may disagree as
// the kernel updates its accounting in response to allocations elsewhere
// in the process.
[[nodiscard]]
auto
read_self_statm() -> cat::scaredy<self_statm, nix::linux_error>;

}  // namespace nix

#include "implementations/futex.tpp"
#include "implementations/process.tpp"
