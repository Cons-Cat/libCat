#include <cat/linux>

auto nix::open_file(char const* p_file_path, nix::OpenMode file_mode,
                    nix::OpenFlags flags)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    // TODO: Figure out how to best support `close_exec`.
    // TODO: `large_file` should only be enabled on 64-bit targets.
    return syscall<nix::FileDescriptor>(
        2, p_file_path,
        nix::OpenFlags::large_file | flags | static_cast<int>(file_mode),
        file_mode);
}
