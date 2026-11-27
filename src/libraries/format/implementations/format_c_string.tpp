// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename T, typename CharT>
   requires(
      is_pointer<T> && is_char_utf8_interconvertible<remove_pointer<T>>
      && is_same<CharT, char>
   )
struct formatter<T, CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, T value, format_context context
   ) const -> scaredy_format<void> {
      if (value == nullptr) {
         return context.append("(null)");
      }
      using element = remove_const<remove_pointer<T>>;
      str_view const string = str_view(basic_str_span<element const>(value));
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }

   auto
   format(allocator_dyn allocator, T value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
