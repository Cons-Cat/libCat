#include <cat/linux>

#if __has_feature(thread_sanitizer)
extern "C" {
// TSan intercepts GlibC's `munmap` to forget the unmapped range, including any
// mutex in it, before the range is mapped again.
auto
munmap(void* _Nonnull p_address, cat::uword length) noexcept -> int;

auto
__errno_location() noexcept  // NOLINT(bugprone-reserved-identifier)
   -> int* _Nonnull;
}
#endif

// `nix::unmap_memory()` wraps the `munmap` Linux syscall.
auto
nix::sys_munmap(cat::span<cat::byte const> memory) noexcept
   -> nix::scaredy_nix<void> {
   poison_memory_region(memory.data(), memory.size());
#if __has_feature(thread_sanitizer)
   if (munmap(const_cast<cat::byte*>(memory.data()), memory.size()) != 0) {
      return static_cast<nix::linux_error>(-*__errno_location());
   }
   return cat::monostate;
#else
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(11, memory.data(), memory.size());
#endif
}
