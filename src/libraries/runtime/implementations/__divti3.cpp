#include <cat/runtime>

// Matches llvm-project compiler-rt `divti3.c` / `modti3.c`
// (`int_div_impl.inc`). Arithmetic-right-shift signs, abs via xor-add,
// `__udivmodti4`, restore sign.

extern "C" auto
__divti3(__int128_t dividend, __int128_t divisor) -> __int128_t {
   __int128_t const sign_dividend = dividend >> 127;
   __int128_t const sign_divisor = divisor >> 127;
   __uint128_t const abs_dividend =
      (dividend ^ sign_dividend) + (-sign_dividend);
   __uint128_t const abs_divisor = (divisor ^ sign_divisor) + (-sign_divisor);
   __int128_t const sign_quotient = sign_dividend ^ sign_divisor;
   return (cat::__udivmodti4(abs_dividend, abs_divisor, nullptr)
           ^ sign_quotient)
          + (-sign_quotient);
}

extern "C" auto
__modti3(__int128_t dividend, __int128_t divisor) -> __int128_t {
   __int128_t const sign_divisor = divisor >> 127;
   __uint128_t const abs_divisor = (divisor ^ sign_divisor) + (-sign_divisor);
   __int128_t const sign_dividend = dividend >> 127;
   __uint128_t const abs_dividend =
      (dividend ^ sign_dividend) + (-sign_dividend);
   __uint128_t remainder = 0;
   static_cast<void>(cat::__udivmodti4(abs_dividend, abs_divisor, &remainder));
   return (remainder ^ sign_dividend) + (-sign_dividend);
}

extern "C" auto
__divmodti4(
   __int128_t dividend, __int128_t divisor, __int128_t* _Nonnull p_remainder
) -> __int128_t {
   __int128_t const sign_dividend = dividend >> 127;
   __int128_t const sign_divisor = divisor >> 127;
   __uint128_t const abs_dividend =
      (dividend ^ sign_dividend) + (-sign_dividend);
   __uint128_t const abs_divisor = (divisor ^ sign_divisor) + (-sign_divisor);
   __uint128_t remainder = 0;
   __uint128_t quotient =
      cat::__udivmodti4(abs_dividend, abs_divisor, &remainder);
   __int128_t const sign_quotient = sign_dividend ^ sign_divisor;
   *p_remainder = (remainder ^ sign_dividend) + (-sign_dividend);
   return (quotient ^ sign_quotient) + (-sign_quotient);
}
