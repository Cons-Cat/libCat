#include <cat/linux>

auto
nix::sys_fcntl(
   file_descriptor file_descriptor, fcntl_command command,
   cat::no_type_ptr p_argument
) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(
      72, file_descriptor, command, p_argument
   );
}
