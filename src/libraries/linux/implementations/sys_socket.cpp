#include <cat/linux>

// Create and return a socket.
auto nix::sys_socket(int8 protocol_family, int8 type, int8 protocol)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::syscall<nix::FileDescriptor>(41, protocol_family, type,
                                             protocol);
}
