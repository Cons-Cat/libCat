#include <cat/linux>

auto
nix::sys_symlinkat(
   cat::zstr_view target_path, file_descriptor new_dirfd,
   cat::zstr_view link_path
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      266, target_path.data(), new_dirfd, link_path.data()
   );
}
