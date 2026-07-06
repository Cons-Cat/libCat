#include <cat/bit>
#include <cat/linux>

extern "C" {
// Supplied by `src/libcat.ld`. Each translation unit that needs these
// declares its own copy; their linker "addresses" encode the integer
// values computed at link time, decoded via `link_absolute_symbol<T>`.
extern char __cat_tls_memory_size[];  // NOLINT(bugprone-reserved-identifier)
extern char __cat_tls_file_size[];    // NOLINT(bugprone-reserved-identifier)
extern char __cat_tls_alignment[];    // NOLINT(bugprone-reserved-identifier)
extern unsigned char
   __cat_tls_initial_image[];  // NOLINT(bugprone-reserved-identifier)
}

namespace {

// Linker absolute symbols use the "address = constant" encoding, so reading
// them requires a `reinterpret_cast` from the symbol's pointer back to the
// underlying integer. `T` is the libCat integer type the caller wants.
template <typename T>
[[nodiscard, gnu::always_inline]]
inline auto
link_absolute_symbol(char const* const p_symbol_address) -> T {
   return reinterpret_cast<__UINTPTR_TYPE__>(p_symbol_address);
}

}  // namespace

namespace nix::detail {

auto
clone_thread_local_buffer_min_bytes() -> cat::idx {
   cat::idx const tls_memory_size =
      link_absolute_symbol<cat::idx>(__cat_tls_memory_size);
   if (tls_memory_size == 0u) {
      return 0u;
   }

   cat::ualign const minimum_alignment =
      cat::max(32u, link_absolute_symbol<cat::uword>(__cat_tls_alignment));
   cat::idx const minimum_alignment_bytes = cat::idx(minimum_alignment);

   cat::idx const aligned_image_bytes =
      cat::idx(tls_memory_size.raw + minimum_alignment_bytes.raw - 1u);
   cat::idx const cache_line_padded_bytes =
      (tls_memory_size + 63u).raw & ~63ull;

   return aligned_image_bytes > cache_line_padded_bytes
             ? aligned_image_bytes
             : cache_line_padded_bytes;
}

void
install_executable_tls_image_at_thread_pointer(
   cat::uintptr<void> const p_process_address
) {
   cat::idx const memory_size =
      link_absolute_symbol<cat::idx>(__cat_tls_memory_size);
   // Skip this if we have no `thread_local` storage.
   if (memory_size == 0u) {
      return;
   }

   cat::idx const file_size =
      link_absolute_symbol<cat::idx>(__cat_tls_file_size);

   // Image lives directly below `p_process_address`, spanning `memory_size`
   // bytes.
   cat::uintptr<void> const tls_image_low = p_process_address - memory_size;
   // Copy `.tdata` (initialised TLS).
   // TODO: This shouldn't be a scalar copy, but we should hint it's small-ish.
   cat::copy_memory_scalar(
      __cat_tls_initial_image, tls_image_low.get(), file_size
   );
   if (memory_size > file_size) {
      // Zero `.tbss` (the trailing uninitialised tail).
      cat::idx const zero_initialized_size = cat::idx(memory_size - file_size);
      cat::zero_memory_scalar(
         (tls_image_low + file_size).get(), zero_initialized_size
      );
   }
}

// If `thread_local` storage is enabled, `mmap()` a buffer of either
// the link-time determined size or the user's hard-coded
// `CAT_THREAD_LOCAL_SIZE`.
[[gnu::no_stack_protector, clang::disable_sanitizer_instrumentation]]
void
init_parent_process_tls() {
#ifdef CAT_THREAD_LOCAL_SIZE
   cat::idx const tls_memory_size = CAT_THREAD_LOCAL_SIZE;
   cat::ualign const minimum_alignment = 32u;
#else
   cat::idx const tls_memory_size =
      link_absolute_symbol<cat::idx>(__cat_tls_memory_size);
   // Skip the syscall if the executable carries no `.tdata` / `.tbss`.
   if (tls_memory_size == 0u) {
      return;
   }

   cat::ualign const minimum_alignment =
      cat::max(32u, link_absolute_symbol<cat::uword>(__cat_tls_alignment));
#endif

   // TLS buffer layout, addresses ascending.
   // `thread_local` buffer (memory_size bytes):
   //    [tls_thread_pointer - memory_size, tls_thread_pointer)
   //
   // self-slot (must hold tls_thread_pointer itself):
   //    [tls_thread_pointer, tls_thread_pointer + 8)
   //
   // Head and tail padding fills the slab to `slab_bytes`. Over-allocate by
   // `minimum_alignment + 8` so the highest aligned `tls_thread_pointer`
   // inside the slab still leaves room for the self-slot above and the full
   // `thread_local` storage image below.
   cat::idx const slab_bytes = cat::idx(
      tls_memory_size.raw + cat::idx(minimum_alignment).raw + sizeof(void*)
   );

   using enum nix::memory_protection_flags;
   using enum nix::memory_flags;

   void* const p_slab =
      nix::sys_mmap(
         nullptr, slab_bytes, read_write, privately | anonymous,
         // Anonymous mappings require fd = -1 and offset = 0.
         nix::file_descriptor(-1), 0u
      )
         // In the unlikely event that the kernel refused to map the slab, fail
         // fast
         .assert();

   cat::uintptr const buffer_end_inclusive =
      cat::uintptr(p_slab) + slab_bytes - sizeof(void*);
   cat::uintptr const tls_thread_pointer =
      cat::align_down(buffer_end_inclusive, minimum_alignment);

   install_executable_tls_image_at_thread_pointer(tls_thread_pointer);

   // Self-slot: some `thread_local` access patterns (notably the ones
   // `unit_tests`'s `test_thread` triggers) lower to `mov %fs:0, %reg`
   // then dereference relative to the loaded value, so the 8 bytes at
   // `tls_thread_pointer` must equal `tls_thread_pointer` itself once
   // `%fs` is set.
   //
   // LTO sees only a store to freshly-mapped memory followed by an opaque
   // syscall so it dead-code-eliminates the store here. The post-`arch_prctl`
   // read through `%fs:0` depends on the same address, but that isn't visible
   // to the optimiser, so this store must be `volatile`.
   *reinterpret_cast<void* volatile*>(tls_thread_pointer.get()) =
      tls_thread_pointer.get();

   // These arguments can never fail.
   auto _ =
      sys_arch_prctl(arch_prctl_request::set_fs_base, tls_thread_pointer.get());
}

}  // namespace nix::detail
