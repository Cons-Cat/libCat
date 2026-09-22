#include <cat/runtime>

// `CAT_THREAD_LOCAL_SIZE == 0` promises no `thread_local` in the executable,
// so nothing would register destructors.
#if !defined(CAT_THREAD_LOCAL_SIZE) || (CAT_THREAD_LOCAL_SIZE) != 0

#include <cat/array>
#include <cat/inplace_allocator>
#include <cat/page_allocator>

namespace {

struct thread_dtor {
   void (*_Nonnull p_invocable)(void* _Nullable);
   void* _Nullable p_arg;
};

struct thread_dtor_page {
   static constexpr cat::idx capacity = 255u;

   thread_dtor_page* _Nullable p_previous;
   cat::idx count;
   cat::array<thread_dtor, capacity> dtors;
};

static_assert(sizeof(thread_dtor_page) == 4'096u);

// Suppress the allocator destructor so Clang does not register it through
// `__cxa_thread_atexit` while this file implements that ABI.
[[clang::no_destroy]]
thread_local cat::inplace_allocator<sizeof(thread_dtor_page)>
   first_page_allocator;

thread_local thread_dtor_page* _Nullable p_thread_dtors;

[[nodiscard]]
auto
allocate_dtor_page() -> thread_dtor_page* _Nullable {
   cat::maybe const p_page =
      p_thread_dtors == nullptr
         ? first_page_allocator.calloc<thread_dtor_page>()
         : cat::page_allocator().calloc<thread_dtor_page>();
   if (p_page.is_empty()) {
      return nullptr;
   }
   p_page.value()->p_previous = p_thread_dtors;
   return p_page.value();
}

void
free_dtor_page(thread_dtor_page* _Nonnull p_page) {
   if (p_page->p_previous == nullptr) {
      first_page_allocator.free(p_page);
      return;
   }
   cat::page_allocator().free(p_page);
}

// Empty means success. A held `int4` is a non-zero Itanium failure code.
[[nodiscard]]
auto
register_thread_dtor(
   void (*_Nonnull p_invocable)(void* _Nullable), void* _Nullable p_arg
) -> cat::maybe_non_zero<cat::int4> {
   if (
      p_thread_dtors == nullptr
      || p_thread_dtors->count == thread_dtor_page::capacity
   ) {
      thread_dtor_page* _Nullable const p_page = allocate_dtor_page();
      if (p_page == nullptr) {
         return -1;
      }
      p_thread_dtors = p_page;
   }

   p_thread_dtors->dtors[p_thread_dtors->count] = {
      .p_invocable = p_invocable,
      .p_arg = p_arg,
   };
   ++p_thread_dtors->count;
   return {};
}

}  // namespace

extern "C" auto
// NOLINTNEXTLINE
__cxa_thread_atexit_impl(
   void (*_Nonnull p_invocable)(void* _Nullable), void* _Nullable p_arg,
   void* _Nullable  // TODO: We need the DSO handle for unloading DLLs.
) -> int {
   return register_thread_dtor(p_invocable, p_arg).value_or_niche().raw;
}

extern "C" auto
// NOLINTNEXTLINE
__cxa_thread_atexit(
   void (*_Nonnull p_invocable)(void* _Nullable), void* _Nullable p_arg,
   void* _Nullable p_dso_handle
) -> int {
   return __cxa_thread_atexit_impl(p_invocable, p_arg, p_dso_handle);
}

extern "C" void
// NOLINTNEXTLINE
__cxa_thread_finalize() {
   while (p_thread_dtors != nullptr) {
      thread_dtor_page* const p_page = p_thread_dtors;
      // This subtraction cannot underflow.
      cat::idx const index = cat::idx(p_page->count - 1u);
      thread_dtor const dtor = p_page->dtors[index];
      p_page->count = index;
      if (index == 0u) {
         p_thread_dtors = p_page->p_previous;
         free_dtor_page(p_page);
      }
      dtor.p_invocable(dtor.p_arg);
   }
}

#endif
