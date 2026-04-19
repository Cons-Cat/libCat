// -*- mode: c++ -*-
#pragma once

// Broadcast scalar-to-`simd` lane initialization (standard, P3844R2). Limits
// round-trip for `really_convertible`-style constexpr broadcast and a
// `consteval` overload that rejects value-changing conversions for integral
// constants.

#include <cat/limits>
#include <cat/meta>

namespace cat::detail {

template <typename From, typename ToLane>
consteval auto
simd_broadcast_really_convertible_to() -> bool {
   if constexpr (!is_constructible<ToLane, From>) {
      return false;
   }
   if constexpr (!is_arithmetic<From> || is_same<From, bool>) {
      return false;
   }
   using ToR = raw_arithmetic_type<ToLane>;
   if constexpr (!is_arithmetic<ToR>) {
      return false;
   }
   if constexpr (is_same<From, ToR>) {
      return true;
   }

   auto roundtrip_ok = [](From x) -> bool {
      ToR y = static_cast<ToR>(x);
      return static_cast<From>(y) == x;
   };

   if constexpr (is_integral<From>) {
      if constexpr (is_floating_point<ToR>) {
         auto endpoint_ok = [](From x) -> bool {
            long double const lx = static_cast<long double>(x);
            ToR const y = static_cast<ToR>(lx);
            return lx == static_cast<long double>(y);
         };
         return endpoint_ok(limits<From>::max())
                && endpoint_ok(limits<From>::min());
      }
      return roundtrip_ok(limits<From>::max())
             && roundtrip_ok(limits<From>::min());
   }
   if constexpr (is_floating_point<From>) {
      return roundtrip_ok(limits<From>::max())
             && roundtrip_ok(limits<From>::min());
   }
   return false;
}

template <typename From, typename ToLane>
consteval auto
simd_broadcast_consteval_value_ok(From f) -> bool {
   using ToR = raw_arithmetic_type<ToLane>;
   ToR const tr = static_cast<ToR>(f);
   return static_cast<long double>(f) == static_cast<long double>(tr);
}

template <typename From, typename ToLane>
concept simd_consteval_broadcast_arg =
   is_arithmetic<From> && !is_same<From, bool> && is_constructible<ToLane, From>
   && !simd_broadcast_really_convertible_to<From, ToLane>()
   && requires { typename common_type<From, raw_arithmetic_type<ToLane>>; }
   && (is_same<common_type<From, raw_arithmetic_type<ToLane>>,
               raw_arithmetic_type<ToLane>>
       || (is_same<From, int> && is_integral<raw_arithmetic_type<ToLane>>)
       || (is_same<From, unsigned int>
           && is_unsigned_integral<raw_arithmetic_type<ToLane>>));

}  // namespace cat::detail
