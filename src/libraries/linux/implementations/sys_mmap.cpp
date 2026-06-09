#include <cat/linux>

// `map_memory()` wraps the `mmap` Linux syscall. This returns the virtual
// memory address which it has allocated a page at. Pass `nullptr` to let
// the kernel choose.
auto
nix::sys_mmap(void* _Nullable p_start_address, cat::uword bytes_size,
              memory_protection_flags protections, memory_flags flags,
              file_descriptor file_descriptor, cat::uword page_offset)
   -> nix::scaredy_nix<void*> {
   // https://filippo.io/linux-syscall-table/
   nix::scaredy_nix<void*> result = nix::syscall_volatile<void*>(
      9, p_start_address, bytes_size, protections, flags, file_descriptor,
      page_offset * cat::page_size);
   if (result.has_value()) {
      // The kernel hands back fresh, usable memory. `sys_munmap` poisons on
      // free and that shadow can outlive the address, so unpoison the new
      // mapping in case this reuses a previously-freed range.
      unpoison_memory_region(result.value(), bytes_size);
      // mmap returns page-aligned memory on success. Communicate the
      // alignment to the optimizer for callers that invoke sys_mmap directly.
      return cat::assume_aligned<cat::page_size.raw>(result.value());
   }
   return result;
}
