#include <cat/math>

// Clang lowers `__builtin_elementwise_pow` to `powf` / `pow` calls. libCat
// provides these symbols without linking `libm`.
extern "C" auto
powf(float x, float y) -> float {
   return cat::detail::emulated_powf(x, y);
}

extern "C" auto
pow(double x, double y) -> double {
   return cat::detail::emulated_pow(x, y);
}
