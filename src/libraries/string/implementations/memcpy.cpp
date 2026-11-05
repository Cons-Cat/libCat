#include <cat/string>

extern "C"
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
   // ASan and TSan use their own `memcpy()` by DLL injection, so we must
   // prefer those.
   [[gnu::visibility("hidden")]]
#endif
   auto
   // NOLINTNEXTLINE(bugprone-std-namespace-modification)
   std::memcpy(
      void* _Nonnull __restrict p_destination,
      void const* _Nonnull __restrict p_source, __SIZE_TYPE__ bytes
   ) noexcept(!__has_feature(cxx_exceptions)) -> void* _Nonnull {
   cat::copy_memory(p_source, p_destination, bytes);
   return p_destination;
}
