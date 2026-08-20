#pragma once

namespace cat {

template <typename T, typename Abi>
   requires(requires {
               limits<T>::min();
               limits<T>::max();
            })
struct limits<simd<T, Abi>> : limits<T> {
 private:
   using vector_type = simd<T, Abi>;
   using scalar_limits = limits<T>;

 public:
   constexpr limits() = delete;

   static constexpr auto
   min() -> vector_type {
      return scalar_limits::min();
   }

   static constexpr auto
   max() -> vector_type {
      return scalar_limits::max();
   }

   static constexpr auto
   epsilon() -> vector_type
      requires requires { scalar_limits::epsilon(); }
   {
      return scalar_limits::epsilon();
   }

   static constexpr auto
   infinity() -> vector_type
      requires requires { scalar_limits::infinity(); }
   {
      return scalar_limits::infinity();
   }

   static constexpr auto
   // NOLINTNEXTLINE Let this weird lettercase be used.
   quiet_NaN() -> vector_type
      requires requires { scalar_limits::quiet_NaN(); }
   {
      return scalar_limits::quiet_NaN();
   }

   static constexpr auto
   // NOLINTNEXTLINE Let this weird lettercase be used.
   signaling_NaN() -> vector_type
      requires requires { scalar_limits::signaling_NaN(); }
   {
      return scalar_limits::signaling_NaN();
   }

   static constexpr auto
   denorm_min() -> vector_type
      requires requires { scalar_limits::denorm_min(); }
   {
      return scalar_limits::denorm_min();
   }
};

}  // namespace cat
