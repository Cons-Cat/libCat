// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/meta>

namespace cat {

class random_seed {
 public:
   template <is_unsigned_integral T>
   constexpr random_seed(T value) noexcept : m_value(value) {
   }

   template <is_unsigned_integral T>
   [[nodiscard]]
   constexpr explicit
   operator T() const noexcept {
      return T(m_value);
   }

 private:
   uint16 m_value;
};

}  // namespace cat
