#include <cat/linux>

// TODO: These should take and return `cat::idx`.

// `write()` forwards its arguments to a failable stdout syscall. It returns the
// number of bytes that it wrote.
auto
nix::sys_write(file_descriptor file_descriptor, cat::str_view string)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(1, file_descriptor, string.data(),
                                          string.size());
}
