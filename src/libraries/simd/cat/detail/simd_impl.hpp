#pragma once

// These headers cause a circular `#include` in `<cat/simd>` due to using
// `cat::bitset`, which uses `cat::array`, which uses `cat::simd`.
// This header is `#include`d in `cat::string` to resolve that error.

#include <cat/detail/simd_avx2.hpp>
#include <cat/detail/simd_sse42.hpp>
