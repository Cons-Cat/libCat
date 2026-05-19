#include <cat/linux>

auto
nix::sys_io_uring_register(file_descriptor ring, io_uring_register_op op,
                           void* _Nullable p_arg, cat::uint4 nr_args)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(427, ring, op, p_arg, nr_args);
}
