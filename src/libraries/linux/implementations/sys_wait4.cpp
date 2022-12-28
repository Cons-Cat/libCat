#include <cat/linux>

// TODO: Make distinct wrappers for `wait` syscall variants.
auto nix::sys_wait4(nix::process_id waiting_on_id, int4* p_status_output,
                    nix::wait_options_flags options, void* p_resource_usage)
    -> nix::scaredy_nix<nix::process_id> {
    // TODO: Use `p_status_output` for failure-handling.
    return nix::syscall<nix::process_id>(61, waiting_on_id, p_status_output,
                                        options, p_resource_usage);
}
