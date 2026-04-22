#include <cat/string>

// `__SIZE_TYPE__` is a GCC macro.
extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `memcpy` shim that interposes the libc one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   std::memcpy(void* p_destination, void const* p_source, __SIZE_TYPE__ bytes)
      -> void* {
   cat::copy_memory(p_source, p_destination, bytes);
   return p_destination;
}
