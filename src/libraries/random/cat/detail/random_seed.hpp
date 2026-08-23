// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/meta>

namespace cat {

class random_seed {
 public:
   template <is_unsigned_integral T>
   constexpr random_seed(T value) : m_value(make_raw_arithmetic(value)) {
   }

   template <is_unsigned_integral T>
   [[nodiscard]]
   constexpr explicit
   operator T() const {
      return T(m_value);
   }

 private:
   unsigned __int128 m_value;
};

}  // namespace cat
