#include <cat/linux>

struct CloneArguments {
    uint8 flags;
    nix::FileDescriptor* process_id_file_descriptor;
    nix::ProcessId* child_thread_id;
    nix::ProcessId* parent_thread_id;
    int8 exit_code;
    void* p_stack;
    usize stack_size;
    // TODO: Deal with these later:
    void* p_tls;
    nix::ProcessId* set_tid;
    usize set_tid_size;
    uint8 cgroup;
};
