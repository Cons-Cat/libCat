// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <unistd.h>

auto write(i8 const& file_descriptor, char8_t const* p_string_buffer,
           isize const& string_length) -> Result<isize> {
    return syscall3(1, file_descriptor, p_string_buffer, string_length);
}
