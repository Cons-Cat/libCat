#include <cat/simd>

void simd::sfence() {
    __builtin_ia32_sfence();
}
