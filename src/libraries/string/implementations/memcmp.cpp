#include <cat/memory>
#include <cat/string>

extern "C"
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
   // ASan and TSan use their own `memcmp()` by DLL injection, so we must
   // prefer those.
   [[gnu::visibility("hidden")]]
#endif
   auto
   // NOLINTNEXTLINE(bugprone-std-namespace-modification)
   std::memcmp(
      void const* _Nonnull p_lhs, void const* _Nonnull p_rhs,
      __SIZE_TYPE__ bytes
   ) noexcept -> int {
   cat::idx const offset = cat::find_disjoint_memory(p_lhs, p_rhs, bytes);
   if (offset == bytes) {
      return 0;
   }

   unsigned char const left =
      static_cast<unsigned char const* _Nonnull>(p_lhs)[offset];
   unsigned char const right =
      static_cast<unsigned char const* _Nonnull>(p_rhs)[offset];
   return left < right ? -1 : 1;
}

// Clang lowers equality-only comparisons such as `__builtin_memcmp(...) == 0`
// into `bcmp`, so it's implemented too.
extern "C"
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   bcmp(
      void const* _Nonnull p_lhs, void const* _Nonnull p_rhs,
      __SIZE_TYPE__ bytes
   ) noexcept -> int {
   return cat::is_memory_equal(p_lhs, p_rhs, bytes) ? 0 : 1;
}
