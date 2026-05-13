#include <cat/linux>

auto
nix::sys_sched_setaffinity(process_id pid, cat::span<cat::uword const> mask)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(203, pid, mask.size_bytes(), mask.data());
}
