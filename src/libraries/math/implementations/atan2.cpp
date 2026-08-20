#include <cat/detail/trigonometry.hpp>

// Clang lowers the atan2 builtins to LLVM intrinsics. The x64 backend emits
// `atan2` and `atan2f` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   atan2f(float y, float x) -> float {
   return cat::detail::emulated_atan2(y, x);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   atan2(double y, double x) -> double {
   return cat::detail::emulated_atan2(y, x);
}
