#include <cat/linux>

auto
nix::sys_recv(file_descriptor socket_descriptor,
              void* _Nonnull p_message_buffer, cat::iword buffer_length,
              cat::Socket const* _Nullable __restrict p_addr,
              cat::iword const* _Nullable __restrict p_addr_length)
   -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   auto foo =
      nix::syscall_volatile<cat::iword>(45, socket_descriptor, p_message_buffer,
                                        buffer_length, p_addr, p_addr_length);
   return foo;
}
