// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::wait(ProcessId waiting_on_id, int4* p_status_output,
               WaitOptionsFlags options, void* p_resource_usage)
    -> Result<cat::Any> {
    // TODO: Use `p_status_output` for failure-handling.
    return nix::syscall4(61, waiting_on_id, p_status_output, options,
                         p_resource_usage);
}
