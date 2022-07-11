#include <cat/linux>

auto nix::create_socket_local(int8 type, int8 protocol)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::sys_socket(1, type, protocol);
}
