#include <cat/linux>

// `read()` transmits a number of bytes into a file descriptor.
auto nix::sys_read(file_descriptor file_descriptor, char const* p_string_buffer,
                   iword length) -> nix::scaredy_nix<iword> {
    return nix::syscall<iword>(0, file_descriptor, p_string_buffer, length);
}
