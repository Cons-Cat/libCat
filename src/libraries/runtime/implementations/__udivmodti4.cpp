#include <cat/bit>
#include <cat/runtime>

// Matches llvm-project compiler-rt `udivmodti4.c` on x86-64.
// 128 / 64 uses `divq`. Wider divisors use a `clz`-normalized loop.

namespace {

[[nodiscard]]
auto
udiv128_by_64(
   cat::wrap_uint8 u1, cat::wrap_uint8 u0, cat::wrap_uint8 v,
   cat::wrap_uint8& remainder
) -> cat::wrap_uint8 {
   unsigned long long quotient;
   unsigned long long rem;
   asm("divq %[v]"
       : "=a"(quotient), "=d"(rem)
       : [v] "r"(v.raw), "a"(u0.raw), "d"(u1.raw));
   remainder = rem;
   return quotient;
}

}  // namespace

extern "C" auto
__udivmodti4(
   __uint128_t dividend, __uint128_t divisor, __uint128_t* _Nullable p_remainder
) -> __uint128_t {
   if (divisor > dividend) {
      if (p_remainder != nullptr) {
         *p_remainder = dividend;
      }
      return 0;
   }

   cat::wrap_uint8 const divisor_high = cat::uint8(divisor >> 64u);
   cat::wrap_uint8 const divisor_low = cat::uint8(divisor);
   cat::wrap_uint8 dividend_high = cat::uint8(dividend >> 64u);
   cat::wrap_uint8 const dividend_low = cat::uint8(dividend);

   if (divisor_high == 0u) {
      cat::wrap_uint8 remainder_low = 0u;
      __uint128_t quotient;
      if (dividend_high < divisor_low) {
         quotient = __uint128_t(udiv128_by_64(
            dividend_high, dividend_low, divisor_low, remainder_low
         ));
      } else {
         cat::wrap_uint8 const quotient_high = dividend_high / divisor_low;
         dividend_high = dividend_high % divisor_low;
         cat::wrap_uint8 const quotient_low = udiv128_by_64(
            dividend_high, dividend_low, divisor_low, remainder_low
         );
         quotient =
            (__uint128_t(quotient_high) << 64u) | __uint128_t(quotient_low);
      }
      if (p_remainder != nullptr) {
         *p_remainder = __uint128_t(remainder_low);
      }
      return quotient;
   }

   cat::int4 shift = cat::int4(divisor_high.countl_zero())
                     - cat::int4(dividend_high.countl_zero());
   __uint128_t shifted_divisor = divisor << cat::make_raw_arithmetic(shift);
   __uint128_t quotient = 0;
   __uint128_t working = dividend;
   for (; shift >= 0; --shift) {
      quotient <<= 1u;
      __int128_t const take =
         static_cast<__int128_t>(shifted_divisor - working - 1u) >> 127;
      quotient |= static_cast<__uint128_t>(take & 1);
      working -= shifted_divisor & static_cast<__uint128_t>(take);
      shifted_divisor >>= 1u;
   }
   if (p_remainder != nullptr) {
      *p_remainder = working;
   }
   return quotient;
}

extern "C" auto
__udivti3(__uint128_t dividend, __uint128_t divisor) -> __uint128_t {
   return __udivmodti4(dividend, divisor, nullptr);
}

extern "C" auto
__umodti3(__uint128_t dividend, __uint128_t divisor) -> __uint128_t {
   __uint128_t remainder = 0;
   static_cast<void>(__udivmodti4(dividend, divisor, &remainder));
   return remainder;
}
