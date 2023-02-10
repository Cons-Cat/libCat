#include <cat/linux>

// Create and return a socket.
auto nix::sys_socket(cat::int8 protocol_family, cat::int8 type,
                     cat::int8 protocol)
    -> nix::scaredy_nix<nix::file_descriptor> {
    return nix::syscall<nix::file_descriptor>(41, protocol_family, type,
                                              protocol);
}
