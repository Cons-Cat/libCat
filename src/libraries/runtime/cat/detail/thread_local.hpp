// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Symbols and helpers shared by libCat's `thread_local` runtime:
//
//   `tls_local_exec.cpp`  The executable's block at a fixed offset below the
//                         thread pointer, and the slabs that carry it.
//
//   `tls_descriptor.cpp`  `libcat.so`'s block, which it reaches through TLS
//                         descriptors.
//
// Initial-exec needs no runtime of its own. The linker relaxes it to local-exec
// in the executable, and the dynamic loader resolves it for `libcat.so`'s
// block.

#include <cat/bit>
#include <cat/linux>

extern "C" {
// Supplied by `src/libcat.ld`. Their linker "addresses" encode the integer
// values computed at link time, decoded via `link_absolute_symbol<T>`. These
// describe the executable's `thread_local` storage image.
extern char __cat_tls_memory_size[];  // NOLINT(bugprone-reserved-identifier)
extern char __cat_tls_file_size[];    // NOLINT(bugprone-reserved-identifier)
extern char __cat_tls_alignment[];    // NOLINT(bugprone-reserved-identifier)
extern unsigned char
   __cat_tls_initial_image[];  // NOLINT(bugprone-reserved-identifier)
}

namespace nix::detail {

// Linker absolute symbols use the "address = constant" encoding, so reading
// them requires a `reinterpret_cast` from the symbol's pointer back to the
// underlying integer. `T` is the libCat integer type the caller wants.
template <typename T>
[[nodiscard, gnu::always_inline]]
inline auto
link_absolute_symbol(char const* _Nonnull const p_symbol_address) noexcept
   -> T {
   return reinterpret_cast<__UINTPTR_TYPE__>(p_symbol_address);
}

}  // namespace nix::detail

#if defined(CAT_BUILD_SHARED) && !defined(CAT_USE_PTHREAD)
extern "C" {
// Supplied by `src/libcat.ld`. These describe this library's own
// `thread_local` storage image.
extern char
   __cat_module_tls_memory_size[];  // NOLINT(bugprone-reserved-identifier)
extern char
   __cat_module_tls_file_size[];  // NOLINT(bugprone-reserved-identifier)
extern unsigned char
   __cat_module_tls_initial_image[];  // NOLINT(bugprone-reserved-identifier)
}

namespace nix::detail {

// The offset of `libcat.so`'s block from the thread pointer, or 0 if the
// dynamic loader did not put it in the static block below every thread
// pointer. This is hidden so that the executable can not preempt it.
[[gnu::visibility("hidden")]]
inline constinit cat::iword library_block_offset = 0;

// Copy `libcat.so`'s `thread_local` storage image into a thread whose thread
// pointer is `thread_pointer`, if `library_block_offset` is not 0.
[[gnu::visibility("hidden")]]
void
install_library_tls_image_at_thread_pointer(
   cat::uintptr<void> thread_pointer
) noexcept(!__has_feature(cxx_exceptions));

}  // namespace nix::detail
#endif
