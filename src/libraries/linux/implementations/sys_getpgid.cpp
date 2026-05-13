#include <cat/linux>

auto
nix::sys_getpgid(process_id pid) -> nix::scaredy_nix<process_id> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<process_id>(121, pid);
}
