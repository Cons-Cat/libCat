#include <cat/linux>

extern "C" {
// Supplied by `src/libcat.ld`
extern char __cat_tls_memory_size[];
extern char __cat_tls_file_size[];
extern char __cat_tls_alignment[];
extern unsigned char __cat_tls_initial_image[];
}

namespace {

// Linker-defined absolute symbols use the usual "address = constant" encoding.
[[gnu::always_inline]]
inline auto
link_absolute_symbol_as_cat_index(char const* const p_symbol_address)
   -> cat::idx {
   return reinterpret_cast<__UINTPTR_TYPE__>(p_symbol_address);
}

[[gnu::always_inline]]
inline auto
link_absolute_symbol_as_cat_uword(char const* const p_symbol_address)
   -> cat::uword {
   return reinterpret_cast<__UINTPTR_TYPE__>(p_symbol_address);
}

}  // namespace

namespace nix::detail {

auto
executable_tls_memory_bytes() -> cat::idx {
   return link_absolute_symbol_as_cat_index(__cat_tls_memory_size);
}

auto
executable_tls_file_bytes() -> cat::idx {
   return link_absolute_symbol_as_cat_index(__cat_tls_file_size);
}

auto
executable_tls_alignment_bytes() -> cat::uword {
   return link_absolute_symbol_as_cat_uword(__cat_tls_alignment);
}

auto
clone_thread_local_slab_min_bytes() -> cat::idx {
   cat::idx const memory_size =
      link_absolute_symbol_as_cat_index(__cat_tls_memory_size);
   if (memory_size == 0u) {
      return 0u;
   }
   cat::uword align_want = executable_tls_alignment_bytes();
   if (align_want < 32u) {
      align_want = 32u;
   }
   cat::idx const for_thread_pointer_alignment = memory_size + align_want - 1u;
   cat::idx const for_cache_line = (memory_size + 63u).raw & ~63ull;
   return for_thread_pointer_alignment > for_cache_line
             ? for_thread_pointer_alignment
             : for_cache_line;
}

void
install_executable_tls_image_at_thread_pointer(
   cat::uintptr<void> const thread_pointer) {
   cat::idx const memory_size =
      link_absolute_symbol_as_cat_index(__cat_tls_memory_size);
   if (memory_size == 0u) {
      return;
   }
   cat::idx const file_size =
      link_absolute_symbol_as_cat_index(__cat_tls_file_size);

   cat::uintptr<void> const tls_image_low = thread_pointer - memory_size;
   unsigned char* const p_destination =
      reinterpret_cast<unsigned char*>(tls_image_low.get());
   void const* const p_source = __cat_tls_initial_image;
   __builtin_memcpy(p_destination, p_source, file_size.raw);
   if (memory_size > file_size) {
      cat::idx const zero_initialized_size = memory_size - file_size;
      __builtin_memset(p_destination + file_size.raw, 0,
                       zero_initialized_size.raw);
   }
}

}  // namespace nix::detail
