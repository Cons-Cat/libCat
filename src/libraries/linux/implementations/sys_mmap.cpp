#include <cat/linux>

#if __has_feature(thread_sanitizer)
extern "C" {
// TSan intercepts POSIX's `mmap` to reset what it knows about the new range,
// which may have been mapped and unmapped before.
auto
mmap(
   void* _Nullable p_address, cat::uword length, int protections, int flags,
   int file_descriptor, cat::iword offset
) noexcept -> void*;

auto
__errno_location() noexcept  // NOLINT(bugprone-reserved-identifier)
   -> int* _Nonnull;
}
#endif

// `map_memory()` wraps the `mmap` Linux syscall. This returns the virtual
// memory address which it has allocated a page at. Pass `nullptr` to let
// the kernel choose.
auto
nix::sys_mmap(
   void* _Nullable p_start_address, cat::uword bytes_size,
   memory_protection_flags protections, memory_flags flags,
   file_descriptor file_descriptor, cat::uword page_offset
) -> nix::scaredy_nix<cat::byte*> {
#if __has_feature(thread_sanitizer)
   void* const p_mapping = mmap(
      p_start_address, bytes_size, cat::to_underlying(protections),
      cat::to_underlying(flags),
      __builtin_bit_cast(int, file_descriptor.value.raw),
      cat::iword(page_offset * cat::page_size)
   );
   // GlibC's `MAP_FAILED`.
   nix::scaredy_nix<cat::byte*> result =
      p_mapping == __builtin_bit_cast(void*, cat::iword(-1))
         ? nix::scaredy_nix<cat::byte*>(
              static_cast<nix::linux_error>(-*__errno_location())
           )
         : nix::scaredy_nix<cat::byte*>(static_cast<cat::byte*>(p_mapping));
#else
   // https://filippo.io/linux-syscall-table/
   nix::scaredy_nix<cat::byte*> result = nix::syscall_volatile<cat::byte*>(
      9, p_start_address, bytes_size, protections, flags, file_descriptor,
      page_offset * cat::page_size
   );
#endif
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
