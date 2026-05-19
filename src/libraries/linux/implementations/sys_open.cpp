#include <cat/linux>

auto
nix::sys_open(char const* _Nonnull p_file_path, open_mode file_mode,
              open_flags flags) -> nix::scaredy_nix<nix::file_descriptor> {
   // TODO: Figure out how to best support `close_exec`.
   // TODO: `large_file` should only be enabled on 64-bit targets.
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<nix::file_descriptor>(
      2, p_file_path,
      nix::open_flags::large_file
         | flags
         | static_cast<nix::open_flags>(file_mode),
      file_mode);
}
