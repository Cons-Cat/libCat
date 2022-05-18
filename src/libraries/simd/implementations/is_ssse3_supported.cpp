#include <cat/simd>

// TODO: Document.
auto is_ssse3_supported() -> bool1 {
    return __builtin_cpu_supports("ssse3");
}
