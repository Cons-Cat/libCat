#include <cat/simd>

void
cat::sfence() {
   __builtin_ia32_sfence();
}
