#include <cat/runtime>

#include "../unit_tests.hpp"

$test(runtime_int128_division_and_multiplication) {
   __uint128_t const unsigned_dividend =
      (static_cast<__uint128_t>(0x12345678'9abcdef0u) << 64u)
      | 0xfedcba98'76543210u;
   __uint128_t const unsigned_divisor =
      (static_cast<__uint128_t>(0x1234u) << 64u) | 0x5678u;
   __uint128_t unsigned_remainder = 0;
   __uint128_t const unsigned_quotient = cat::__udivmodti4(
      unsigned_dividend, unsigned_divisor, &unsigned_remainder
   );
   cat::verify(
      (unsigned_quotient * unsigned_divisor) + unsigned_remainder
      == unsigned_dividend
   );
   cat::verify(unsigned_remainder < unsigned_divisor);
   cat::verify(
      cat::__udivti3(unsigned_dividend, unsigned_divisor) == unsigned_quotient
   );
   cat::verify(
      cat::__umodti3(unsigned_dividend, unsigned_divisor) == unsigned_remainder
   );

   __int128_t const signed_dividend =
      -static_cast<__int128_t>(unsigned_dividend);
   __int128_t const signed_divisor = static_cast<__int128_t>(unsigned_divisor);
   __int128_t signed_remainder = 0;
   __int128_t const signed_quotient =
      cat::__divmodti4(signed_dividend, signed_divisor, &signed_remainder);
   cat::verify(
      (signed_quotient * signed_divisor) + signed_remainder == signed_dividend
   );
   cat::verify(signed_remainder <= 0);
   cat::verify(
      cat::__divti3(signed_dividend, signed_divisor) == signed_quotient
   );
   cat::verify(
      cat::__modti3(signed_dividend, signed_divisor) == signed_remainder
   );

   __int128_t const first = static_cast<__int128_t>(
      (static_cast<__uint128_t>(0x12345678u) << 64u) | 0x9abcdef0u
   );
   __int128_t const second = 0x10203040;
   cat::verify(cat::__multi3(first, second) == first * second);
   cat::verify(cat::__negti2(first) == -first);
   cat::verify(cat::__addvti3(first, second) == first + second);
   cat::verify(cat::__subvti3(first, second) == first - second);
   cat::verify(cat::__negvti2(first) == -first);
   cat::verify(cat::__absvti2(-first) == first);
   int overflow = 0;
   cat::verify(cat::__muloti4(first, second, &overflow) == first * second);
   cat::verify(overflow == 0);
   cat::verify(cat::__mulvti3(first, second) == first * second);
   cat::verify(cat::__cmpti2(-first, first) == 0);
   cat::verify(cat::__cmpti2(first, first) == 1);
   cat::verify(cat::__cmpti2(first, -first) == 2);
   cat::verify(cat::__ucmpti2(1u, 2u) == 0);
   cat::verify(cat::__ucmpti2(2u, 2u) == 1);
   cat::verify(cat::__ucmpti2(2u, 1u) == 2);
}

$test(runtime_int128_shift_bit_and_conversion) {
   __uint128_t const value =
      (static_cast<__uint128_t>(0x80000000'00000001u) << 64u) | 8u;
   cat::verify(cat::__clzti2(value) == 0);
   cat::verify(cat::__ctzti2(value) == 3);
   cat::verify(cat::__ffsti2(value) == 4);
   cat::verify(cat::__popcountti2(value) == 3);
   cat::verify(cat::__parityti2(value) == 1);
   cat::verify(cat::__lshrti3(value, 68) == value >> 68u);
   cat::verify(
      cat::__ashlti3(static_cast<__int128_t>(value), 17)
      == static_cast<__int128_t>(value << 17u)
   );
   __int128_t const negative = -static_cast<__int128_t>(value >> 1u);
   cat::verify(cat::__ashrti3(negative, 68) == negative >> 68u);

   __int128_t const integer = static_cast<__int128_t>(
      (static_cast<__uint128_t>(1) << 100u)
      + (static_cast<__uint128_t>(1) << 47u)
   );
   double const converted = cat::__floattidf(integer);
   cat::verify(cat::__fixdfti(converted) == static_cast<__int128_t>(converted));
   cat::verify(cat::__floattisf(16'777'217) == 16'777'216.0F);
   cat::verify(cat::__floatuntidf(1u << 24u) == 16'777'216.0);
   cat::verify(cat::__floatuntisf(1u << 24u) == 16'777'216.0F);
   cat::verify(cat::__fixsfti(-123.75F) == -123);
   cat::verify(cat::__fixunsdfti(123.75) == 123u);
   cat::verify(cat::__fixunssfti(-1.0F) == 0u);
   cat::verify(cat::__floattixf(integer) == static_cast<long double>(integer));
   cat::verify(
      cat::__floatuntixf(static_cast<__uint128_t>(integer))
      == static_cast<long double>(integer)
   );
   cat::verify(cat::__fixxfti(-123.75L) == -123);
   cat::verify(cat::__fixunsxfti(123.75L) == 123u);
   cat::verify(cat::__floattitf(integer) == static_cast<__float128>(integer));
   cat::verify(
      cat::__floatuntitf(static_cast<__uint128_t>(integer))
      == static_cast<__float128>(integer)
   );
   cat::verify(cat::__fixtfti(static_cast<__float128>(-123.75)) == -123);
   cat::verify(cat::__fixunstfti(static_cast<__float128>(123.75)) == 123u);
}
