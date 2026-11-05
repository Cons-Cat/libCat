#include <cat/detail/thread_local.hpp>

namespace nix::detail {

auto
thread_local_bytes_below_thread_pointer() noexcept -> cat::idx {
   cat::idx const memory_size =
      link_absolute_symbol<cat::idx>(__cat_tls_memory_size);
#if defined(CAT_BUILD_SHARED) && !defined(CAT_USE_PTHREAD)
   // The dynamic loader puts this library's block below the executable's.
   if (library_block_offset != 0) {
      return cat::max(memory_size, cat::idx(-library_block_offset));
   }
#endif
   return memory_size;
}

auto
clone_thread_local_buffer_min_bytes() noexcept(!__has_feature(cxx_exceptions))
   -> cat::idx {
   cat::idx const tls_memory_size = thread_local_bytes_below_thread_pointer();
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
) noexcept(!__has_feature(cxx_exceptions)) {
#if defined(CAT_BUILD_SHARED) && !defined(CAT_USE_PTHREAD)
   install_library_tls_image_at_thread_pointer(p_process_address);
#endif

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

// If `thread_local` storage is enabled, page allocate a buffer of either
// the link-time determined size or the user's hard-coded
// `CAT_THREAD_LOCAL_SIZE`.
[[gnu::no_stack_protector, clang::disable_sanitizer_instrumentation]]
void
init_parent_process_tls() noexcept(!__has_feature(cxx_exceptions)) {
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
   cat::idx const slab_bytes =
      cat::idx(tls_memory_size + minimum_alignment + sizeof(void*));

   using enum nix::memory_protection_flags;
   using enum nix::memory_flags;

   cat::byte* const p_slab =
      nix::sys_mmap(
         nullptr, slab_bytes, read_write, privately | anonymous,
         // Anonymous mappings require fd = -1 and offset = 0.
         nix::invalid_file_descriptor, 0u
      )
         // In the unlikely event that the kernel refused to map the slab, fail
         // fast
         .assert();

   cat::uintptr const buffer_end_inclusive =
      cat::uintptr(p_slab) + slab_bytes - sizeof(void*);
   cat::uintptr const tls_thread_pointer =
      cat::align_down(buffer_end_inclusive, minimum_alignment);

   install_executable_tls_image_at_thread_pointer(tls_thread_pointer);

   // The base of a thread pointer must equal its own address.
   *reinterpret_cast<void* volatile*>(tls_thread_pointer.get()) =
      tls_thread_pointer.get();

   // These arguments can never fail.
   auto _ =
      sys_arch_prctl(arch_prctl_request::set_fs_base, tls_thread_pointer.get());
}

}  // namespace nix::detail
