#include <cat/linux>

auto
nix::sys_sendmsg(file_descriptor socket_descriptor, msg_header const& message,
                 cat::int4 flags) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(46, socket_descriptor, &message,
                                            flags);
}
