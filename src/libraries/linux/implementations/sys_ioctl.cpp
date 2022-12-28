#include <cat/linux>

// Control the settings of special files, such as stdin or stdout.
auto nix::sys_ioctl(file_descriptor io_descriptor, io_requests request)
    -> scaredy_nix<void> {
    return syscall<void>(54, io_descriptor, request);
}

// Control the settings of special files, such as stdin or stdout, with a
// request that requires a parameter.
auto nix::sys_ioctl(file_descriptor io_descriptor, io_requests request,
                    cat::no_type_ptr p_argument) -> scaredy_nix<void> {
    return syscall<void>(16, io_descriptor, request, p_argument);
}
