#include <cat/runtime>

namespace {

[[nodiscard]]
auto
low(__uint128_t value) -> unsigned long long {
   return static_cast<unsigned long long>(value);
}

[[nodiscard]]
auto
high(__uint128_t value) -> unsigned long long {
   return static_cast<unsigned long long>(value >> 64u);
}

}  // namespace

extern "C" auto
__clzti2(__uint128_t value) -> int {
   unsigned long long const upper = high(value);
   return upper == 0u ? __builtin_clzll(low(value)) + 64
                      : __builtin_clzll(upper);
}

extern "C" auto
__ctzti2(__uint128_t value) -> int {
   unsigned long long const lower = low(value);
   return lower == 0u ? __builtin_ctzll(high(value)) + 64
                      : __builtin_ctzll(lower);
}

extern "C" auto
__ffsti2(__uint128_t value) -> int {
   if (low(value) != 0u) {
      return __builtin_ctzll(low(value)) + 1;
   }
   if (high(value) != 0u) {
      return __builtin_ctzll(high(value)) + 65;
   }
   return 0;
}

extern "C" auto
__popcountti2(__uint128_t value) -> int {
   return __builtin_popcountll(low(value)) + __builtin_popcountll(high(value));
}

extern "C" auto
__parityti2(__uint128_t value) -> int {
   return __builtin_parityll(low(value) ^ high(value));
}

extern "C" auto
__cmpti2(__int128_t first, __int128_t second) -> int {
   long long const first_high = static_cast<long long>(high(first));
   long long const second_high = static_cast<long long>(high(second));
   if (first_high != second_high) {
      return first_high < second_high ? 0 : 2;
   }
   if (low(first) != low(second)) {
      return low(first) < low(second) ? 0 : 2;
   }
   return 1;
}

extern "C" auto
__ucmpti2(__uint128_t first, __uint128_t second) -> int {
   if (high(first) != high(second)) {
      return high(first) < high(second) ? 0 : 2;
   }
   if (low(first) != low(second)) {
      return low(first) < low(second) ? 0 : 2;
   }
   return 1;
}
