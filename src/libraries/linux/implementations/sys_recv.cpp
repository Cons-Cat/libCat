#include <cat/linux>

auto nix::sys_recv(nix::file_descriptor socket_descriptor,
                   void* p_message_buffer, ssize buffer_length,
                   cat::Socket const* __restrict p_addr,
                   ssize const* __restrict p_addr_length)
    -> nix::scaredy_nix<ssize> {
    auto foo = nix::syscall<ssize>(45, socket_descriptor, p_message_buffer,
                                   buffer_length, p_addr, p_addr_length);
    return foo;
}
