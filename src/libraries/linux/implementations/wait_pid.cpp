#include <cat/linux>

auto nix::wait_pid(process_id pid, file_status* p_file_status,
                   wait_options_flags options) -> scaredy_nix<process_id> {
    return syscall<process_id>(61, pid, p_file_status, options);
}
