#include <cat/linux>

auto
nix::sys_rt_tgsigqueueinfo(process_id tgid, process_id tid, signal s,
                           signal_info const& info) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(297, tgid, tid, s, &info);
}
