#include <cat/simd>

// TODO: Document.
auto is_sse2_supported() -> bool1 {
    return __builtin_cpu_supports("sse2");
}
