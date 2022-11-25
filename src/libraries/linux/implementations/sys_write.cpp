#include <cat/linux>

// `write()` forwards its arguments to a failable stdout syscall. It returns
// the number of bytes that it wrote.
auto nix::sys_write(nix::FileDescriptor file_descriptor,
                    char const* p_string_buffer, ssize length)
    -> nix::ScaredyLinux<ssize> {
    return nix::syscall<ssize>(1, file_descriptor, p_string_buffer, length);
}

// `write()` forwards its arguments to a failable catout syscall. It returns
// the number of bytes that it wrote.
auto nix::sys_write(nix::FileDescriptor file_descriptor,
                    cat::String const string) -> nix::ScaredyLinux<ssize> {
    return nix::syscall<ssize>(1, file_descriptor, string.data(),
                               string.size());
}
