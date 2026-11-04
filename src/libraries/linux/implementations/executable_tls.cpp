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

#if defined(CAT_BUILD_SHARED) && !defined(CAT_USE_PTHREAD)
// `libcat.so` reaches `thread_local` storage through `__tls_get_addr`, both its
// own and the executable's, which can preempt `libcat.so`'s inline
// `thread_local` variables. GlibC's needs GlibC's thread control block, which a
// raw `clone` child lacks, so `libcat.so` is linked with
// `--wrap=__tls_get_addr` to call this one instead. When the dynamic loader put
// a module's block at a fixed offset from every thread pointer, which it does
// for modules loaded at startup, this adds that offset to the thread pointer.
// Otherwise, such as when `clang-repl` `dlopen`s `libcat.so`, this forwards to
// GlibC's.

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

namespace {

struct tls_index {
   cat::uword module;
   cat::uword offset;
};

// A module whose `thread_local` block is at a fixed offset from the thread
// pointer.
struct static_module {
   cat::uword id = 0u;
   // 0 if the block is not at a fixed offset.
   cat::iword block_offset = 0;
};

// The dynamic loader numbers the executable's block 1.
constinit static_module executable_module{.id = 1u};
constinit static_module library_module;

// Taking this address while `probing` records this library's module id. It has
// internal linkage, so the executable can not preempt it.
constinit thread_local char module_probe = 0;
constinit bool probing = false;

[[nodiscard, gnu::always_inline]]
inline auto
thread_pointer() -> cat::uintptr<void> {
   void* p_thread_pointer;
   asm("movq %%fs:0, %0"
       : "=r"(p_thread_pointer));
   return p_thread_pointer;
}

}  // namespace

extern "C" {
// GlibC's `__tls_get_addr`.
auto
__real___tls_get_addr(  // NOLINT(bugprone-reserved-identifier)
   tls_index* _Nonnull p_index
) -> void* _Nonnull;

// GlibC's dynamic loader exports this as `GLIBC_PRIVATE`. It measures the
// static `thread_local` block that every thread carries below its thread
// pointer.
void
_dl_get_tls_static_info(  // NOLINT(bugprone-reserved-identifier)
   cat::idx* _Nonnull p_size, cat::idx* _Nonnull p_alignment
) noexcept;

[[gnu::visibility("hidden")]]
auto
__wrap___tls_get_addr(  // NOLINT(bugprone-reserved-identifier)
   tls_index* _Nonnull p_index
) -> void* _Nonnull {
   for (static_module const& module : {library_module, executable_module}) {
      if (module.block_offset != 0 && module.id == p_index->module) {
         return (thread_pointer() + module.block_offset + p_index->offset)
            .get();
      }
   }
   if (probing) {
      library_module.id = p_index->module;
   }
   return __real___tls_get_addr(p_index);
}
}

namespace {

// The offset of `module`'s block from the thread pointer, or 0 if it is not in
// the static block that every thread carries.
[[nodiscard]]
auto
static_block_offset(cat::uword module, cat::idx static_size) -> cat::iword {
   tls_index index{.module = module, .offset = 0u};
   cat::uintptr<void> const block = __real___tls_get_addr(&index);
   cat::uintptr<void> const pointer = thread_pointer();
   if (block < pointer && pointer - block <= static_size) {
      return -cat::iword(pointer - block);
   }
   return 0;
}

[[gnu::constructor]]
void
find_static_modules() {
   probing = true;
   char* p_probe = &module_probe;
   asm volatile(""
                :
                : "r"(p_probe)
                : "memory");
   probing = false;

   cat::idx static_size;
   cat::idx static_alignment;
   _dl_get_tls_static_info(&static_size, &static_alignment);
   library_module.block_offset =
      static_block_offset(library_module.id, static_size);

   // A raw `clone` child's slab only matches the executable's block where
   // `install_executable_tls_image_at_thread_pointer` puts it.
   cat::idx const executable_size =
      link_absolute_symbol<cat::idx>(__cat_tls_memory_size);
   if (executable_size != 0u && library_module.id != executable_module.id) {
      cat::iword const offset =
         static_block_offset(executable_module.id, static_size);
      if (offset == -cat::iword(executable_size)) {
         executable_module.block_offset = offset;
      }
   }
}

}  // namespace
#endif

namespace nix::detail {

auto
thread_local_bytes_below_thread_pointer() -> cat::idx {
   cat::idx const memory_size =
      link_absolute_symbol<cat::idx>(__cat_tls_memory_size);
#if defined(CAT_BUILD_SHARED) && !defined(CAT_USE_PTHREAD)
   // The dynamic loader puts this library's block below the executable's.
   if (library_module.block_offset != 0) {
      return cat::max(memory_size, cat::idx(-library_module.block_offset));
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
) {
#if defined(CAT_BUILD_SHARED) && !defined(CAT_USE_PTHREAD)
   if (library_module.block_offset != 0) {
      cat::idx const module_memory_size =
         link_absolute_symbol<cat::idx>(__cat_module_tls_memory_size);
      cat::idx const module_file_size =
         link_absolute_symbol<cat::idx>(__cat_module_tls_file_size);
      cat::uintptr<void> const module_block =
         p_process_address + library_module.block_offset;
      cat::copy_memory_scalar(
         __cat_module_tls_initial_image, module_block.get(), module_file_size
      );
      cat::zero_memory_scalar(
         (module_block + module_file_size).get(),
         cat::idx(module_memory_size - module_file_size)
      );
   }
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

// If `thread_local` storage is enabled, `mmap()` a buffer of either
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
   cat::idx const slab_bytes = cat::idx(
      tls_memory_size.raw + cat::idx(minimum_alignment).raw + sizeof(void*)
   );

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
