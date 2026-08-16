// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

namespace cat {

constexpr void
assert(bool invariant_expression, source_location const& callsite) {
   if consteval {
      [[assume(invariant_expression)]];
   }
#ifndef NDEBUG
   else {
      verify(invariant_expression, callsite);
   }
#endif
}

constexpr void
assert(
   bool invariant_expression, detail::assert_handler p_assert_handler,
   source_location const& callsite
) {
   if consteval {
      [[assume(invariant_expression)]];
   }
#ifndef NDEBUG
   else {
      verify(invariant_expression, p_assert_handler, callsite);
   }
#endif
}

constexpr void
assert(
   bool invariant_expression, str_view const& error_string,
   source_location const& callsite
) {
   if consteval {
      [[assume(invariant_expression)]];
   }
#ifndef NDEBUG
   else {
      verify(invariant_expression, error_string, callsite);
   }
#endif
}

constexpr void
assert(
   bool invariant_expression, str_view const& error_string,
   detail::assert_handler p_assert_handler, source_location const& callsite
) {
   if consteval {
      [[assume(invariant_expression)]];
   }
#ifndef NDEBUG
   else {
      verify(invariant_expression, error_string, p_assert_handler, callsite);
   }
#endif
}

}  // namespace cat

#pragma clang diagnostic pop
