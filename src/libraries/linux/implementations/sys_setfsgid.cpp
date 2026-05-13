#include <cat/linux>

auto
nix::sys_setfsgid(group_id group) -> group_id {
   // The kernel always returns the previous fsgid. There is no error path.
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<group_id>(123, group).value();
}
