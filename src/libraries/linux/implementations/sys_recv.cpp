#include <cat/linux>

auto nix::sys_recv(nix::file_descriptor socket_descriptor,
                   void* p_message_buffer, iword buffer_length,
                   cat::Socket const* __restrict p_addr,
                   iword const* __restrict p_addr_length)
    -> nix::scaredy_nix<iword> {
    auto foo = nix::syscall<iword>(45, socket_descriptor, p_message_buffer,
                                   buffer_length, p_addr, p_addr_length);
    return foo;
}
