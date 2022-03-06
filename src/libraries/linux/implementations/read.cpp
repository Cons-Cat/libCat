// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto read(FileDescriptor const file_descriptor, char8_t const* p_string_buffer,
          isize const string_length) -> Result<isize> {
    return syscall3(0, file_descriptor, p_string_buffer, string_length);
}
