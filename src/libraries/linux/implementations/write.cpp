// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

// `write()` forwards its arguments to a failable catout syscall. It returns
// the number of bytes that it wrote.
auto nix::write(nix::FileDescriptor const file_descriptor,
                char const* p_string_buffer, ssize const length)
    -> Result<ssize> {
    // TODO: Handle partial writes somehow.
    return nix::syscall3(1, file_descriptor, p_string_buffer, length);
}

// `write()` forwards its arguments to a failable catout syscall. It returns
// the number of bytes that it wrote.
auto nix::write(nix::FileDescriptor const file_descriptor,
                cat::String const& string) -> Result<ssize> {
    // TODO: Handle partial writes somehow.
    return nix::syscall3(1, file_descriptor, string.p_data(), string.size());
}
