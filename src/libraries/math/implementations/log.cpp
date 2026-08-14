#include <cat/detail/log.hpp>

// Clang lowers these builtins to LLVM log intrinsics. The x64 backend emits
// `log` and `logf` libcalls, which libCat provides without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `logf` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   logf(float argument) -> float {
   return cat::detail::emulated_log(argument);
}

extern "C"
#if __has_feature(address_sanitizer)
   // asan has its own `log` shim that interposes the libm one. We should
   // prefer that.
   [[gnu::visibility("hidden")]]
#endif
   auto
   log(double argument) -> double {
   return cat::detail::emulated_log(argument);
}
