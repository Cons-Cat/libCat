#pragma once

// Lane-width sugar for `simd<T, Abi>`, included only from `<cat/simd>` inside
// `namespace cat`.

// TODO: Add Clang `__bf16` vectors.

using int1x2 = fixed_size_simd<int1, 2u>;
using int1x4 = fixed_size_simd<int1, 4u>;
using int1x8 = fixed_size_simd<int1, 8u>;
using int1x16 = fixed_size_simd<int1, 16u>;
using int1x32 = fixed_size_simd<int1, 32u>;
using int1x_ = native_simd<int1>;

using uint1x2 = fixed_size_simd<uint1, 2u>;
using uint1x4 = fixed_size_simd<uint1, 4u>;
using uint1x8 = fixed_size_simd<uint1, 8u>;
using uint1x16 = fixed_size_simd<uint1, 16u>;
using uint1x32 = fixed_size_simd<uint1, 32u>;
using uint1x_ = native_simd<uint1>;

using int_unalign_1x2 = fixed_size_unaligned_simd<int1, 2u>;
using int_unalign_1x4 = fixed_size_unaligned_simd<int1, 4u>;
using int_unalign_1x8 = fixed_size_unaligned_simd<int1, 8u>;
using int_unalign_1x16 = fixed_size_unaligned_simd<int1, 16u>;
using int_unalign_1x32 = fixed_size_unaligned_simd<int1, 32u>;
using int_unalign_1x_ = native_unaligned_simd<int1>;

using uint_unalign_1x2 = fixed_size_unaligned_simd<uint1, 2u>;
using uint_unalign_1x4 = fixed_size_unaligned_simd<uint1, 4u>;
using uint_unalign_1x8 = fixed_size_unaligned_simd<uint1, 8u>;
using uint_unalign_1x16 = fixed_size_unaligned_simd<uint1, 16u>;
using uint_unalign_1x32 = fixed_size_unaligned_simd<uint1, 32u>;
using uint_unalign_1x_ = native_unaligned_simd<uint1>;

// Same portable shapes as `int4x16`-style aliases. Fixed lane counts versus
// `native_abi` for the widest tag the build selects as `native_simd<char>`.
// TODO: Think over the string vectorization API.
// TODO: Support `char2x_` vector family.
// strings need their own vectors.
using char1x16 = fixed_size_simd<char, 16u>;
using char1x32 = fixed_size_simd<char, 32u>;
using char1x_ = native_simd<char>;

using char_unalign_1x16 = fixed_size_unaligned_simd<char, 16u>;
using char_unalign_1x32 = fixed_size_unaligned_simd<char, 32u>;
using char_unalign_1x_ = native_unaligned_simd<char>;

using int2x2 = fixed_size_simd<int2, 2u>;
using int2x4 = fixed_size_simd<int2, 4u>;
using int2x8 = fixed_size_simd<int2, 8u>;
using int2x16 = fixed_size_simd<int2, 16u>;
using int2x_ = native_simd<int2>;

using uint2x2 = fixed_size_simd<uint2, 2u>;
using uint2x4 = fixed_size_simd<uint2, 4u>;
using uint2x8 = fixed_size_simd<uint2, 8u>;
using uint2x16 = fixed_size_simd<uint2, 16u>;
using uint2x_ = native_simd<uint2>;

using int_unalign_2x2 = fixed_size_unaligned_simd<int2, 2u>;
using int_unalign_2x4 = fixed_size_unaligned_simd<int2, 4u>;
using int_unalign_2x8 = fixed_size_unaligned_simd<int2, 8u>;
using int_unalign_2x16 = fixed_size_unaligned_simd<int2, 16u>;
using int_unalign_2x_ = native_unaligned_simd<int2>;

using uint_unalign_2x2 = fixed_size_unaligned_simd<uint2, 2u>;
using uint_unalign_2x4 = fixed_size_unaligned_simd<uint2, 4u>;
using uint_unalign_2x8 = fixed_size_unaligned_simd<uint2, 8u>;
using uint_unalign_2x16 = fixed_size_unaligned_simd<uint2, 16u>;
using uint_unalign_2x_ = native_unaligned_simd<uint2>;

using int4x2 = fixed_size_simd<int4, 2u>;
using int4x4 = fixed_size_simd<int4, 4u>;
using int4x8 = fixed_size_simd<int4, 8u>;
using int4x_ = native_simd<int4>;

using uint4x2 = fixed_size_simd<uint4, 2u>;
using uint4x4 = fixed_size_simd<uint4, 4u>;
using uint4x8 = fixed_size_simd<uint4, 8u>;
using uint4x_ = native_simd<uint4>;

using int_unalign_4x2 = fixed_size_unaligned_simd<int4, 2u>;
using int_unalign_4x4 = fixed_size_unaligned_simd<int4, 4u>;
using int_unalign_4x8 = fixed_size_unaligned_simd<int4, 8u>;
using int_unalign_4x_ = native_unaligned_simd<int4>;

using uint_unalign_4x2 = fixed_size_unaligned_simd<uint4, 2u>;
using uint_unalign_4x4 = fixed_size_unaligned_simd<uint4, 4u>;
using uint_unalign_4x8 = fixed_size_unaligned_simd<uint4, 8u>;
using uint_unalign_4x_ = native_unaligned_simd<uint4>;

using int8x2 = fixed_size_simd<int8, 2u>;
using int8x4 = fixed_size_simd<int8, 4u>;
using int8x_ = native_simd<int8>;

using uint8x2 = fixed_size_simd<uint8, 2u>;
using uint8x4 = fixed_size_simd<uint8, 4u>;
using uint8x_ = native_simd<uint8>;

using int_unalign_8x2 = fixed_size_unaligned_simd<int8, 2u>;
using int_unalign_8x4 = fixed_size_unaligned_simd<int8, 4u>;
using int_unalign_8x_ = native_unaligned_simd<int8>;

using uint_unalign_8x2 = fixed_size_unaligned_simd<uint8, 2u>;
using uint_unalign_8x4 = fixed_size_unaligned_simd<uint8, 4u>;
using uint_unalign_8x_ = native_unaligned_simd<uint8>;

using float4x2 = fixed_size_simd<float4, 2u>;
using float4x4 = fixed_size_simd<float4, 4u>;
using float4x8 = fixed_size_simd<float4, 8u>;
using float4x_ = native_simd<float4>;

using float8x2 = fixed_size_simd<float8, 2u>;
using float8x4 = fixed_size_simd<float8, 4u>;
using float8x_ = native_simd<float8>;

using float_unalign_4x2 = fixed_size_unaligned_simd<float4, 2u>;
using float_unalign_4x4 = fixed_size_unaligned_simd<float4, 4u>;
using float_unalign_4x8 = fixed_size_unaligned_simd<float4, 8u>;
using float_unalign_4x_ = native_unaligned_simd<float4>;

using float_unalign_8x2 = fixed_size_unaligned_simd<float8, 2u>;
using float_unalign_8x4 = fixed_size_unaligned_simd<float8, 4u>;
using float_unalign_8x_ = native_unaligned_simd<float8>;

// TODO: Support `bool` family vectors.
