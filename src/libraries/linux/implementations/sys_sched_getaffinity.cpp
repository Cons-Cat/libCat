#include <cat/linux>

auto
nix::sys_sched_getaffinity(process_id pid, cat::span<cat::uword> mask)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<cat::idx>(204, pid, mask.size_bytes(), mask.data());
}
