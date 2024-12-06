#include <cat/linux>

// TODO: These should take and return `cat::idx`.

// `write()` forwards its arguments to a failable stdout syscall. It returns
// the number of bytes that it wrote.
auto
nix::sys_write(nix::file_descriptor file_descriptor,
               char const* p_string_buffer, cat::iword length)
   -> nix::scaredy_nix<cat::iword> {
   return nix::syscall<cat::iword>(1, file_descriptor, p_string_buffer, length);
}

// `write()` forwards its arguments to a failable stdout syscall. It returns
// the number of bytes that it wrote.
auto
nix::sys_write(nix::file_descriptor file_descriptor, cat::str_view const string)
   -> nix::scaredy_nix<cat::iword> {
   return nix::syscall<cat::iword>(1, file_descriptor, string.data(),
                                   string.size());
}
