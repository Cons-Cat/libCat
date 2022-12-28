#include <cat/linux>

auto nix::sys_fstat(file_descriptor file_descriptor)
    -> cat::scaredy<file_status, linux_error> {
    file_status status;
    scaredy_nix<void> result = syscall<void>(5, file_descriptor, &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<linux_error>();
}
