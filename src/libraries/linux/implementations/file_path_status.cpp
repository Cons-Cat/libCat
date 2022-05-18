#include <cat/linux>

auto nix::file_path_status(cat::String const& file_path)
    -> cat::Scaredy<nix::FileStatus, nix::LinuxError> {
    nix::FileStatus status;
    nix::ScaredyLinux<void> result =
        nix::syscall<void>(4, file_path.p_data(), &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<nix::LinuxError>();
}
