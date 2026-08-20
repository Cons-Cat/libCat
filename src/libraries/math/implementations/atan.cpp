#include <cat/detail/trigonometry.hpp>

// Clang lowers the atan builtins to LLVM intrinsics. The x64 backend emits
// `atan` and `atanf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   atanf(float argument) -> float {
   return cat::detail::emulated_atan(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   atan(double argument) -> double {
   return cat::detail::emulated_atan(argument);
}
