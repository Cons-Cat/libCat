#include <cat/linux>

auto nix::sys_fstat(FileDescriptor file_descriptor)
    -> cat::Scaredy<FileStatus, LinuxError> {
    FileStatus status;
    ScaredyLinux<void> result = syscall<void>(5, file_descriptor, &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<LinuxError>();
}
