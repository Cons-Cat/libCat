#include <cat/linux>

auto
nix::sys_utime(char const* p_file_path, utimbuf const* p_times)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(132, p_file_path, p_times);
}
