#include <cat/linux>

auto
nix::sys_geteuid() -> user_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<user_id>(107).value();
}
