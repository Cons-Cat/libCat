#include <cat/linux>

auto
nix::sys_get_robust_list(process_id target, robust_list_head** pp_head,
                         cat::idx* p_length_bytes) -> nix::scaredy_nix<void> {
   return nix::syscall<void>(274, target, pp_head, p_length_bytes);
}
