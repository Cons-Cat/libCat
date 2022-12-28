#include <cat/linux>

auto nix::sys_open(char const* p_file_path, nix::open_mode file_mode,
                   nix::open_flags flags)
    -> nix::scaredy_nix<nix::file_descriptor> {
    // TODO: Figure out how to best support `close_exec`.
    // TODO: `large_file` should only be enabled on 64-bit targets.
    return syscall<nix::file_descriptor>(
        2, p_file_path,
        nix::open_flags::large_file | flags | static_cast<int>(file_mode),
        file_mode);
}
