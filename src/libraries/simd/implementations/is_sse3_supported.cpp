#include <cat/simd>

// TODO: Document.
auto is_sse3_supported() -> bool1 {
    return __builtin_cpu_supports("sse3");
}
