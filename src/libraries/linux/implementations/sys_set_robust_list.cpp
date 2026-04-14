#include <cat/linux>

auto
nix::sys_set_robust_list(robust_list_head* p_head, cat::uword length_bytes)
   -> nix::scaredy_nix<void> {
   return nix::syscall<void>(273, p_head, length_bytes);
}
