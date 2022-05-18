#include <cat/simd>

// TODO: Document.
void simd::zero_avx_registers() {
    __builtin_ia32_vzeroall();
}
