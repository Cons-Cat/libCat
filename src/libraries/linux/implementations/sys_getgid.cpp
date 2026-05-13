#include <cat/linux>

auto
nix::sys_getgid() -> group_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<group_id>(104).value();
}
