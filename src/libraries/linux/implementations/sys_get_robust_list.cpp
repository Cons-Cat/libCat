#include <cat/linux>

auto
nix::sys_get_robust_list(process_id target,
                         robust_list_head* _Nullable& out_head,
                         cat::idx& out_length_bytes) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(274, target, &out_head, &out_length_bytes);
}
