#include <cat/linux>

auto nix::sys_recv(nix::FileDescriptor const socket_descriptor,
                   void* const p_message_buffer, ssize const buffer_length,
                   cat::Socket const* const __restrict p_addr,
                   ssize const* const __restrict p_addr_length)
    -> nix::ScaredyLinux<ssize> {
    auto foo = nix::syscall<ssize>(45, socket_descriptor, p_message_buffer,
                                   buffer_length, p_addr, p_addr_length);
    return foo;
}
