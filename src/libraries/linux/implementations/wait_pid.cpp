#include <cat/linux>

auto nix::wait_pid(ProcessId pid, FileStatus* p_file_status,
                   WaitOptionsFlags options) -> ScaredyLinux<ProcessId> {
    return syscall<ProcessId>(61, pid, p_file_status, options);
}
