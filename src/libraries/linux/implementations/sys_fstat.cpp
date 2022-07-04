#include <cat/linux>

auto nix::sys_fstat(nix::FileDescriptor const file_descriptor)
    -> cat::Scaredy<nix::FileStatus, nix::LinuxError> {
    nix::FileStatus status;
    nix::ScaredyLinux<void> result =
        nix::syscall<void>(5, file_descriptor, &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<nix::LinuxError>();
}
