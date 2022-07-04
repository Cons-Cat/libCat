#include <cat/linux>

auto nix::sys_socket(int8 const protocol_family, int8 const type,
                     int8 const protocol)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::syscall<nix::FileDescriptor>(41, protocol_family, type,
                                             protocol);
}
