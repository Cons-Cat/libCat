#include <cat/detail/complex_builtins.hpp>

#include <cat/runtime>

// Clang calls this for `_Complex double` division.

extern "C" auto
__divdc3(
   double real_left, double imag_left, double real_right, double imag_right
) -> _Complex double {
   return cat::detail::complex_quotient(
      real_left, imag_left, real_right, imag_right
   );
}
