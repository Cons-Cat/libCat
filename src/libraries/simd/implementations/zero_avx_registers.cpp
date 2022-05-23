#include <cat/simd>

// TODO: Document.
void cat::zero_avx_registers() {
    __builtin_ia32_vzeroall();
}
