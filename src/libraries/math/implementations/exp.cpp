#include <cat/detail/exp.hpp>

// Clang lowers these builtins to LLVM exp intrinsics. The x64 backend emits
// `exp` and `expf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `expf` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   expf(float argument) -> float {
   return cat::detail::emulated_exp(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `exp` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   exp(double argument) -> double {
   return cat::detail::emulated_exp(argument);
}
