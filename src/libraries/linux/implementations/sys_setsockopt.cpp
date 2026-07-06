#include <cat/linux>

auto
nix::sys_setsockopt(
   file_descriptor socket_descriptor, cat::int4 level, cat::int4 option_name,
   void const* _Nonnull p_option_value, cat::int4 option_length
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      54, socket_descriptor, level, option_name, p_option_value, option_length
   );
}
