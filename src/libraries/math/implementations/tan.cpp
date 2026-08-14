#include <cat/detail/trigonometry.hpp>

// Clang lowers the tan builtins to LLVM intrinsics. The x64 backend emits
// `tan` and `tanf` libcalls, which libCat provides without linking libM.
// ASan does not intercept these symbols.

extern "C" auto
tanf(float argument) -> float {
   return cat::detail::emulated_tan(argument);
}

extern "C" auto
tan(double argument) -> double {
   return cat::detail::emulated_tan(argument);
}
