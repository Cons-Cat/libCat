#include <cat/linux>

auto
nix::sys_utime(cat::zstr_view file_path,
               utimbuf const* _Nullable p_times) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(132, file_path.data(), p_times);
}
