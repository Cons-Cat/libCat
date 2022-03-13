// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::unlink(char const* path_name) -> Result<> {
    return nix::syscall1(87u, path_name);
}
