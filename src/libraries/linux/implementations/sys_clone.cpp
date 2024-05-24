#include <cat/linux>

auto
nix::sys_clone(clone_flags flags, void* p_stack_entry, process_id* p_parent_id,
               process_id* p_child_id,
               void* p_thread_pointer) -> scaredy_nix<process_id> {
    return syscall<process_id>(56, flags, p_stack_entry, p_parent_id,
                               p_child_id, p_thread_pointer);
}
