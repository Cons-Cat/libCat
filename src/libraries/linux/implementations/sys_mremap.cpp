#include <cat/linux>

// `nix::sys_mremap()` wraps the `mremap` Linux syscall. Without
// `mremap_flags::may_move` the kernel resizes the mapping at the same address
// or fails, which makes it an in-place reallocation primitive.
auto
nix::sys_mremap(void* _Nonnull p_old_address, cat::idx old_size,
                cat::idx new_size, mremap_flags flags)
   -> nix::scaredy_nix<void*> {
   // https://filippo.io/linux-syscall-table/
   nix::scaredy_nix<void*> result = nix::syscall_volatile<void*>(
      25, p_old_address, old_size, new_size, flags);
   if (result.has_value()) {
      // The kernel may relocate the mapping. Poison the abandoned source so
      // a stale pointer is caught, and unpoison the live destination, since
      // `sys_munmap` poisons on free and that shadow can outlive the address.
      if (result.value() != p_old_address) {
         poison_memory_region(p_old_address, old_size);
      }
      unpoison_memory_region(result.value(), new_size);
   }
   return result;
}
