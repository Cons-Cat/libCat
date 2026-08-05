#include <cat/linux>

auto
nix::sys_recvmsg(
   file_descriptor socket_descriptor, msg_header& message, message_flags flags
) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      47, socket_descriptor, &message, flags
   );
}
