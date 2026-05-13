#include <cat/linux>

[[noreturn]]
void
nix::sys_exit(cat::int4 status) {
   for (;;) {
      // https://filippo.io/linux-syscall-table/
      nix::syscall1_volatile(60, status);
   }
}
