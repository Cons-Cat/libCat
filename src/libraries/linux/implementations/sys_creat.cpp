#include <cat/linux>

auto
nix::sys_creat(cat::zstr_view file_path, file_permissions mode)
   -> nix::scaredy_nix<file_descriptor> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(85, file_path.data(), mode);
}
