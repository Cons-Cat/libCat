// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto write(FileDescriptor const file_descriptor, char const* p_string_buffer,
           isize const string_length) -> Result<isize> {
    // TODO: Handle partial writes somehow.
    return syscall3(1, file_descriptor, p_string_buffer, string_length);
}
