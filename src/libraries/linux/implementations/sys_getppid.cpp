#include <cat/linux>

auto
nix::sys_getppid() -> process_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<process_id>(110).value();
}
