#include <cat/math>

// Clang lowers `__builtin_sqrt` / `__builtin_sqrtf` (and sometimes
// `__builtin_elementwise_sqrt`) to `sqrt` / `sqrtf`. libCat provides these
// symbols without linking `libm`.
extern "C" auto
sqrtf(float argument) -> float {
   return cat::detail::emulated_sqrtf(argument);
}

extern "C" auto
sqrt(double argument) -> double {
   return cat::detail::emulated_sqrt(argument);
}
