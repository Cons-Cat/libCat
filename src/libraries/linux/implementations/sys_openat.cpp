#include <cat/linux>

auto
nix::sys_openat(file_descriptor dirfd, cat::zstr_view file_path, open_mode mode,
                open_flags flags, file_permissions permissions)
   -> nix::scaredy_nix<file_descriptor> {
   // Mirrors `sys_open()`'s `large_file` opt-in plus the `open_mode` bits, so
   // both syscalls behave identically aside from `dirfd`.
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(
      257, dirfd, file_path.data(),
      open_flags::large_file | flags | static_cast<open_flags>(mode),
      permissions);
}
