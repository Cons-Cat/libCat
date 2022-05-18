#include <cat/simd>

// TODO: Document.
auto is_avx512vl_supported() -> bool1 {
    return __builtin_cpu_supports("avx512vl");
}
