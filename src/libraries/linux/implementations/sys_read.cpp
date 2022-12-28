#include <cat/linux>

// `read()` transmits a number of bytes into a file descriptor.
auto nix::sys_read(file_descriptor file_descriptor, char const* p_string_buffer,
                   ssize length) -> nix::scaredy_nix<ssize> {
    return nix::syscall<ssize>(0, file_descriptor, p_string_buffer, length);
}
