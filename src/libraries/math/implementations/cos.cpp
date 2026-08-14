#include <cat/detail/trigonometry.hpp>

// Clang lowers the cos builtins to LLVM intrinsics. The x64 backend emits
// `cos` and `cosf` libcalls, which libCat provides without linking libM.
// ASan does not intercept these symbols.

extern "C" auto
cosf(float argument) -> float {
   return cat::detail::emulated_cos(argument);
}

extern "C" auto
cos(double argument) -> double {
   return cat::detail::emulated_cos(argument);
}
