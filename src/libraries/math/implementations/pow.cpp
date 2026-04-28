#include <cat/math>

// Clang lowers `__builtin_elementwise_pow` to `powf`/`pow` calls. libCat
// provides these symbols without linking libM.
extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `powf` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   powf(float x, float y) -> float {
   return cat::detail::emulated_powf(x, y);
}

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `pow` shim that interposes the libm one. We should prefer
   // that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   pow(double x, double y) -> double {
   return cat::detail::emulated_pow(x, y);
}
