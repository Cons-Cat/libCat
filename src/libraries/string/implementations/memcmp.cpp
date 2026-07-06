#include <cat/memory>
#include <cat/string>

namespace {

[[nodiscard]]
auto
compare_memory_as_memcmp_int(
   void const* _Nonnull p_lhs, void const* _Nonnull p_rhs, __SIZE_TYPE__ bytes
) -> int {
   auto const order = cat::compare_memory(p_lhs, p_rhs, bytes);
   if (order < 0) {
      return -1;
   }
   if (order > 0) {
      return 1;
   }
   return 0;
}

}  // namespace

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `memcmp` shim that interposes the libc one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   std::memcmp(
      void const* _Nonnull p_lhs, void const* _Nonnull p_rhs,
      __SIZE_TYPE__ bytes
   ) -> int {
   return compare_memory_as_memcmp_int(p_lhs, p_rhs, bytes);
}

// Clang lowers equality-only comparisons such as `__builtin_memcmp(...) == 0`
// into `bcmp` in optimized builds. `bcmp` returns zero when the regions are
// equal and any non-zero value otherwise.
extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   bcmp(
      void const* _Nonnull p_lhs, void const* _Nonnull p_rhs,
      __SIZE_TYPE__ bytes
   ) -> int {
   return compare_memory_as_memcmp_int(p_lhs, p_rhs, bytes) != 0;
}
