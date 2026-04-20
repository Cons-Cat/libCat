// -*- mode: c++ -*-
#pragma once

// Broadcast scalar-to-`simd` lane initialization (standard, P3844R2). Mirrors
// value-preserving conversions. Limits round-trip for non-`bool` arithmetic
// sources, both `false` and `true` for `bool`, plus a `consteval` overload that
// rejects value-changing conversions for integral constants.

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/meta>

namespace cat::detail {

template <typename ToLane>
struct simd_broadcast_lane_raw_trait {
   using type = raw_arithmetic_type<ToLane>;
};

template <>
struct simd_broadcast_lane_raw_trait<bool> {
   using type = unsigned char;
};

template <typename ToLane>
using simd_broadcast_lane_raw = simd_broadcast_lane_raw_trait<ToLane>::type;

template <typename From, typename ToLane>
consteval auto
simd_broadcast_really_convertible_to() -> bool {
   using unqualified = remove_cvref<From>;

   if constexpr (!is_constructible<ToLane, From>) {
      return false;
   }

   if constexpr (!is_arithmetic<unqualified> && !is_bool<unqualified>) {
      return false;
   }

   using ToRaw = simd_broadcast_lane_raw<ToLane>;
   if constexpr (!is_arithmetic<ToRaw>) {
      return false;
   }

   // For boolean types, both truth values must round-trip through the lane raw
   // type. When `From` is `bool` and `ToLane` is `bool2` or `bool4`, that
   // matches implicit `bool` to `ToLane` conversion (e.g. `bool4(true)`), so
   // `simd<bool4> == bool` forms a temporary via the non-explicit broadcast
   // constructor `simd(bool)` rather than relying on some other conversion.
   if constexpr (is_bool<unqualified>) {
      auto ok = [](From x) -> bool {
         ToRaw const y = static_cast<ToRaw>(x);
         return static_cast<unqualified>(y) == static_cast<unqualified>(x);
      };
      return ok(static_cast<From>(false)) && ok(static_cast<From>(true));
   }

   if constexpr (is_same<unqualified, ToRaw>) {
      return true;
   }

   auto roundtrip_ok = [](From x) -> bool {
      ToRaw y = static_cast<ToRaw>(x);
      return static_cast<From>(y) == x;
   };

   if constexpr (is_integral<unqualified>) {
      if constexpr (is_floating_point<ToRaw>) {
         auto endpoint_ok = [](From x) -> bool {
            long double const lx = static_cast<long double>(x);
            ToRaw const y = static_cast<ToRaw>(lx);
            return lx == static_cast<long double>(y);
         };
         return endpoint_ok(limits<unqualified>::max())
                && endpoint_ok(limits<unqualified>::min());
      }
      return roundtrip_ok(limits<unqualified>::max())
             && roundtrip_ok(limits<unqualified>::min());
   }
   if constexpr (is_floating_point<unqualified>) {
      return roundtrip_ok(limits<unqualified>::max())
             && roundtrip_ok(limits<unqualified>::min());
   }
   return false;
}

template <typename From, typename ToLane>
consteval auto
simd_broadcast_consteval_value_ok(From f) -> bool {
   using ToR = simd_broadcast_lane_raw<ToLane>;
   ToR const tr = static_cast<ToR>(f);
   return static_cast<long double>(f) == static_cast<long double>(tr);
}

template <typename From, typename ToLane>
concept simd_consteval_broadcast_arg =
   is_arithmetic<From> && is_constructible<ToLane, From>
   && !simd_broadcast_really_convertible_to<From, ToLane>()
   && requires { typename common_type<From, simd_broadcast_lane_raw<ToLane>>; }
   && (is_same<common_type<From, simd_broadcast_lane_raw<ToLane>>,
               simd_broadcast_lane_raw<ToLane>>
       || (is_same<From, int> && is_integral<simd_broadcast_lane_raw<ToLane>>)
       || (is_same<From, unsigned int>
           && is_unsigned_integral<simd_broadcast_lane_raw<ToLane>>));

}  // namespace cat::detail
