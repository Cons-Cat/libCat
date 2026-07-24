// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// libCat provides `std::initializer_list` for use within our container
// functions.

// NOLINTBEGIN(bugprone-std-namespace-modification)
namespace std {

template <typename T>
class initializer_list {
   constexpr initializer_list(
      T const* _Nullable p_data, __SIZE_TYPE__ in_size
   ) noexcept
       : m_p_data(p_data), m_size(in_size) {
   }

 public:
   using value_type = T;
   using reference = T const&;
   using const_reference = T const&;
   using size_type = __SIZE_TYPE__;
   using iterator = T const*;
   using const_iterator = T const*;

   constexpr initializer_list() noexcept = default;

   [[nodiscard]]
   constexpr auto
   size() const noexcept -> __SIZE_TYPE__ {
      return m_size;
   }

   [[nodiscard]]
   constexpr auto
   data() const noexcept -> T const* _Nullable {
      return m_p_data;
   }

   [[nodiscard]]
   constexpr auto
   empty() const noexcept -> bool {
      return m_size == 0u;
   }

   [[nodiscard]]
   constexpr auto
   begin() const noexcept -> T const* _Nullable {
      return m_p_data;
   }

   [[nodiscard]]
   constexpr auto
   end() const noexcept -> T const* _Nullable {
      return m_p_data + m_size;
   }

 private:
   T const* _Nullable m_p_data = nullptr;
   __SIZE_TYPE__ m_size = 0u;
};

}  // namespace std

// NOLINTEND(bugprone-std-namespace-modification)

namespace cat {
using std::initializer_list;
}
