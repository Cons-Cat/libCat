// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <linux>

struct CloneArguments {
    u8 flags;
    FileDescriptor* process_id_file_descriptor;
    ProcessId* child_thread_id;
    ProcessId* parent_thread_id;
    i8 exit_code;
    void* p_stack;
    usize stack_size;
    // TODO: Deal with these later:
    void* p_tls;
    ProcessId* set_tid;
    usize set_tid_size;
    u8 cgroup;
};
