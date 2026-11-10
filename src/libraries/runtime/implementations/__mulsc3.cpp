#include <cat/detail/complex_builtins.hpp>

#include <cat/runtime>

// Clang calls this for `_Complex float` multiplication.

extern "C" auto
__mulsc3(float real_left, float imag_left, float real_right, float imag_right)
   -> _Complex float {
   return cat::detail::complex_product(
      real_left, imag_left, real_right, imag_right
   );
}
