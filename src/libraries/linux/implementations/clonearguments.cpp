#include <cat/linux>

struct CloneArguments {
    uint8 flags;
    nix::file_descriptor* process_id_file_descriptor;
    nix::process_id* child_thread_id;
    nix::process_id* parent_thread_id;
    int8 exit_code;
    void* p_stack;
    uword stack_size;
    // TODO: Deal with these later:
    void* p_tls;
    nix::process_id* set_tid;
    uword set_tid_size;
    uint8 cgroup;
};
