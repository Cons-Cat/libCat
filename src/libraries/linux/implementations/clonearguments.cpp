#include <cat/linux>

struct clone_arguments {
    cat::uint8 flags;
    nix::file_descriptor* process_id_file_descriptor;
    nix::process_id* child_thread_id;
    nix::process_id* parent_thread_id;
    cat::int8 exit_code;
    void* p_stack;
    cat::uword stack_size;
    // TODO: Deal with these later:
    void* p_tls;
    nix::process_id* set_tid;
    cat::uword set_tid_size;
    cat::uint8 cgroup;
};
