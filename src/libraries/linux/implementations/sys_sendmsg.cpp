#include <cat/linux>

auto
nix::sys_sendmsg(
   file_descriptor socket_descriptor, msg_header const& message,
   message_flags flags
) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      46, socket_descriptor, &message, flags
   );
}
