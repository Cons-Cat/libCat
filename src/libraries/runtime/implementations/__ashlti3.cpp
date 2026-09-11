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

}  // namespace

extern "C" auto
__ashlti3(__int128_t value, int shift) -> __int128_t {
   words const input = split(static_cast<__uint128_t>(value));
   if ((shift & 64) != 0) {
      return static_cast<__int128_t>(
         join({0u, input.low << static_cast<unsigned>(shift - 64)})
      );
   }
   if (shift == 0) {
      return value;
   }
   return static_cast<__int128_t>(join(
      {input.low << static_cast<unsigned>(shift),
       (input.high << static_cast<unsigned>(shift))
          | (input.low >> static_cast<unsigned>(64 - shift))}
   ));
}

extern "C" auto
__lshrti3(__uint128_t value, int shift) -> __uint128_t {
   words const input = split(value);
   if ((shift & 64) != 0) {
      return join({input.high >> static_cast<unsigned>(shift - 64), 0u});
   }
   if (shift == 0) {
      return value;
   }
   return join(
      {(input.high << static_cast<unsigned>(64 - shift))
          | (input.low >> static_cast<unsigned>(shift)),
       input.high >> static_cast<unsigned>(shift)}
   );
}

extern "C" auto
__ashrti3(__int128_t value, int shift) -> __int128_t {
   words const input = split(static_cast<__uint128_t>(value));
   long long const high = static_cast<long long>(input.high);
   if ((shift & 64) != 0) {
      return static_cast<__int128_t>(join(
         {static_cast<unsigned long long>(
             high >> static_cast<unsigned>(shift - 64)
          ),
          static_cast<unsigned long long>(high >> 63)}
      ));
   }
   if (shift == 0) {
      return value;
   }
   return static_cast<__int128_t>(join(
      {(input.high << static_cast<unsigned>(64 - shift))
          | (input.low >> static_cast<unsigned>(shift)),
       static_cast<unsigned long long>(high >> static_cast<unsigned>(shift))}
   ));
}
