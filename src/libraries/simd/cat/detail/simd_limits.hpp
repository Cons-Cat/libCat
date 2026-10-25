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
   min() noexcept -> vector_type {
      return scalar_limits::min();
   }

   static constexpr auto
   max() noexcept -> vector_type {
      return scalar_limits::max();
   }

   static constexpr auto
   epsilon() noexcept -> vector_type
      requires requires { scalar_limits::epsilon(); }
   {
      return scalar_limits::epsilon();
   }

   static constexpr auto
   infinity() noexcept -> vector_type
      requires requires { scalar_limits::infinity(); }
   {
      return scalar_limits::infinity();
   }

   static constexpr auto
   // NOLINTNEXTLINE Let this weird lettercase be used.
   quiet_NaN() noexcept -> vector_type
      requires requires { scalar_limits::quiet_NaN(); }
   {
      return scalar_limits::quiet_NaN();
   }

   static constexpr auto
   // NOLINTNEXTLINE Let this weird lettercase be used.
   signaling_NaN() noexcept -> vector_type
      requires requires { scalar_limits::signaling_NaN(); }
   {
      return scalar_limits::signaling_NaN();
   }

   static constexpr auto
   denorm_min() noexcept -> vector_type
      requires requires { scalar_limits::denorm_min(); }
   {
      return scalar_limits::denorm_min();
   }
};

}  // namespace cat
