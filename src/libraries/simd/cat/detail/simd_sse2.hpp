#pragma once

// SSE2-class movmsk helpers (`simd_sse2_movmsk.hpp`) and
// `unary_full<op_rsqrt>` (`simd_sse2_unary_full.hpp`). Included from
// `<cat/simd>` after `simd_mask`. `simd_avx2_unary_full.hpp` covers the
// matching AVX2 hooks.

#include "cat/detail/simd_sse2_movmsk.hpp"
#include "cat/detail/simd_sse2_unary_full.hpp"
