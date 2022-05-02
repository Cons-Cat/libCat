// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::close(nix::FileDescriptor const object) -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(3, object);
}
