#include <cat/runtime>

namespace {

constexpr __uint128_t sign_bit = static_cast<__uint128_t>(1) << 127u;
constexpr __uint128_t abs_mask = sign_bit - 1u;
constexpr __uint128_t inf_rep = static_cast<__uint128_t>(0x7fff) << 112u;

}  // namespace

extern "C" auto
__eqtf2(__float128 first, __float128 second) -> int {
   __int128_t const first_int =
      static_cast<__int128_t>(__builtin_bit_cast(__uint128_t, first));
   __int128_t const second_int =
      static_cast<__int128_t>(__builtin_bit_cast(__uint128_t, second));
   __uint128_t const first_abs = static_cast<__uint128_t>(first_int) & abs_mask;
   __uint128_t const second_abs =
      static_cast<__uint128_t>(second_int) & abs_mask;

   if (first_abs > inf_rep || second_abs > inf_rep) {
      return 1;
   }
   if ((first_abs | second_abs) == 0u) {
      return 0;
   }
   if ((first_int & second_int) >= 0) {
      if (first_int < second_int) {
         return -1;
      }
      if (first_int == second_int) {
         return 0;
      }
      return 1;
   }
   if (first_int > second_int) {
      return -1;
   }
   if (first_int == second_int) {
      return 0;
   }
   return 1;
}
