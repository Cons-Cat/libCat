#include <cat/simd>

// TODO: Document.
auto
is_sse1_supported() -> bool {
    return __builtin_cpu_supports("sse");
}
