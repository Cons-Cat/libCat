// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto close(FileDescriptor const object) -> Result<> {
    return syscall1(3, object);
}
