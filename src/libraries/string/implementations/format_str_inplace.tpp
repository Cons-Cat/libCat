// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/string>

// String headers are included by `<cat/format>`, so this specialization cannot
// be defined under `basic_str_inplace`. It is declared there and defined here
// after `formatter_base` exists.

namespace cat {

template <typename CharT, idx inline_capacity, str_vec_flags flags>
   requires(is_char_utf8_interconvertible<CharT>)
struct formatter<basic_str_inplace<CharT, inline_capacity, flags>, char>
    : debug_formatter<char> {
   auto
   format(
      basic_str_inplace<CharT, inline_capacity, flags> const& value,
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
