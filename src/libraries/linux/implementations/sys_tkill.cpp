#include <cat/linux>

auto
nix::sys_tkill(process_id pid, signal signal) noexcept -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<void>(200, pid, signal);
}
