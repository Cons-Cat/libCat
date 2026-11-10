#include <cat/detail/complex_builtins.hpp>

#include <cat/runtime>

// Clang calls this for `_Complex float` division.

extern "C" auto
__divsc3(float real_left, float imag_left, float real_right, float imag_right)
   -> _Complex float {
   return cat::detail::complex_quotient(
      real_left, imag_left, real_right, imag_right
   );
}
