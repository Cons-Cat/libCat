#include <cat/linux>
#include <cat/socket>

#include "../unit_tests.hpp"

// Smoke tests for every libCat Linux syscall wrapper. The goal is to invoke
// each `sys_*` at least once and verify it either succeeds or returns the
// expected `linux_error` for the calling process's privilege level. A few
// syscalls cannot be exercised directly (`sys_exit`, `sys_exit_group`,
// `sys_execve`-class destructors, blocking ones like `sys_rt_sigsuspend`)
// and are documented inline rather than tested.

namespace {
// All tests run in a single binary so each `$test` gets a unique path. If a
// prior run crashed and left a stale file behind, the test's first step is
// always to remove it and ignore the result.
//
// Paths are `cat::zstr_view` because the string-literal constructor deduces
// the length at compile time and keeps the trailing null for syscall ABIs.
inline constexpr cat::zstr_view tmp_basic = "/tmp/libcat_test_basic";
inline constexpr cat::zstr_view tmp_link_target =
   "/tmp/libcat_test_link_target";
inline constexpr cat::zstr_view tmp_link = "/tmp/libcat_test_link";
inline constexpr cat::zstr_view tmp_symlink = "/tmp/libcat_test_symlink";
inline constexpr cat::zstr_view tmp_rename_a = "/tmp/libcat_test_rename_a";
inline constexpr cat::zstr_view tmp_rename_b = "/tmp/libcat_test_rename_b";
inline constexpr cat::zstr_view tmp_dir = "/tmp/libcat_test_dir";
inline constexpr cat::zstr_view tmp_dir_inside = "/tmp/libcat_test_dir/file";
inline constexpr cat::zstr_view tmp_truncate = "/tmp/libcat_test_truncate";
inline constexpr cat::zstr_view tmp_atfile = "/tmp/libcat_test_atfile";

// Inline payloads used by tests that write data to `tmp_basic`.
inline constexpr cat::zstr_view payload_basic = "libcat syscall test\n";
inline constexpr cat::idx payload_basic_length = 20u;
inline constexpr cat::zstr_view payload_short = "abcd";
inline constexpr cat::idx payload_short_length = 4u;
inline constexpr cat::zstr_view payload_two = "hi";
inline constexpr cat::idx payload_two_length = 2u;
inline constexpr cat::zstr_view payload_ping = "ping";
inline constexpr cat::idx payload_ping_length = 4u;
inline constexpr cat::zstr_view payload_hello = "hello, ";
inline constexpr cat::idx payload_hello_length = 7u;
inline constexpr cat::zstr_view payload_world = "world\n";
inline constexpr cat::idx payload_world_length = 6u;

// `EPERM` and `EACCES` are common when running unprivileged. Treat both as
// "the syscall worked, we just don't have permission".
auto
is_denied_or_invalid(nix::linux_error e) -> bool {
   return e == nix::linux_error::perm || e == nix::linux_error::acces
          || e == nix::linux_error::inval || e == nix::linux_error::nomem
          || e == nix::linux_error::fault;
}
}  // namespace

// Identity / process info.

$test(syscall_identity) {
   nix::process_id pid = nix::sys_getpid();
   cat::verify(pid.value > 0);

   nix::process_id ppid = nix::sys_getppid();
   cat::verify(ppid.value > 0);

   nix::process_id tid = nix::sys_gettid();
   // The main thread's tid equals the pid.
   cat::verify(tid.value == pid.value);

   nix::user_id uid = nix::sys_getuid();
   cat::verify(uid.value >= 0);

   nix::user_id euid = nix::sys_geteuid();
   cat::verify(euid.value >= 0);

   nix::group_id gid = nix::sys_getgid();
   cat::verify(gid.value >= 0);

   nix::group_id egid = nix::sys_getegid();
   cat::verify(egid.value >= 0);

   nix::process_id pgrp = nix::sys_getpgrp();
   cat::verify(pgrp.value > 0);

   nix::process_id pgid = nix::sys_getpgid(nix::process_id{0}).verify();
   cat::verify(pgid.value > 0);

   nix::process_id sid = nix::sys_getsid(nix::process_id{0}).verify();
   cat::verify(sid.value > 0);

   nix::user_id ruid;
   nix::user_id eu;
   nix::user_id su;
   nix::sys_getresuid(ruid, eu, su).verify();
   cat::verify(ruid.value == uid.value);

   nix::group_id rgid;
   nix::group_id eg;
   nix::group_id sg;
   nix::sys_getresgid(rgid, eg, sg).verify();
   cat::verify(rgid.value == gid.value);

   // `getgroups(0, nullptr)` returns the count without filling.
   nix::sys_getgroups({}).verify();
}

// Credentials writes.

$test(syscall_credentials_set) {
   // Every `set*` syscall here changes per-process state. As an unprivileged
   // user setting our own ids back to themselves is a no-op success.
   // Attempting any other change returns `linux_error::perm`.
   nix::user_id uid = nix::sys_getuid();
   nix::group_id gid = nix::sys_getgid();

   nix::sys_setuid(uid).verify();
   nix::sys_setgid(gid).verify();
   nix::sys_setreuid(uid, uid).verify();
   nix::sys_setregid(gid, gid).verify();
   nix::sys_setresuid(uid, uid, uid).verify();
   nix::sys_setresgid(gid, gid, gid).verify();

   // `setfsuid` / `setfsgid` cannot fail. They always return the previous
   // value. Pass `-1` to query without changing.
   nix::user_id prev_fsuid = nix::sys_setfsuid(nix::user_id{-1});
   cat::verify(prev_fsuid.value >= 0);
   nix::group_id prev_fsgid = nix::sys_setfsgid(nix::group_id{-1});
   cat::verify(prev_fsgid.value >= 0);

   // `setpgid(self, self)` puts the calling process into its own group.
   // For a process that's already a group leader this is a no-op.
   nix::process_id pid = nix::sys_getpid();
   auto pgid_result = nix::sys_setpgid(pid, pid);
   cat::verify(pgid_result.has_value()
               || pgid_result.error() == nix::linux_error::perm);

   // `setsid` typically fails because we are already a process group leader.
   auto sid_result = nix::sys_setsid();
   cat::verify(sid_result.has_value()
               || sid_result.error() == nix::linux_error::perm);
}

// uname / kernel version.

$test(syscall_uname) {
   nix::utsname uts{};
   nix::sys_uname(uts).verify();
   cat::verify(uts.sysname[0] != '\0');
   cat::verify(uts.release[0] != '\0');
   cat::verify(uts.machine[0] != '\0');
}

$test(syscall_kernel_version) {
   auto v = nix::get_kernel_version();
   // libCat targets Linux 5+ (reasonable lower bound for a modern dev box).
   cat::verify(v.major >= 5);
}

// File I/O round trip.

$test(syscall_io_basic) {
   auto _ = nix::sys_unlink(tmp_basic);

   nix::file_descriptor fd =
      nix::sys_creat(tmp_basic, nix::open_mode::read_write).verify();
   cat::verify(fd.value >= 0);

   cat::idx written =
      nix::sys_write(fd, payload_basic.data(), payload_basic_length)
         .verify();
   cat::verify(written == payload_basic_length);

   // Rewind via `sys_lseek` and read it back.
   cat::iword pos = nix::sys_lseek(fd, 0, nix::seek_whence::beginning).verify();
   cat::verify(pos == 0);

   char buffer[64] = {};
   cat::iword got = nix::sys_read(fd, buffer, sizeof(buffer)).verify();
   cat::verify(got == cat::iword(payload_basic_length));

   // Positioned read/write don't move the file offset.
   cat::idx written_pwrite =
      nix::sys_pwrite64(fd, payload_short.data(), payload_short_length, 0)
         .verify();
   cat::verify(written_pwrite == payload_short_length);

   char preadbuf[16] = {};
   cat::iword read_pread =
      nix::sys_pread64(fd, preadbuf, sizeof(preadbuf), 0).verify();
   cat::verify(read_pread >= 4);
   cat::verify(preadbuf[0] == 'a');

   nix::sys_fsync(fd).verify();
   nix::sys_fdatasync(fd).verify();

   nix::sys_ftruncate(fd, 0).verify();
   nix::sys_close(fd).verify();
   auto truncate_result = nix::sys_truncate(tmp_basic, 4);
   cat::verify(truncate_result.has_value()
               || is_denied_or_invalid(truncate_result.error()));
   nix::sys_unlink(tmp_basic).verify();
}

// Scatter/gather and offsets.

$test(syscall_io_vector) {
   auto _ = nix::sys_unlink(tmp_basic);
   nix::file_descriptor fd =
      nix::sys_creat(tmp_basic, nix::open_mode::read_write).verify();

   nix::io_vector vecs[2] = {
      nix::io_vector(
         __builtin_bit_cast(cat::byte* _Nonnull, payload_hello.data()),
         payload_hello_length),
      nix::io_vector(
         __builtin_bit_cast(cat::byte* _Nonnull, payload_world.data()),
         payload_world_length),
   };
   cat::idx written =
      nix::sys_writev(fd, vecs, 2u).verify();
   cat::verify(written == 13);

   nix::sys_lseek(fd, 0, nix::seek_whence::beginning).verify();

   char ra[7] = {};
   char rb[6] = {};
   nix::io_vector rvecs[2] = {
      nix::io_vector(reinterpret_cast<cat::byte*>(ra), cat::idx(7)),
      nix::io_vector(reinterpret_cast<cat::byte*>(rb), cat::idx(6)),
   };
   cat::iword got =
      nix::sys_readv(fd, rvecs, 2u).verify();
   cat::verify(got == 13);
   cat::verify(ra[0] == 'h');
   cat::verify(rb[0] == 'w');

   nix::sys_close(fd).verify();
   nix::sys_unlink(tmp_basic).verify();
}

// pipe / dup.

$test(syscall_pipe_dup) {
   cat::int4 fds[2] = {};
   nix::sys_pipe(fds).verify();
   nix::file_descriptor read_end{fds[0]};
   nix::file_descriptor write_end{fds[1]};

   nix::sys_write(write_end, payload_ping.data(), payload_ping_length)
      .verify();
   char buf[8] = {};
   cat::iword got = nix::sys_read(read_end, buf, sizeof(buf)).verify();
   cat::verify(got == 4);

   // `sys_dup` allocates the lowest free fd. `sys_dup2` overwrites a target.
   nix::file_descriptor dup_fd = nix::sys_dup(write_end).verify();
   cat::verify(dup_fd.value > write_end.value);
   nix::sys_close(dup_fd).verify();

   cat::int4 second_pair[2] = {};
   nix::sys_pipe2(second_pair, nix::pipe2_flags::close_exec).verify();
   nix::sys_dup3(nix::file_descriptor{second_pair[1]}, write_end,
                 nix::dup3_flags::none)
      .verify();
   nix::sys_close(nix::file_descriptor{second_pair[0]}).verify();

   nix::sys_close(read_end).verify();
   nix::sys_close(write_end).verify();
}

// fcntl / flock.

$test(syscall_fcntl_flock) {
   auto _ = nix::sys_unlink(tmp_basic);
   nix::file_descriptor fd =
      nix::sys_creat(tmp_basic, nix::open_mode::read_write).verify();

   // Read the descriptor flags back. The set side requires packing a bit into
   // the syscall's third register, which is awkward to express via
   // `cat::no_type_ptr` here. Just exercise the get-side dispatch.
   cat::iword flags =
      nix::sys_fcntl(fd, nix::fcntl_command::get_fd_flags).verify();
   cat::verify(flags >= 0);

   // Advisory exclusive lock then unlock.
   nix::sys_flock(fd, nix::flock_op::exclusive).verify();
   nix::sys_flock(fd, nix::flock_op::unlock).verify();

   nix::sys_close(fd).verify();
   nix::sys_unlink(tmp_basic).verify();
}

// stat family / access / xattr-less metadata.

$test(syscall_fs_metadata) {
   auto _ = nix::sys_unlink(tmp_basic);
   nix::file_descriptor fd =
      nix::sys_creat(tmp_basic, nix::open_mode::read_write).verify();
   nix::sys_write(fd, payload_short.data(), payload_short_length).verify();
   nix::sys_close(fd).verify();
   nix::sys_chmod(tmp_basic, nix::file_permissions::user_read
                                       | nix::file_permissions::user_write)
      .verify();

   nix::file_status st_path{};
   nix::sys_stat(tmp_basic, st_path).verify();
   cat::verify(st_path.is_regular());
   cat::verify(st_path.file_size == 4);

   nix::file_status st_lpath{};
   nix::sys_lstat(tmp_basic, st_lpath).verify();
   cat::verify(st_lpath.is_regular());

   nix::file_status st_at{};
   nix::sys_newfstatat(nix::at_fdcwd, tmp_basic, st_at).verify();
   cat::verify(st_at.is_regular());

   fd = nix::sys_open(tmp_basic, nix::open_mode::read_only).verify();
   nix::file_status st_fstat = nix::sys_fstat(fd).verify();
   cat::verify(st_fstat.is_regular());
   nix::sys_close(fd).verify();

   nix::sys_access(tmp_basic, nix::access_mode::exists).verify();
   nix::sys_access(tmp_basic,
                   nix::access_mode::readable | nix::access_mode::writable)
      .verify();

   nix::sys_faccessat(nix::at_fdcwd, tmp_basic, nix::access_mode::exists)
      .verify();

   // `sys_statx` is Linux 4.11+, present everywhere libCat targets.
   nix::statx_data sx{};
   nix::sys_statx(nix::at_fdcwd, tmp_basic, nix::atfile_flags::none,
                  nix::statx_mask::basic_stats, sx)
      .verify();
   cat::verify(sx.file_size == 4);

   nix::statfs_data fs{};
   nix::sys_statfs(tmp_basic, fs).verify();
   cat::verify(fs.block_size > 0);

   fd = nix::sys_open(tmp_basic, nix::open_mode::read_only).verify();
   nix::statfs_data fs2{};
   nix::sys_fstatfs(fd, fs2).verify();
   cat::verify(fs2.block_size > 0);
   nix::sys_close(fd).verify();

   nix::sys_unlink(tmp_basic).verify();
}

// mode / ownership.

$test(syscall_fs_mode) {
   auto _ = nix::sys_unlink(tmp_basic);
   nix::file_descriptor fd =
      nix::sys_creat(tmp_basic, nix::open_mode::read_write).verify();

   nix::sys_chmod(tmp_basic, nix::file_permissions::user_read
                                       | nix::file_permissions::user_write)
      .verify();
   nix::sys_fchmod(
      fd, nix::file_permissions::user_read | nix::file_permissions::user_write)
      .verify();
   nix::sys_fchmodat(
      nix::at_fdcwd, tmp_basic,
      nix::file_permissions::user_read | nix::file_permissions::user_write)
      .verify();

   // Setting our own uid/gid back on a file we own is always permitted.
   nix::user_id uid = nix::sys_getuid();
   nix::group_id gid = nix::sys_getgid();
   nix::sys_chown(tmp_basic, uid, gid).verify();
   nix::sys_fchown(fd, uid, gid).verify();
   nix::sys_lchown(tmp_basic, uid, gid).verify();
   nix::sys_fchownat(nix::at_fdcwd, tmp_basic, uid, gid).verify();

   // `sys_umask` returns the previous umask. Setting and restoring is a no-op.
   nix::file_permissions previous =
      nix::sys_umask(nix::file_permissions::other_read).verify();
   auto unused_restore = nix::sys_umask(previous);

   nix::sys_close(fd).verify();
   nix::sys_unlink(tmp_basic).verify();
}

// Directories / dirents / cwd.

$test(syscall_fs_directories) {
   // Clean leftover state.
   auto unused_unlink = nix::sys_unlink(tmp_dir_inside);
   auto unused_rmdir_inside = nix::sys_rmdir(tmp_dir_inside);
   auto unused_rmdir = nix::sys_rmdir(tmp_dir);

   nix::sys_mkdir(tmp_dir, nix::file_permissions::user_read
                                     | nix::file_permissions::user_write
                                     | nix::file_permissions::user_execute)
      .verify();
   nix::sys_mkdirat(
      nix::at_fdcwd, tmp_dir_inside,
      nix::file_permissions::user_read | nix::file_permissions::user_write)
      .verify();

   nix::file_descriptor dir_fd =
      nix::sys_open(tmp_dir, nix::open_mode::read_only,
                    nix::open_flags::directory)
         .verify();
   alignas(8) char dirent_buf[1'024] = {};
   cat::idx bytes =
      nix::sys_getdents64(dir_fd,
                          reinterpret_cast<nix::linux_dirent64*>(dirent_buf),
                          sizeof(dirent_buf))
         .verify();
   cat::verify(bytes > 0);
   nix::sys_close(dir_fd).verify();

   // `getcwd` fills a caller buffer with a NUL-terminated path. The kernel
   // returns the buffer length including the terminator on success.
   char cwd[256] = {};
   cat::idx cwd_len = nix::sys_getcwd(cwd, sizeof(cwd)).verify();
   cat::verify(cwd_len > 0);
   cat::verify(cwd[0] == '/');

   nix::sys_chdir(tmp_dir).verify();
   nix::sys_chdir(&cwd[0]).verify();
   nix::file_descriptor cwd_fd =
      nix::sys_open(&cwd[0], nix::open_mode::read_only, nix::open_flags::directory)
         .verify();
   nix::sys_fchdir(cwd_fd).verify();
   nix::sys_close(cwd_fd).verify();

   nix::sys_unlinkat(nix::at_fdcwd, tmp_dir_inside,
                     nix::atfile_flags::remove_directory)
      .verify();
   nix::sys_unlinkat(nix::at_fdcwd, tmp_dir,
                     nix::atfile_flags::remove_directory)
      .verify();
}

// Links / symlinks / rename.

$test(syscall_fs_links) {
   // Clean leftovers.
   auto unused_unlink_target = nix::sys_unlink(tmp_link_target);
   auto unused_unlink_link = nix::sys_unlink(tmp_link);
   auto unused_unlink_symlink = nix::sys_unlink(tmp_symlink);
   auto unused_unlink_rename_a = nix::sys_unlink(tmp_rename_a);
   auto unused_unlink_rename_b = nix::sys_unlink(tmp_rename_b);

   nix::sys_close(
      nix::sys_creat(tmp_link_target, nix::open_mode::read_write)
         .verify())
      .verify();

   nix::sys_link(tmp_link_target, tmp_link).verify();
   nix::sys_symlink(tmp_link_target, tmp_symlink).verify();

   char readlink_buffer[128] = {};
   cat::idx readlink_len =
      nix::sys_readlink(tmp_symlink, readlink_buffer, sizeof(readlink_buffer))
         .verify();
   // "/tmp/libcat_test_link_target" is 28 characters.
   cat::verify(readlink_len == cat::idx{28u});

   nix::sys_readlinkat(nix::at_fdcwd, tmp_symlink, readlink_buffer,
                       sizeof(readlink_buffer))
      .verify();

   // *at variants of link/symlink/rename.
   nix::sys_close(
      nix::sys_creat(tmp_rename_a, nix::open_mode::read_write).verify())
      .verify();
   nix::sys_renameat(nix::at_fdcwd, tmp_rename_a, nix::at_fdcwd,
                     tmp_rename_b)
      .verify();
   nix::sys_rename(tmp_rename_b, tmp_rename_a).verify();
   // `sys_renameat2` has the same arity plus flags. Pass `none` to mirror.
   nix::sys_renameat2(nix::at_fdcwd, tmp_rename_a, nix::at_fdcwd,
                      tmp_rename_b, nix::renameat2_flags::none)
      .verify();

   nix::sys_unlink(tmp_link).verify();
   nix::sys_unlink(tmp_symlink).verify();
   nix::sys_unlink(tmp_link_target).verify();
   nix::sys_unlink(tmp_rename_b).verify();
}

// openat / openat2 / fallocate / utimensat.

$test(syscall_fs_at_variants) {
   auto _ = nix::sys_unlink(tmp_atfile);

   nix::file_descriptor fd =
      nix::sys_openat(
         nix::at_fdcwd, tmp_atfile, nix::open_mode::read_write,
         nix::open_flags::create | nix::open_flags::truncate,
         nix::file_permissions::user_read | nix::file_permissions::user_write)
         .verify();
   nix::sys_close(fd).verify();

   nix::open_how how = {
      .flags = cat::uint8{cat::to_underlying(nix::open_mode::read_write)},
      .mode = 0u,
      .resolve = 0u,
   };
   nix::file_descriptor fd2 =
      nix::sys_openat2(nix::at_fdcwd, tmp_atfile, how).verify();
   nix::sys_ftruncate(fd2, cat::page_size).verify();
   // `sys_fallocate` may legitimately return `linux_error::opnotsupp` on tmpfs.
   auto fallocate_result =
      nix::sys_fallocate(fd2, nix::fallocate_flags::none, 0, cat::page_size);
   cat::verify(fallocate_result.has_value()
               || fallocate_result.error() == nix::linux_error::opnotsupp);

   // `utimensat` with `nullptr` p_times sets both to the current time.
   nix::sys_utimensat(nix::at_fdcwd, tmp_atfile, nullptr).verify();
   // `utime` likewise.
   nix::sys_utime(tmp_atfile, nullptr).verify();

   nix::sys_close(fd2).verify();
   nix::sys_unlink(tmp_atfile).verify();
}

// Memory.

$test(syscall_memory) {
   constexpr cat::uword bytes = cat::page_size;
   void* p_mapping =
      nix::sys_mmap(nullptr, bytes, nix::memory_protection_flags::read_write,
                    nix::memory_flags::privately | nix::memory_flags::anonymous,
                    nix::file_descriptor{-1}, 0)
         .verify();
   cat::verify(p_mapping != nullptr);
   // Touch the page to confirm it's mapped.
   *static_cast<unsigned char*>(p_mapping) = 42u;

   nix::sys_mprotect(p_mapping, bytes, nix::memory_protection_flags::read)
      .verify();

   // `mlock`/`mlockall` need RLIMIT_MEMLOCK or CAP_IPC_LOCK. Tolerate denial.
   nix::sys_mprotect(p_mapping, bytes, nix::memory_protection_flags::read_write)
      .verify();
   auto mlock_result = nix::sys_mlock(p_mapping, bytes);
   cat::verify(mlock_result.has_value()
               || is_denied_or_invalid(mlock_result.error()));
   if (mlock_result.has_value()) {
      nix::sys_munlock(p_mapping, bytes).verify();
   }
   auto mlock2_result =
      nix::sys_mlock2(p_mapping, bytes, nix::mlock2_flags::on_fault);
   cat::verify(mlock2_result.has_value()
               || is_denied_or_invalid(mlock2_result.error()));
   if (mlock2_result.has_value()) {
      nix::sys_munlock(p_mapping, bytes).verify();
   }
   auto mlockall_result = nix::sys_mlockall(nix::mlockall_flags::current);
   cat::verify(mlockall_result.has_value()
               || is_denied_or_invalid(mlockall_result.error()));
   if (mlockall_result.has_value()) {
      nix::sys_munlockall().verify();
   }

   // `mseal` is Linux 6.10+ and permanently blocks the mapping from being
   // modified or unmapped, so probe it on a throwaway mapping rather than
   // `p_mapping`.
   if (nix::has_sys_mseal()) {
      void* p_sealed =
         nix::sys_mmap(
            nullptr, bytes, nix::memory_protection_flags::read_write,
            nix::memory_flags::privately | nix::memory_flags::anonymous,
            nix::file_descriptor{-1}, 0)
            .verify();
      auto mseal_result = nix::sys_mseal(p_sealed, bytes);
      cat::verify(mseal_result.has_value()
                  || is_denied_or_invalid(mseal_result.error()));
      if (mseal_result.has_value()) {
         // A sealed mapping rejects `munmap` with `linux_error::perm`.
         auto munmap_sealed = nix::sys_munmap(p_sealed, bytes);
         cat::verify(!munmap_sealed.has_value()
                     && munmap_sealed.error() == nix::linux_error::perm);
      } else {
         nix::sys_munmap(p_sealed, bytes).verify();
      }
   }

   // `map_shadow_stack` requires Intel CET. Just probe the support query.
   (void)nix::has_sys_map_shadow_stack();

   // `sys_brk(nullptr)` queries the current break.
   auto* p_brk = nix::sys_brk(nullptr).verify();
   cat::verify(p_brk != nullptr);

   nix::sys_munmap(p_mapping, bytes).verify();
}

// `/proc/self/statm` parsing.

$test(syscall_read_self_statm) {
   nix::statm statm = nix::read_self_statm().verify();

   // The process must have a non-zero virtual size and a non-zero RSS.
   cat::verify(statm.total_pages > 0u);
   cat::verify(statm.resident_pages > 0u);
   // RSS is a subset of the total mapped pages.
   cat::verify(statm.resident_pages <= statm.total_pages);
   // Shared pages are a subset of resident pages.
   cat::verify(statm.shared_pages <= statm.resident_pages);
   // The text segment is always present and resident for a running binary.
   cat::verify(statm.text_pages > 0u);
   // The fifth field is `lib`, unused since Linux 2.6 and always 0.
   cat::verify(statm._ == 0u);
   // Data + stack always occupies at least one page.
   cat::verify(statm.data_pages > 0u);

   // The kernel updates the accounting on each allocation, so a second
   // call after a fresh mapping should report at least as much total VM.
   cat::idx const before = statm.total_pages;
   constexpr cat::uword bytes = 64u * cat::page_size;
   void* p_mapping =
      nix::sys_mmap(nullptr, bytes, nix::memory_protection_flags::read_write,
                    nix::memory_flags::privately | nix::memory_flags::anonymous,
                    nix::file_descriptor{-1}, 0)
         .verify();
   nix::statm after = nix::read_self_statm().verify();
   cat::verify(after.total_pages >= before);
   nix::sys_munmap(p_mapping, bytes).verify();
}

$test(syscall_read_self_anon_smaps) {
   nix::anon_smaps smaps = nix::read_self_anon_smaps().verify();

   cat::verify(smaps.resident_bytes > 0u);
   cat::verify(smaps.mapped_bytes > 0u);
   cat::verify(smaps.resident_bytes <= smaps.mapped_bytes);
   cat::verify(smaps.resident_bytes % cat::page_size == 0u);
   cat::verify(smaps.mapped_bytes % cat::page_size == 0u);

   cat::idx const mapped_before = smaps.mapped_bytes;
   cat::idx const resident_before = smaps.resident_bytes;
   constexpr cat::uword bytes = 64u * cat::page_size;
   void* p_mapping =
      nix::sys_mmap(nullptr, bytes, nix::memory_protection_flags::read_write,
                    nix::memory_flags::privately
                       | nix::memory_flags::populate
                       | nix::memory_flags::anonymous,
                    nix::file_descriptor{-1}, 0)
         .verify();
   static_cast<unsigned char*>(p_mapping)[0] = 42u;
   nix::anon_smaps after = nix::read_self_anon_smaps().verify();
   cat::verify(after.mapped_bytes >= mapped_before);
   cat::verify(after.resident_bytes >= resident_before);
   nix::sys_munmap(p_mapping, bytes).verify();
}

$test(syscall_mremap) {
   // Map two pages and write a sentinel into the first.
   void* p_old =
      nix::sys_mmap(nullptr, 2 * cat::page_size,
                    nix::memory_protection_flags::read_write,
                    nix::memory_flags::privately | nix::memory_flags::anonymous,
                    nix::file_descriptor{-1}, 0)
         .verify();
   static_cast<unsigned char*>(p_old)[0] = 42u;

   // Grow to four pages, allowing the kernel to relocate the mapping. The
   // contents move with it, so the sentinel survives at the (possibly new)
   // address.
   void* p_grown =
      nix::sys_mremap(p_old, 2 * cat::page_size, 4 * cat::page_size,
                      nix::mremap_flags::may_move)
         .verify();
   cat::verify(p_grown != nullptr);
   cat::verify(static_cast<unsigned char*>(p_grown)[0] == 42u);

   // Shrink in place: a shrink never needs to relocate, so even without
   // `may_move` it succeeds at the same address.
   void* p_shrunk = nix::sys_mremap(p_grown, 4 * cat::page_size, cat::page_size,
                                    nix::mremap_flags::none)
                       .verify();
   cat::verify(p_shrunk == p_grown);
   cat::verify(static_cast<unsigned char*>(p_shrunk)[0] == 42u);

   // A same-size remap without `may_move` is a no-op that returns the same
   // address.
   void* p_same = nix::sys_mremap(p_shrunk, cat::page_size, cat::page_size,
                                  nix::mremap_flags::none)
                     .verify();
   cat::verify(p_same == p_shrunk);

   nix::sys_munmap(p_shrunk, cat::page_size).verify();
}

// Scheduling / rlimit.

$test(syscall_resource) {
   nix::rlimit limits{};
   nix::sys_getrlimit(nix::rlimit_resource::max_open_files, limits).verify();
   cat::verify(limits.soft > 0u);

   // Setting the limit to its current value is always permitted.
   nix::sys_setrlimit(nix::rlimit_resource::max_open_files, limits).verify();

   // `sched_yield` always returns 0.
   nix::sys_sched_yield().verify();

   // Self-affinity round trip.
   cat::uword mask[16] = {};
   nix::sys_sched_getaffinity(nix::process_id{0},
                              cat::span<cat::uword>{mask, 16u})
      .verify();
   nix::sys_sched_setaffinity(nix::process_id{0},
                              cat::span<cat::uword const>{mask, 16u})
      .verify();
}

// Signals.

$test(syscall_signals_state) {
   // Query and restore the calling thread's signal mask.
   nix::signals_mask_set saved{};
   auto query_mask =
      nix::sys_rt_sigprocmask(nix::signal_action::block, nullptr, &saved);
   cat::verify(query_mask.has_value()
               || is_denied_or_invalid(query_mask.error()));
   auto restore_mask =
      nix::sys_rt_sigprocmask(nix::signal_action::set_mask, &saved, nullptr);
   cat::verify(restore_mask.has_value()
               || is_denied_or_invalid(restore_mask.error()));

   nix::signals_mask_set pending{};
   nix::sys_rt_sigpending(pending).verify();

   nix::signal_stack current_stack{};
   nix::sys_sigaltstack(nullptr, &current_stack).verify();

   // Query the current handler for SIGUSR1 then write it back unchanged.
   nix::sigaction usr1_old{};
   nix::sys_rt_sigaction(nix::signal::usr1, nullptr, &usr1_old).verify();
   nix::sys_rt_sigaction(nix::signal::usr1, &usr1_old, nullptr).verify();
}

$test(syscall_signals_self_kill) {
   // Use impossible target ids so signal delivery fails without mutating the
   // current thread's signal state.
   nix::process_id self_pid = nix::sys_getpid();
   auto tkill_result = nix::sys_tkill(nix::process_id{-1}, nix::signal::usr1);
   cat::verify(!tkill_result.has_value()
               && (tkill_result.error() == nix::linux_error::srch
                   || tkill_result.error() == nix::linux_error::inval));

   auto tgkill_result =
      nix::sys_tgkill(self_pid, nix::process_id{-1}, nix::signal::usr1);
   cat::verify(!tgkill_result.has_value()
               && (tgkill_result.error() == nix::linux_error::srch
                   || tgkill_result.error() == nix::linux_error::inval));
}

// Sockets.

$test(syscall_socket_pair) {
   // `AF_UNIX = 1`, `SOCK_STREAM = 1`.
   cat::int4 fds[2] = {};
   nix::sys_socketpair(1, 1, 0, fds).verify();
   nix::file_descriptor a{fds[0]};
   nix::file_descriptor b{fds[1]};

   // Round-trip through `send`/`recv`.
   // Use the namespace-scope null-terminated payload.
   cat::iword sent =
      nix::sys_sendto(a, payload_two.data(), payload_two_length)
         .verify();
   cat::verify(sent == cat::iword(payload_two_length));

   char buffer[8] = {};
   cat::iword got = nix::sys_recv(b, buffer, sizeof(buffer)).verify();
   cat::verify(got == 2);
   cat::verify(buffer[0] == 'h');
   sent = nix::sys_sendto(a, payload_two.data(), payload_two_length)
             .verify();
   cat::verify(sent == cat::iword(payload_two_length));
   char explicit_null_buffer[8] = {};
   got = nix::sys_recv(b, explicit_null_buffer, sizeof(explicit_null_buffer),
                       nullptr, nullptr)
            .verify();
   cat::verify(got == 2);
   cat::verify(explicit_null_buffer[0] == 'h');
   sent = nix::sys_sendto(a, payload_two.data(), payload_two_length)
             .verify();
   cat::verify(sent == cat::iword(payload_two_length));
   cat::socket_unix<cat::socket_type::stream> socket_receiver(b);
   char socket_buffer[8] = {};
   got = socket_receiver
            .recieve(socket_buffer, sizeof(socket_buffer), nullptr, nullptr)
            .value();
   cat::verify(got == 2);
   cat::verify(socket_buffer[0] == 'h');

   // `sendmsg` / `recvmsg` round trip.
   nix::io_vector iov_send[1] = {
      nix::io_vector(
         __builtin_bit_cast(cat::byte* _Nonnull, payload_two.data()),
         payload_two_length),
   };
   nix::msg_header sender{};
   sender.p_name = nullptr;
   sender.name_length = 0u;
   sender.p_io_vectors = iov_send;
   sender.io_vectors_count = 1u;
   sender.p_control = nullptr;
   sender.control_length = 0u;
   sender.flags = 0;
   cat::iword sent_msg = nix::sys_sendmsg(a, sender).verify();
   cat::verify(sent_msg == cat::iword(payload_two_length));

   char recv_buf[8] = {};
   nix::io_vector iov_recv[1] = {
      nix::io_vector(reinterpret_cast<cat::byte*>(recv_buf),
                     cat::idx(sizeof(recv_buf))),
   };
   nix::msg_header receiver{};
   receiver.p_name = nullptr;
   receiver.name_length = 0u;
   receiver.p_io_vectors = iov_recv;
   receiver.io_vectors_count = 1u;
   receiver.p_control = nullptr;
   receiver.control_length = 0u;
   receiver.flags = 0;
   cat::iword got_msg = nix::sys_recvmsg(b, receiver).verify();
   cat::verify(got_msg == 2);

   // `shutdown` half-close, then close both ends.
   nix::sys_shutdown(a, nix::shutdown_how::write).verify();
   nix::sys_close(a).verify();
   nix::sys_close(b).verify();
}

$test(syscall_socket_options) {
   // Create a UDP socket so `getsockname` returns a meaningful address.
   // `AF_INET = 2`, `SOCK_DGRAM = 2`.
   nix::file_descriptor sock = nix::sys_socket(2, 2, 0).verify();

   // `SOL_SOCKET = 1`, `SO_RCVBUF = 8`. The kernel may double the buffer
   // size on set, just verify the call paths execute.
   cat::int4 buf_size = cat::int4(cat::page_size.raw);
   nix::sys_setsockopt(sock, 1, 8, &buf_size, sizeof(buf_size)).verify();

   cat::int4 read_back = 0;
   cat::int4 length = sizeof(read_back);
   nix::sys_getsockopt(sock, 1, 8, &read_back, length).verify();
   cat::verify(read_back >= buf_size);

   nix::sys_close(sock).verify();
}

// Randomness.

$test(syscall_random) {
   unsigned char buffer[16] = {};
   cat::iword got = nix::sys_getrandom(buffer, sizeof(buffer),
                                       nix::getrandom_flags::nonblocking)
                       .verify();
   cat::verify(got == cat::iword(sizeof(buffer)));
}

// io_uring (gated on runtime probe).

$test(syscall_io_uring) {
   if (!nix::has_sys_io_uring()) {
      return;
   }

   // Minimum-sized ring. Use `IORING_SETUP_R_DISABLED` to avoid SQ-thread
   // creation we'd then need to clean up.
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::ring_disabled;
   auto setup_result = nix::sys_io_uring_setup(4u, params);
   if (!setup_result.has_value()) {
      // Some sandboxes deny io_uring_setup with `linux_error::perm` even
      // though probing returned true (the probe uses entries=0 which the
      // kernel rejects before checking the disabled flag).
      cat::verify(setup_result.error() == nix::linux_error::perm
                  || setup_result.error() == nix::linux_error::nosys);
      return;
   }
   nix::file_descriptor ring = setup_result.value();

   // `IORING_REGISTER_PROBE` needs an io_uring_probe buffer, just exercise
   // `enter` with `to_submit == 0` and no waiting.
   auto enter_result =
      nix::sys_io_uring_enter(ring, 0u, 0u, nix::io_uring_enter_flags::none);
   cat::verify(enter_result.has_value()
               || enter_result.error() == nix::linux_error::badfd);

   nix::sys_close(ring).verify();
}

// Linux 6.x optional syscalls.

$test(syscall_optional_six_x) {
   // `cachestat` (Linux 6.5+).
   if (nix::has_sys_cachestat()) {
      auto _ = nix::sys_unlink(tmp_basic);
      nix::file_descriptor fd =
         nix::sys_creat(tmp_basic, nix::open_mode::read_write).verify();
      nix::sys_write(fd, payload_short.data(), payload_short_length)
         .verify();
      nix::cachestat_range range = {.offset = 0, .length = 4};
      nix::cachestat_stats out{};
      auto cs = nix::sys_cachestat(fd, range, out);
      cat::verify(cs.has_value() || cs.error() == nix::linux_error::nosys
                  || cs.error() == nix::linux_error::perm);
      nix::sys_close(fd).verify();
      nix::sys_unlink(tmp_basic).verify();
   }

   // `fchmodat2` (Linux 6.6+).
   if (nix::has_sys_fchmodat2()) {
      auto unused_unlink = nix::sys_unlink(tmp_basic);
      nix::sys_close(
         nix::sys_creat(tmp_basic, nix::open_mode::read_write).verify())
         .verify();
      auto fc = nix::sys_fchmodat2(nix::at_fdcwd, tmp_basic, 0644u, 0);
      cat::verify(fc.has_value() || fc.error() == nix::linux_error::nosys
                  || fc.error() == nix::linux_error::perm);
      nix::sys_unlink(tmp_basic).verify();
   }

   // `futex_wake/wait/requeue` (Linux 6.7+) need real waiters. Just confirm
   // the queries and the syscalls are dispatchable.
   if (nix::has_sys_futex_wake()) {
      nix::futex_word word{};
      auto wake = nix::sys_futex_wake(word, ~0ul, 0, 2u);
      cat::verify(wake.has_value() || wake.error() == nix::linux_error::inval
                  || wake.error() == nix::linux_error::nosys);
   }
}
