// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::close(FileDescriptor const object) -> Result<> {
    return nix::syscall1(3, object);
}
