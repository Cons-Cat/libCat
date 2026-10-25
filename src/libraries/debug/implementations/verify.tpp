// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

namespace cat {

constexpr void
verify(bool invariant_expression, source_location const& callsite) noexcept(
   false
) {
   if consteval {
      [[assume(invariant_expression)]];
   } else {
      if (!invariant_expression) [[unlikely]] {
         detail::assert_failed(current_assert_handler(), callsite);
      }
   }
}

constexpr void
verify(
   bool invariant_expression, assert_handler p_assert_handler,
   source_location const& callsite
) noexcept(!__has_feature(cxx_exceptions)) {
   if consteval {
      [[assume(invariant_expression)]];
   } else {
      if (!invariant_expression) [[unlikely]] {
         detail::assert_failed(p_assert_handler, callsite);
      }
   }
}

constexpr void
verify(
   bool invariant_expression, str_view const& error_string,
   source_location const& callsite
) noexcept(!__has_feature(cxx_exceptions)) {
   if consteval {
      [[assume(invariant_expression)]];
   } else {
      if (!invariant_expression) [[unlikely]] {
         detail::assert_failed(
            error_string, current_assert_handler(), callsite
         );
      }
   }
}

constexpr void
verify(
   bool invariant_expression, str_view const& error_string,
   assert_handler p_assert_handler, source_location const& callsite
) noexcept(!__has_feature(cxx_exceptions)) {
   if consteval {
      [[assume(invariant_expression)]];
   } else {
      if (!invariant_expression) [[unlikely]] {
         detail::assert_failed(error_string, p_assert_handler, callsite);
      }
   }
}

}  // namespace cat
