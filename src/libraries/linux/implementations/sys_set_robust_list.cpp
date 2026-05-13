#include <cat/linux>

auto
nix::sys_set_robust_list(robust_list_head* p_head, cat::uword length_bytes)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(273, p_head, length_bytes);
}
