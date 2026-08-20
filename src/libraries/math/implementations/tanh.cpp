#include <cat/detail/trigonometry.hpp>

// Clang lowers the tanh builtins to LLVM intrinsics. The x64 backend emits
// `tanh` and `tanhf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   tanhf(float argument) -> float {
   return cat::detail::emulated_tanh(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   tanh(double argument) -> double {
   return cat::detail::emulated_tanh(argument);
}
