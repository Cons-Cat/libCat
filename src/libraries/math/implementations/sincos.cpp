#include <cat/detail/trigonometry.hpp>

// Clang lowers the sincos builtins to LLVM intrinsics. The x64 backend emits
// `sincos` and `sincosf` libcalls, which libCat provides without linking libM.
// ASan does not intercept these symbols.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   sincosf(float argument, float* _Nonnull p_sine, float* _Nonnull p_cosine)
      -> void {
   *p_sine = cat::detail::emulated_sin(argument);
   *p_cosine = cat::detail::emulated_cos(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   sincos(double argument, double* _Nonnull p_sine, double* _Nonnull p_cosine)
      -> void {
   *p_sine = cat::detail::emulated_sin(argument);
   *p_cosine = cat::detail::emulated_cos(argument);
}
