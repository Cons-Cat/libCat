#include <cat/linux>

auto
nix::sys_recv(file_descriptor socket_descriptor, cat::span<char> buffer,
              cat::Socket const* _Nullable __restrict p_addr,
              cat::iword const* _Nullable __restrict p_addr_length)
   -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   auto foo = nix::syscall_volatile<cat::iword>(
      45, socket_descriptor, buffer.data(), buffer.size(), p_addr,
      p_addr_length);
   return foo;
}
