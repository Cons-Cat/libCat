// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::write(FileDescriptor const file_descriptor,
                char const* p_string_buffer, ssize const string_length)
    -> Result<ssize> {
    // TODO: Handle partial writes somehow.
    return nix::syscall3(1, file_descriptor, p_string_buffer, string_length);
}

auto nix::write(FileDescriptor const file_descriptor, String const& string)
    -> Result<ssize> {
    // TODO: Handle partial writes somehow.
    return nix::syscall3(1, file_descriptor, string.p_data, string.length);
}
