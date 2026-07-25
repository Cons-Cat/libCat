#include <cat/string>

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   // NOLINTNEXTLINE(bugprone-std-namespace-modification)
   std::memmove(
      void* _Nonnull p_destination, void const* _Nonnull p_source,
      __SIZE_TYPE__ bytes
   ) -> void* _Nonnull {
   cat::copy_memory_backward(p_source, p_destination, bytes);
   return p_destination;
}
