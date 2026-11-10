#include <cat/detail/round.hpp>

// Without SSE4.1 `roundss`/`roundsd`, Clang lowers the floor and ceil builtins
// to `floor`, `floorf`, `ceil`, and `ceilf` libcalls. LLVM also rewrites some
// `floor` calls, such as those in complex `cat::lgamma()`, into `trunc` and
// `truncf` libcalls.

extern "C" auto
floorf(float argument) -> float {
   return cat::detail::emulated_floor(argument);
}

extern "C" auto
floor(double argument) -> double {
   return cat::detail::emulated_floor(argument);
}

extern "C" auto
ceilf(float argument) -> float {
   return cat::detail::emulated_ceil(argument);
}

extern "C" auto
ceil(double argument) -> double {
   return cat::detail::emulated_ceil(argument);
}

extern "C" auto
truncf(float argument) -> float {
   return cat::detail::emulated_trunc(argument);
}

extern "C" auto
trunc(double argument) -> double {
   return cat::detail::emulated_trunc(argument);
}
