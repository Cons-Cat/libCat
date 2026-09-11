#include <cat/runtime>

namespace {

struct words {
   unsigned long long low;
   unsigned long long high;
};

[[nodiscard]]
auto
split(__uint128_t value) -> words {
   return {
      static_cast<unsigned long long>(value),
      static_cast<unsigned long long>(value >> 64u)
   };
}

[[nodiscard]]
auto
join(words value) -> __uint128_t {
   return (static_cast<__uint128_t>(value.high) << 64u) | value.low;
}

[[nodiscard]]
auto
multiply_64(unsigned long long first, unsigned long long second) -> words {
   constexpr unsigned long long low_mask = 0xffffffffu;
   unsigned long long const first_low = first & low_mask;
   unsigned long long const first_high = first >> 32u;
   unsigned long long const second_low = second & low_mask;
   unsigned long long const second_high = second >> 32u;

   unsigned long long low = first_low * second_low;
   unsigned long long carry = low >> 32u;
   low &= low_mask;
   carry += first_high * second_low;
   low += (carry & low_mask) << 32u;
   unsigned long long high = carry >> 32u;
   carry = low >> 32u;
   low &= low_mask;
   carry += second_high * first_low;
   low += (carry & low_mask) << 32u;
   high += (carry >> 32u) + first_high * second_high;
   return {low, high};
}

}  // namespace

extern "C" auto
__multi3(__int128_t first, __int128_t second) -> __int128_t {
   words const first_words = split(static_cast<__uint128_t>(first));
   words const second_words = split(static_cast<__uint128_t>(second));
   words result = multiply_64(first_words.low, second_words.low);
   result.high +=
      first_words.high * second_words.low + first_words.low * second_words.high;
   return static_cast<__int128_t>(join(result));
}

extern "C" auto
__negti2(__int128_t value) -> __int128_t {
   return static_cast<__int128_t>(0u - static_cast<__uint128_t>(value));
}

extern "C" auto
__addvti3(__int128_t first, __int128_t second) -> __int128_t {
   __int128_t const result = static_cast<__int128_t>(
      static_cast<__uint128_t>(first) + static_cast<__uint128_t>(second)
   );
   if ((second >= 0 && result < first) || (second < 0 && result >= first)) {
      __builtin_trap();
   }
   return result;
}

extern "C" auto
__subvti3(__int128_t first, __int128_t second) -> __int128_t {
   __int128_t const result = static_cast<__int128_t>(
      static_cast<__uint128_t>(first) - static_cast<__uint128_t>(second)
   );
   if ((second >= 0 && result > first) || (second < 0 && result <= first)) {
      __builtin_trap();
   }
   return result;
}

extern "C" auto
__negvti2(__int128_t value) -> __int128_t {
   __int128_t const minimum =
      static_cast<__int128_t>(static_cast<__uint128_t>(1) << 127u);
   if (value == minimum) {
      __builtin_trap();
   }
   return __negti2(value);
}

extern "C" auto
__absvti2(__int128_t value) -> __int128_t {
   return value < 0 ? __negvti2(value) : value;
}

extern "C" auto
__muloti4(__int128_t first, __int128_t second, int* _Nonnull p_overflow)
   -> __int128_t {
   bool const negative = (first < 0) != (second < 0);
   __uint128_t first_magnitude = first;
   __uint128_t second_magnitude = second;
   if (first < 0) {
      first_magnitude = 0u - first_magnitude;
   }
   if (second < 0) {
      second_magnitude = 0u - second_magnitude;
   }
   __uint128_t const limit = negative
                                ? static_cast<__uint128_t>(1) << 127u
                                : (static_cast<__uint128_t>(1) << 127u) - 1u;
   *p_overflow = second_magnitude != 0u
                 && first_magnitude > cat::__udivti3(limit, second_magnitude);
   return __multi3(first, second);
}

extern "C" auto
__mulvti3(__int128_t first, __int128_t second) -> __int128_t {
   int overflow = 0;
   __int128_t const result = __muloti4(first, second, &overflow);
   if (overflow != 0) {
      __builtin_trap();
   }
   return result;
}
