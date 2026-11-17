#pragma once

#include <cat/arithmetic>
#include <cat/string>
#include <cat/type_list>

// The decimal limits of an integer type, and the values just beyond them.
struct integer_bounds {
   cat::str_view min;
   cat::str_view max;
   cat::str_view below_min;
   cat::str_view above_max;
};

template <typename T>
constexpr auto
bounds_of() -> integer_bounds {
   if constexpr (sizeof(T) == 1u && cat::is_signed<T>) {
      return {
         .min = "-128",
         .max = "127",
         .below_min = "-129",
         .above_max = "128",
      };
   } else if constexpr (sizeof(T) == 1u && !cat::is_signed<T>) {
      return {
         .min = "0",
         .max = "255",
         .below_min = "-1",
         .above_max = "256",
      };
   } else if constexpr (sizeof(T) == 2u && cat::is_signed<T>) {
      return {
         .min = "-32768",
         .max = "32767",
         .below_min = "-32769",
         .above_max = "32768",
      };
   } else if constexpr (sizeof(T) == 2u && !cat::is_signed<T>) {
      return {
         .min = "0",
         .max = "65535",
         .below_min = "-1",
         .above_max = "65536",
      };
   } else if constexpr (sizeof(T) == 4u && cat::is_signed<T>) {
      return {
         .min = "-2147483648",
         .max = "2147483647",
         .below_min = "-2147483649",
         .above_max = "2147483648",
      };
   } else if constexpr (sizeof(T) == 4u && !cat::is_signed<T>) {
      return {
         .min = "0",
         .max = "4294967295",
         .below_min = "-1",
         .above_max = "4294967296",
      };
   } else if constexpr (sizeof(T) == 8u && cat::is_signed<T>) {
      return {
         .min = "-9223372036854775808",
         .max = "9223372036854775807",
         .below_min = "-9223372036854775809",
         .above_max = "9223372036854775808",
      };
   } else if constexpr (sizeof(T) == 8u && !cat::is_signed<T>) {
      return {
         .min = "0",
         .max = "18446744073709551615",
         .below_min = "-1",
         .above_max = "18446744073709551616",
      };
   } else if constexpr (sizeof(T) == 16u && cat::is_signed<T>) {
      return {
         .min = "-170141183460469231731687303715884105728",
         .max = "170141183460469231731687303715884105727",
         .below_min = "-170141183460469231731687303715884105729",
         .above_max = "170141183460469231731687303715884105728",
      };
   } else if constexpr (sizeof(T) == 16u && !cat::is_signed<T>) {
      return {
         .min = "0",
         .max = "340282366920938463463374607431768211455",
         .below_min = "-1",
         .above_max = "340282366920938463463374607431768211456",
      };
   }
}

// `idx` is narrower than its raw `uint8`.
inline constexpr integer_bounds idx_bounds = {
   .min = "0",
   .max = "9223372036854775807",
   .below_min = "-1",
   .above_max = "9223372036854775808",
};

// Every integer type, except quantities. `idx` is tested separately.
// TODO: Test integers with quantities.
using parseable_integers = cat::type_list<
   signed char, unsigned char, short, unsigned short, int, unsigned, long,
   unsigned long, long long, unsigned long long, __int128, unsigned __int128,
   int1, uint1, int2, uint2, int4, uint4, int8, uint8, int16, uint16,
   cat::wrap_int1, cat::wrap_uint1, cat::wrap_int2, cat::wrap_uint2,
   cat::wrap_int4, cat::wrap_uint4, cat::wrap_int8, cat::wrap_uint8,
   cat::wrap_int16, cat::wrap_uint16, cat::sat_int1, cat::sat_uint1,
   cat::sat_int2, cat::sat_uint2, cat::sat_int4, cat::sat_uint4, cat::sat_int8,
   cat::sat_uint8, cat::sat_int16, cat::sat_uint16, cat::iword, cat::uword,
   cat::intptr<void>, cat::uintptr<void>>;
