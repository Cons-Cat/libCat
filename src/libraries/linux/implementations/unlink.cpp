// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::unlink(char const* p_path_name) -> Result<> {
    return nix::syscall1(87, p_path_name);
}
