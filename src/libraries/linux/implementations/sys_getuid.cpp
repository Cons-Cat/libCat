#include <cat/linux>

auto
nix::sys_getuid() -> user_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<user_id>(102).value();
}
