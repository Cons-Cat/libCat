#include <cat/linux>

auto
nix::sys_set_tid_address(cat::int4* p_tid) -> process_id {
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<process_id>(218, p_tid).value();
}
