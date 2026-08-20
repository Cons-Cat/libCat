#include <cat/detail/trigonometry.hpp>

// Clang lowers the acos builtins to LLVM intrinsics. The x64 backend emits
// `acos` and `acosf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   acosf(float argument) -> float {
   return cat::detail::emulated_acos(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   acos(double argument) -> double {
   return cat::detail::emulated_acos(argument);
}
