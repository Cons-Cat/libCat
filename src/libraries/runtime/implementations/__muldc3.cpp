#include <cat/detail/complex_builtins.hpp>

#include <cat/runtime>

// Clang calls this for `_Complex double` multiplication.

extern "C" auto
__muldc3(
   double real_left, double imag_left, double real_right, double imag_right
) -> _Complex double {
   return cat::detail::complex_product(
      real_left, imag_left, real_right, imag_right
   );
}
