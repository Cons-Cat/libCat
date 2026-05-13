#include <cat/linux>

auto
nix::sys_getegid() -> group_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<group_id>(108).value();
}
