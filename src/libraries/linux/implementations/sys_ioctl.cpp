#include <cat/linux>

// Control the settings of special files, such as stdin or stdout.
auto nix::sys_ioctl(FileDescriptor io_descriptor, IoRequests request)
    -> ScaredyLinux<void> {
    return syscall<void>(54, io_descriptor, request);
}

// Control the settings of special files, such as stdin or stdout, with a
// request that requires a parameter.
auto nix::sys_ioctl(FileDescriptor io_descriptor, IoRequests request,
                    cat::NoTypePtr p_argument) -> ScaredyLinux<void> {
    return syscall<void>(16, io_descriptor, request, p_argument);
}
