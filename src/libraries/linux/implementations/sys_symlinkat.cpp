#include <cat/linux>

auto
nix::sys_symlinkat(char const* p_target_path, file_descriptor new_dirfd,
                   char const* p_link_path) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(266, p_target_path, new_dirfd,
                                      p_link_path);
}
