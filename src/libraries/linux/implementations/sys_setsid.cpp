#include <cat/linux>

auto
nix::sys_setsid() -> nix::scaredy_nix<process_id> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<process_id>(112);
}
