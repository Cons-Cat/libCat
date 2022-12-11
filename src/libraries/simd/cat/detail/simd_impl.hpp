#pragma once

// These headers cause a circular `#include` in `<cat/simd>` due to using
// `cat::Bitset`, which uses `cat::Array`, which uses `cat::Simd`.
// This header is `#include`d in `cat::String` to resolve that error.

#include <cat/detail/simd_avx2.hpp>
#include <cat/detail/simd_sse42.hpp>
