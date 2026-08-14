#include <cat/math>

// Clang lowers gamma builtins to `lgammaf` and `lgamma`. libCat provides these
// symbols without linking libM.

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   lgammaf(float argument) -> float {
   return static_cast<float>(
      cat::detail::emulated_lgamma(static_cast<double>(argument))
   );
}

extern "C"
#if __has_feature(address_sanitizer)
   [[gnu::visibility("hidden")]]
#endif
   auto
   lgamma(double argument) -> double {
   return cat::detail::emulated_lgamma(argument);
}
