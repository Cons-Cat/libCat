// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

// `read()` transmits a number of bytes into a file descriptor.
auto nix::read(FileDescriptor const file_descriptor,
               char const* p_string_buffer, ssize const length)
    -> Result<ssize> {
    return nix::syscall3(0, file_descriptor, p_string_buffer, length);
}
