// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/string>

// String headers are included by `<cat/format>`, so this specialization cannot
// be defined under `basic_str_vec`. It is declared there and defined here
// after `formatter_base` exists.

namespace cat {

template <str_vec_flags flags, typename CharT>
   requires(is_same<CharT, char>)
struct formatter<basic_str_vec<char, flags>, CharT> : debug_formatter<CharT> {
   auto
   format(
      basic_str_vec<char, flags> const& value, format_context& context
   ) const -> scaredy_format<void> {
      char const* p_data = value.data();
      str_view const string =
         p_data == nullptr ? str_view() : str_view(p_data, value.size());
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }
};

}  // namespace cat
