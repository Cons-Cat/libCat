#include <cat/runtime>

namespace {

[[nodiscard]]
auto
countl_zero(__uint128_t value) -> int {
   unsigned long long const high = value >> 64u;
   return high == 0u
             ? __builtin_clzll(static_cast<unsigned long long>(value)) + 64
             : __builtin_clzll(high);
}

template <typename Float, typename Rep, int significand_bits, int exponent_bias>
[[nodiscard]]
auto
integer_to_float(__uint128_t value, bool negative) -> Float {
   if (value == 0u) {
      return Float{0};
   }
   constexpr int mantissa_digits = significand_bits + 1;
   int significant_digits = 128 - countl_zero(value);
   int exponent = significant_digits - 1;
   if (significant_digits > mantissa_digits) {
      if (significant_digits == mantissa_digits + 1) {
         value <<= 1u;
      } else if (significant_digits > mantissa_digits + 2) {
         int const shift = significant_digits - (mantissa_digits + 2);
         __uint128_t const discarded_mask =
            static_cast<__uint128_t>(-1) >> (128 - shift);
         value = (value >> shift) | ((value & discarded_mask) != 0u);
      }
      value |= (value & 4u) != 0u;
      ++value;
      value >>= 2u;
      if ((value & (static_cast<__uint128_t>(1) << mantissa_digits)) != 0u) {
         value >>= 1u;
         ++exponent;
      }
   } else {
      value <<= mantissa_digits - significant_digits;
   }

   constexpr int rep_bits = sizeof(Rep) * 8;
   Rep const sign = negative ? Rep{1} << (rep_bits - 1) : 0;
   Rep const significand_mask = (Rep{1} << significand_bits) - 1;
   Rep const result = sign
                      | (Rep(exponent + exponent_bias) << significand_bits)
                      | (Rep(value) & significand_mask);
   return __builtin_bit_cast(Float, result);
}

template <typename Float, typename Rep, int significand_bits, int exponent_bias>
[[nodiscard]]
auto
float_to_unsigned(Float value) -> __uint128_t {
   constexpr int rep_bits = sizeof(Rep) * 8;
   Rep const representation = __builtin_bit_cast(Rep, value);
   if ((representation >> (rep_bits - 1)) != 0u) {
      return 0;
   }
   Rep const exponent_mask = (Rep{1} << (rep_bits - significand_bits - 1)) - 1;
   int const exponent =
      int((representation >> significand_bits) & exponent_mask) - exponent_bias;
   if (exponent < 0) {
      return 0;
   }
   if (exponent >= 128) {
      return static_cast<__uint128_t>(-1);
   }
   Rep const significand_mask = (Rep{1} << significand_bits) - 1;
   Rep const significand =
      (representation & significand_mask) | (Rep{1} << significand_bits);
   if (exponent < significand_bits) {
      return significand >> (significand_bits - exponent);
   }
   return static_cast<__uint128_t>(significand)
          << (exponent - significand_bits);
}

template <typename Float, typename Rep, int significand_bits, int exponent_bias>
[[nodiscard]]
auto
float_to_signed(Float value) -> __int128_t {
   constexpr int rep_bits = sizeof(Rep) * 8;
   Rep const representation = __builtin_bit_cast(Rep, value);
   bool const negative = (representation >> (rep_bits - 1)) != 0u;
   Rep const absolute = representation & ~(Rep{1} << (rep_bits - 1));
   Float const magnitude_value = __builtin_bit_cast(Float, absolute);
   __uint128_t magnitude =
      float_to_unsigned<Float, Rep, significand_bits, exponent_bias>(
         magnitude_value
      );
   __uint128_t const signed_max = (static_cast<__uint128_t>(1) << 127u) - 1u;
   if (magnitude > signed_max) {
      return negative
                ? static_cast<__int128_t>(static_cast<__uint128_t>(1) << 127u)
                : static_cast<__int128_t>(signed_max);
   }
   if (negative) {
      magnitude = 0u - magnitude;
   }
   return static_cast<__int128_t>(magnitude);
}

struct extended_rep {
   unsigned long long significand;
   unsigned short sign_exponent;
   unsigned short padding[3];
};

union extended_bits {
   long double value;
   extended_rep representation;
};

[[nodiscard]]
auto
integer_to_extended(__uint128_t value, bool negative) -> long double {
   if (value == 0u) {
      return 0.0L;
   }
   int significant_digits = 128 - countl_zero(value);
   int exponent = significant_digits - 1;
   if (significant_digits > 64) {
      if (significant_digits == 65) {
         value <<= 1u;
      } else if (significant_digits > 66) {
         int const shift = significant_digits - 66;
         __uint128_t const discarded_mask =
            static_cast<__uint128_t>(-1) >> (128 - shift);
         value = (value >> shift) | ((value & discarded_mask) != 0u);
      }
      value |= (value & 4u) != 0u;
      ++value;
      value >>= 2u;
      if ((value & (static_cast<__uint128_t>(1) << 64u)) != 0u) {
         value >>= 1u;
         ++exponent;
      }
   } else {
      value <<= 64 - significant_digits;
   }
   extended_bits result{};
   result.representation.significand = static_cast<unsigned long long>(value);
   result.representation.sign_exponent = static_cast<unsigned short>(
      (negative ? 0x8000u : 0u) | exponent + 16'383
   );
   return result.value;
}

[[nodiscard]]
auto
extended_to_unsigned(long double value) -> __uint128_t {
   extended_bits bits{value};
   if ((bits.representation.sign_exponent & 0x8000u) != 0u) {
      return 0;
   }
   int const exponent = (bits.representation.sign_exponent & 0x7fffu) - 16'383;
   if (exponent < 0) {
      return 0;
   }
   if (exponent >= 128) {
      return static_cast<__uint128_t>(-1);
   }
   __uint128_t result = bits.representation.significand;
   if (exponent > 63) {
      result <<= exponent - 63;
   } else {
      result >>= 63 - exponent;
   }
   return result;
}

}  // namespace

extern "C" auto
__floattisf(__int128_t value) -> float {
   bool const negative = value < 0;
   __uint128_t magnitude = value;
   if (negative) {
      magnitude = 0u - magnitude;
   }
   return integer_to_float<float, unsigned, 23, 127>(magnitude, negative);
}

extern "C" auto
__floattidf(__int128_t value) -> double {
   bool const negative = value < 0;
   __uint128_t magnitude = value;
   if (negative) {
      magnitude = 0u - magnitude;
   }
   return integer_to_float<double, unsigned long long, 52, 1'023>(
      magnitude, negative
   );
}

extern "C" auto
__floattixf(__int128_t value) -> long double {
   bool const negative = value < 0;
   __uint128_t magnitude = value;
   if (negative) {
      magnitude = 0u - magnitude;
   }
   return integer_to_extended(magnitude, negative);
}

extern "C" auto
__floattitf(__int128_t value) -> __float128 {
   bool const negative = value < 0;
   __uint128_t magnitude = value;
   if (negative) {
      magnitude = 0u - magnitude;
   }
   return integer_to_float<__float128, __uint128_t, 112, 16'383>(
      magnitude, negative
   );
}

extern "C" auto
__floatuntisf(__uint128_t value) -> float {
   return integer_to_float<float, unsigned, 23, 127>(value, false);
}

extern "C" auto
__floatuntidf(__uint128_t value) -> double {
   return integer_to_float<double, unsigned long long, 52, 1'023>(value, false);
}

extern "C" auto
__floatuntixf(__uint128_t value) -> long double {
   return integer_to_extended(value, false);
}

extern "C" auto
__floatuntitf(__uint128_t value) -> __float128 {
   return integer_to_float<__float128, __uint128_t, 112, 16'383>(value, false);
}

extern "C" auto
__fixsfti(float value) -> __int128_t {
   return float_to_signed<float, unsigned, 23, 127>(value);
}

extern "C" auto
__fixdfti(double value) -> __int128_t {
   return float_to_signed<double, unsigned long long, 52, 1'023>(value);
}

extern "C" auto
__fixxfti(long double value) -> __int128_t {
   extended_bits const bits{value};
   bool const negative = (bits.representation.sign_exponent & 0x8000u) != 0u;
   long double magnitude_value = value;
   if (negative) {
      extended_bits magnitude{value};
      magnitude.representation.sign_exponent &= 0x7fffu;
      magnitude_value = magnitude.value;
   }
   __uint128_t magnitude = extended_to_unsigned(magnitude_value);
   __uint128_t const signed_max = (static_cast<__uint128_t>(1) << 127u) - 1u;
   if (magnitude > signed_max) {
      return negative
                ? static_cast<__int128_t>(static_cast<__uint128_t>(1) << 127u)
                : static_cast<__int128_t>(signed_max);
   }
   return static_cast<__int128_t>(negative ? 0u - magnitude : magnitude);
}

extern "C" auto
__fixtfti(__float128 value) -> __int128_t {
   return float_to_signed<__float128, __uint128_t, 112, 16'383>(value);
}

extern "C" auto
__fixunssfti(float value) -> __uint128_t {
   return float_to_unsigned<float, unsigned, 23, 127>(value);
}

extern "C" auto
__fixunsdfti(double value) -> __uint128_t {
   return float_to_unsigned<double, unsigned long long, 52, 1'023>(value);
}

extern "C" auto
__fixunsxfti(long double value) -> __uint128_t {
   return extended_to_unsigned(value);
}

extern "C" auto
__fixunstfti(__float128 value) -> __uint128_t {
   return float_to_unsigned<__float128, __uint128_t, 112, 16'383>(value);
}
