#include <cat/linux>

auto
nix::sys_getpid() -> process_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<process_id>(39).value();
}
