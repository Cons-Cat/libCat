#include <cat/detail/fma.hpp>

// Without `-mfma`, Clang lowers scalar and vector FMA builtins on `float` to
// `fmaf` libcalls. libCat provides this symbol without linking libM.

extern "C" auto
fmaf(float value, float multiplier, float addend) -> float {
   return cat::detail::emulated_fma(value, multiplier, addend);
}
