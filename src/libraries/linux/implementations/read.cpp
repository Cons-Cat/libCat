// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::read(FileDescriptor const file_descriptor,
               char const* p_string_buffer, ssize const string_length)
    -> Result<ssize> {
    return nix::syscall3(0, file_descriptor, p_string_buffer, string_length);
}
