// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

namespace cat {

constexpr void
verify(bool invariant_expression, source_location const& callsite) {
   if consteval {
      [[assume(invariant_expression)]];
   } else {
      if (!invariant_expression) [[unlikely]] {
         detail::assert_failed(assert_handler, callsite);
      }
   }
}

constexpr void
verify(
   bool invariant_expression, detail::assert_handler p_assert_handler,
   source_location const& callsite
) {
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
) {
   if consteval {
      [[assume(invariant_expression)]];
   } else {
      if (!invariant_expression) [[unlikely]] {
         detail::assert_failed(error_string, assert_handler, callsite);
      }
   }
}

constexpr void
verify(
   bool invariant_expression, str_view const& error_string,
   detail::assert_handler p_assert_handler, source_location const& callsite
) {
   if consteval {
      [[assume(invariant_expression)]];
   } else {
      if (!invariant_expression) [[unlikely]] {
         detail::assert_failed(error_string, p_assert_handler, callsite);
      }
   }
}

}  // namespace cat
