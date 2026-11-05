#include <cat/memory>
#include <cat/string>

extern "C"
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
   // ASan and TSan use their own `memset()` by DLL injection, so we must
   // prefer those.
   [[gnu::visibility("hidden")]]
#endif
   auto
   // NOLINTNEXTLINE(bugprone-std-namespace-modification)
   std::memset(
      void* _Nonnull p_source, int byte_value, __SIZE_TYPE__ bytes
   ) noexcept(!__has_feature(cxx_exceptions)) -> void* _Nonnull {
   cat::byte fill_byte;
   fill_byte = cat::byte(byte_value);
   cat::detail::fill_memory_impl(
      static_cast<cat::byte* _Nonnull>(p_source), fill_byte, bytes
   );
   return p_source;
}
