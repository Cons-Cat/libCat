#include <cat/linux>

auto nix::sys_stat(cat::string const file_path)
    -> cat::scaredy<file_status, linux_error> {
    file_status status;
    scaredy_nix<void> result = syscall<void>(4, file_path.data(), &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<linux_error>();
}
