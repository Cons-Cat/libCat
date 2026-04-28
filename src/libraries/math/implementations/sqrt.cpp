#include <cat/math>

// Clang lowers `__builtin_sqrt`/`__builtin_sqrtf` (and sometimes
// `__builtin_elementwise_sqrt`) to `sqrt`/`sqrtf`. libCat provides these
// symbols without linking libM.
extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `sqrtf` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   sqrtf(float argument) -> float {
   return cat::detail::emulated_sqrtf(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `sqrt` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   sqrt(double argument) -> double {
   return cat::detail::emulated_sqrt(argument);
}
