#include <cat/linux>

auto
nix::sys_setfsuid(user_id user) -> user_id {
   // The kernel always returns the previous fsuid. There is no error path.
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<user_id>(122, user).value();
}
