#include <cat/detail/trigonometry.hpp>

// Clang lowers the asin builtins to LLVM intrinsics. The x64 backend emits
// `asin` and `asinf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   asinf(float argument) -> float {
   return cat::detail::emulated_asin(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   asin(double argument) -> double {
   return cat::detail::emulated_asin(argument);
}
