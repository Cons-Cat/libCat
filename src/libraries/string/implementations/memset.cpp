#include <cat/string>

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `memset` shim that interposes the libc one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   [[clang::no_builtin]]
   auto
   std::memset(void* p_source, int byte_value, __SIZE_TYPE__ bytes) -> void* {
#ifdef __clang__
   // TODO: Clang inserts `memset()` calls as some structs' `default`
   // assignment operators. The vectorized `set_memory` causes some
   // problems with that, so this trivial loop is done insead. Find a
   // better solution.
   for (unsigned long long i = 0; i < bytes; ++i) {
      *(static_cast<unsigned char*>(p_source) + i) =
         static_cast<unsigned char>(byte_value);
   }
#else
   cat::set_memory(p_source, static_cast<unsigned char>(byte_value), bytes);
#endif
   return p_source;
}
