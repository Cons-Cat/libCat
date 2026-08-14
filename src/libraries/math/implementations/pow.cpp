#include <cat/detail/pow.hpp>

// Clang lowers `__builtin_elementwise_pow` to `powf`/`pow` calls. libCat
// provides these symbols without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `powf` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   powf(float base, float exponent) -> float {
   return cat::detail::emulated_pow(base, exponent);
}

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `powf` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   pow(double base, double exponent) -> double {
   return cat::detail::emulated_pow(base, exponent);
}
