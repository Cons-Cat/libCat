#include <cat/simd>

// TODO: Document.
auto
is_avx2_supported() -> bool {
   return __builtin_cpu_supports("avx2");
}
