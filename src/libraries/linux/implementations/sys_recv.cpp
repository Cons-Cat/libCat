#include <cat/linux>

auto
nix::sys_recv(
   file_descriptor socket_descriptor, cat::span<char> buffer,
   message_flags flags, void* _Nullable __restrict p_addr,
   cat::iword* _Nullable __restrict p_addr_length
) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      45, socket_descriptor, buffer.data(), buffer.size(), flags, p_addr,
      p_addr_length
   );
}
