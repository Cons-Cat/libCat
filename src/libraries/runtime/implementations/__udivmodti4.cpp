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
   unsigned __int128 dividend, unsigned __int128 divisor,
   unsigned __int128* _Nullable p_remainder
) -> unsigned __int128 {
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
      unsigned __int128 quotient;
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
   unsigned __int128 shifted_divisor = divisor
                                       << cat::make_raw_arithmetic(shift);
   unsigned __int128 quotient = 0;
   unsigned __int128 working = dividend;
   for (; shift >= 0; --shift) {
      quotient <<= 1u;
      __int128 const take =
         static_cast<__int128>(shifted_divisor - working - 1u) >> 127;
      quotient |= static_cast<unsigned __int128>(take & 1);
      working -= shifted_divisor & static_cast<unsigned __int128>(take);
      shifted_divisor >>= 1u;
   }
   if (p_remainder != nullptr) {
      *p_remainder = working;
   }
   return quotient;
}

extern "C" auto
__udivti3(unsigned __int128 dividend, unsigned __int128 divisor)
   -> unsigned __int128 {
   return __udivmodti4(dividend, divisor, nullptr);
}

extern "C" auto
__umodti3(unsigned __int128 dividend, unsigned __int128 divisor)
   -> unsigned __int128 {
   unsigned __int128 remainder = 0;
   static_cast<void>(__udivmodti4(dividend, divisor, &remainder));
   return remainder;
}
