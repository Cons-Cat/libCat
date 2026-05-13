#include <cat/linux>

auto
nix::sys_gettid() -> process_id {
   // https://filippo.io/linux-syscall-table/
   return syscall<process_id>(186).value();
}
