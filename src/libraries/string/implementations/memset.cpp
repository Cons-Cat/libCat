#include <cat/memory>
#include <cat/string>

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `memset` shim that interposes the libc one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   [[clang::no_builtin]]
   auto
   std::memset(void* _Nonnull p_source, int byte_value, __SIZE_TYPE__ bytes)
      -> void* _Nonnull {
   cat::byte fill_byte;
   fill_byte = cat::byte(byte_value);
   cat::detail::fill_memory_impl(
      static_cast<cat::byte* _Nonnull>(p_source), fill_byte, bytes
   );
   return p_source;
}
