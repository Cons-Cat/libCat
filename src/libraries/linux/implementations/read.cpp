// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::read(FileDescriptor const file_descriptor,
               char const* p_string_buffer, isize const string_length)
    -> Result<isize> {
    return nix::syscall3(0, file_descriptor, p_string_buffer, string_length);
}
