#include <cat/linux>

struct clone_arguments {
   cat::uint8 flags;
   nix::file_descriptor* _Nullable process_id_file_descriptor;
   nix::process_id* _Nullable child_thread_id;
   nix::process_id* _Nullable parent_thread_id;
   cat::int8 exit_code;
   void* _Nullable p_stack;
   cat::uword stack_size;
   // TODO: Deal with these later:
   void* _Nullable p_tls;
   nix::process_id* _Nullable set_tid;
   cat::uword set_tid_size;
   cat::uint8 cgroup;
};
