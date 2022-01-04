// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto wait4(ProcessId waiting_on_id, i4* p_status_output, i4 options,
           void* p_resource_usage) -> Result<Any> {
    // TODO: Use `p_status_output` for failure-handling.
    return syscall4(61u, waiting_on_id, p_status_output, options,
                    p_resource_usage);
}
