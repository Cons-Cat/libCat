#include <cat/linux>

auto
nix::sys_io_uring_setup(cat::uint4 entries, io_uring_params& params)
   -> nix::scaredy_nix<file_descriptor> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(425, entries, &params);
}
