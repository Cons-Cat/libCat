#include <cat/linux>

auto
nix::sys_link(char const* _Nonnull __restrict p_existing_path,
              char const* _Nonnull __restrict p_new_path)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(86, p_existing_path, p_new_path);
}
