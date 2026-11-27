// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename T, typename CharT>
   requires(is_string_utf8_interconvertible<T> && is_same<CharT, char>)
struct formatter<T, CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, T const& value,
      format_context context
   ) const -> scaredy_format<void> {
      using element = remove_const<typename T::value_type>;
      str_view const string = str_view(basic_str_span<element const>(value));
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }

   auto
   format(allocator_dyn allocator, T const& value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
