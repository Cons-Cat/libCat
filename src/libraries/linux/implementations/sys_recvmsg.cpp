#include <cat/linux>

auto
nix::sys_recvmsg(
   file_descriptor socket_descriptor, msg_header& message, message_flags flags
) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(
      47, socket_descriptor, &message, flags
   );
}
