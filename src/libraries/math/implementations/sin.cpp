#include <cat/detail/trigonometry.hpp>

// Clang lowers the sin builtins to LLVM intrinsics. The x64 backend emits
// `sin` and `sinf` libcalls, which libCat provides without linking libM.
// ASan does not intercept these symbols.

extern "C" auto
sinf(float argument) -> float {
   return cat::detail::emulated_sin(argument);
}

extern "C" auto
sin(double argument) -> double {
   return cat::detail::emulated_sin(argument);
}
