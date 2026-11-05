#include <cat/detail/thread_local.hpp>

#if defined(CAT_BUILD_SHARED) && !defined(CAT_USE_PTHREAD)
// Shared/dynamically linked libCat programs need to support POSIX runtimes such
// as GlibC and Musl. This file provides minimal code to interoperate with their
// thread local storage through `cat::thread` and `nix::process` when we aren't
// using pthreads instead.
//
// `libcat.so` is compiled with `-mtls-dialect=gnu2`, so it reaches
// `thread_local` storage through TLS descriptors. When the dynamic loader puts
// a module's block in the static block below every thread pointer, which it
// does for modules loaded at startup, the descriptor's resolver only returns
// the block's offset from the thread pointer.
//
// That works in a raw `sys_clone` child as long as its slab carries
// `libcat.so`'s block at the same offset, which
// `install_library_tls_image_at_thread_pointer` does. Otherwise, such as when
// `clang-repl` `dlopen()`s `libcat.so`, the resolver needs a libC's thread
// control block, which a raw `sys_clone` child lacks. Here we provide handles
// into a libC's POSIX thread control block.

extern "C" {
// GlibC's dynamic loader exports this as `GLIBC_PRIVATE`. It measures the
// static `thread_local` block that every thread carries below its thread
// pointer. Without this, GlibC can't be used as a runtime, but other libC's
// don't need it.
void
_dl_get_tls_static_info(  // NOLINT(bugprone-reserved-identifier)
   cat::idx* _Nonnull p_size, cat::idx* _Nonnull p_alignment
) noexcept;

// The linker defines this at the start of this library's `thread_local` block.
[[gnu::visibility("hidden")]]
extern constinit thread_local char
   _TLS_MODULE_BASE_;  // NOLINT(bugprone-reserved-identifier)
}

namespace {

[[gnu::constructor]]
void
find_library_block_offset() noexcept {
   if (
      nix::detail::link_absolute_symbol<cat::idx>(__cat_module_tls_memory_size)
      == 0u
   ) {
      return;
   }

   // The descriptor of `_TLS_MODULE_BASE_` resolves to the offset of this
   // library's block from the thread pointer.
   cat::iword const offset =
      &_TLS_MODULE_BASE_ - static_cast<char*>(__builtin_thread_pointer());

   cat::idx static_size;
   cat::idx static_alignment;
   _dl_get_tls_static_info(&static_size, &static_alignment);
   if (offset < 0 && -offset <= static_size) {
      nix::detail::library_block_offset = offset;
   }
}

}  // namespace

void
nix::detail::install_library_tls_image_at_thread_pointer(
   cat::uintptr<void> const thread_pointer
) noexcept(!__has_feature(cxx_exceptions)) {
   if (library_block_offset == 0) {
      return;
   }

   cat::idx const module_memory_size =
      link_absolute_symbol<cat::idx>(__cat_module_tls_memory_size);
   cat::idx const module_file_size =
      link_absolute_symbol<cat::idx>(__cat_module_tls_file_size);
   cat::uintptr<void> const module_block =
      thread_pointer + library_block_offset;

   // TODO: This shouldn't be a scalar copy, but we should hint it's small-ish.
   cat::copy_memory_scalar(
      __cat_module_tls_initial_image, module_block.get(), module_file_size
   );

   cat::zero_memory_scalar(
      (module_block + module_file_size).get(),
      cat::idx(module_memory_size - module_file_size)
   );
}
#endif
