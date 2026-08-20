#include <cat/detail/trigonometry.hpp>

// Clang lowers the sinh builtins to LLVM intrinsics. The x64 backend emits
// `sinh` and `sinhf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   sinhf(float argument) -> float {
   return cat::detail::emulated_sinh(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   sinh(double argument) -> double {
   return cat::detail::emulated_sinh(argument);
}
