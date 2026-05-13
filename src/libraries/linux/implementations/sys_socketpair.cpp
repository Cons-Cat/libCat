#include <cat/linux>

auto
nix::sys_socketpair(cat::int8 protocol_family, cat::int8 type,
                    cat::int8 protocol, cat::int4 (&socket_vector)[2])
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(53, protocol_family, type, protocol,
                                      &socket_vector[0]);
}
