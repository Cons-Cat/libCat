#include <cat/linux>

// TODO: Make distinct wrappers for `wait` syscall variants.
auto nix::sys_wait4(nix::ProcessId waiting_on_id, int4* p_status_output,
                    nix::WaitOptionsFlags options, void* p_resource_usage)
    -> nix::ScaredyLinux<nix::ProcessId> {
    // TODO: Use `p_status_output` for failure-handling.
    return nix::syscall<nix::ProcessId>(61, waiting_on_id, p_status_output,
                                        options, p_resource_usage);
}
