#include <cat/linux>

// `write()` forwards its arguments to a failable stdout syscall. It returns
// the number of bytes that it wrote.
auto nix::sys_write(nix::file_descriptor file_descriptor,
                    char const* p_string_buffer, iword length)
    -> nix::scaredy_nix<iword> {
    return nix::syscall<iword>(1, file_descriptor, p_string_buffer, length);
}

// `write()` forwards its arguments to a failable catout syscall. It returns
// the number of bytes that it wrote.
auto nix::sys_write(nix::file_descriptor file_descriptor,
                    cat::string const string) -> nix::scaredy_nix<iword> {
    return nix::syscall<iword>(1, file_descriptor, string.data(),
                               string.size());
}
