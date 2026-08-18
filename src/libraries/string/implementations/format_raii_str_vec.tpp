// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/string>

// String headers are included by `<cat/format>`, so this specialization cannot
// be defined under `raii::basic_str_vec`. It is declared there and defined
// here after `formatter_base` exists.

namespace cat {

template <typename CharT, str_vec_flags flags, is_allocator Allocator>
   requires(is_char_utf8_interconvertible<CharT>)
struct formatter<raii::basic_str_vec<CharT, flags, Allocator>, char>
    : debug_formatter<char> {
   auto
   format(
      raii::basic_str_vec<CharT, flags, Allocator> const& value,
      format_context& context
   ) const -> scaredy_format<void> {
      str_view const string = str_view(basic_str_span<CharT const>(value));
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }
};

}  // namespace cat
