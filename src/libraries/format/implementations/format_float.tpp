// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename T, typename CharT>
   requires(is_floating_point<T> && is_same<CharT, char>)
struct formatter<T, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, T const& value,
      format_context context
   ) const -> scaredy_format<void> {
      $prop(detail::append_arithmetic(context, make_raw_arithmetic(value)));
      if constexpr (has_quantity<T>) {
         return detail::append_arithmetic_quantity_symbol<T>(context);
      }
      return monostate;
   }

   auto
   format(allocator_dyn allocator, T const& value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
