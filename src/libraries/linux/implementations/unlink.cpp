// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto unlink(char const* path_name) -> Result<> {
    return syscall1(87u, path_name);
}
