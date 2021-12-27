#include <simd.h>

// TODO: Implement more types of fences, such as mfence.
void simd::fence() {
    __builtin_ia32_sfence();
}

void simd::zero_avx_registers() {
    __builtin_ia32_vzeroall();
}

void simd::zero_upper_avx_registers() {
    __builtin_ia32_vzeroupper();
}

auto is_mmx_supported() -> bool {
    return __builtin_cpu_supports("mmx");
}

auto is_sse1_supported() -> bool {
    return __builtin_cpu_supports("sse");
}

auto is_sse2_supported() -> bool {
    return __builtin_cpu_supports("sse2");
}

auto is_sse3_supported() -> bool {
    return __builtin_cpu_supports("sse3");
}

auto is_ssse3_supported() -> bool {
    return __builtin_cpu_supports("ssse3");
}

auto is_sse4_1_supported() -> bool {
    return __builtin_cpu_supports("sse4.1");
}

auto is_sse4_2_supported() -> bool {
    return __builtin_cpu_supports("sse4.2");
}

auto is_avx_supported() -> bool {
    return __builtin_cpu_supports("avx");
}

auto is_avx2_supported() -> bool {
    return __builtin_cpu_supports("avx2");
}

auto is_avx512f_supported() -> bool {
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f");
}

auto is_avx512vl_supported() -> bool {
    return __builtin_cpu_supports("avx512vl");
}
