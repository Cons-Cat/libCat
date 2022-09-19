#include <cat/linux>

auto nix::sys_stat(cat::String const file_path)
    -> cat::Scaredy<FileStatus, LinuxError> {
    FileStatus status;
    ScaredyLinux<void> result = syscall<void>(4, file_path.p_data(), &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<LinuxError>();
}
