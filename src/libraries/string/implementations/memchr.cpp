#include <cat/string>

extern "C"
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
   // ASan and TSan use their own `memchr()` by DLL injection, so we must
   // prefer those.
   [[gnu::visibility("hidden")]]
#endif
   auto
   // NOLINTNEXTLINE(bugprone-std-namespace-modification)
   std::memchr(
      void const* _Nonnull p_haystack, int needle, __SIZE_TYPE__ bytes
   ) noexcept(!__has_feature(cxx_exceptions)) -> void const* _Nonnull {
   cat::str_view const haystack{
      static_cast<char const* _Nonnull>(p_haystack), cat::idx(bytes)
   };
   cat::maybe<cat::idx> const hit =
      haystack.find(static_cast<char>(needle), 0u);
   return hit.has_value()
             ? reinterpret_cast<char const* _Nonnull>(p_haystack) + hit.value()
             : nullptr;
}
