#include <cat/detail/trigonometry.hpp>

// Clang lowers the cosh builtins to LLVM intrinsics. The x64 backend emits
// `cosh` and `coshf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   coshf(float argument) -> float {
   return cat::detail::emulated_cosh(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   cosh(double argument) -> double {
   return cat::detail::emulated_cosh(argument);
}
