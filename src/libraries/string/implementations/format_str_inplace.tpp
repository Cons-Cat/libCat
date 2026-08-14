// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/string>

// String headers are included by `<cat/format>`, so this specialization cannot
// be defined under `basic_str_inplace`. It is declared there and defined here
// after `formatter_base` exists.

namespace cat {

template <
   idx inline_capacity, bool null_terminated, vec_flags flags, typename CharT>
   requires(is_same<CharT, char>)
struct formatter<
   basic_str_inplace<char, inline_capacity, null_terminated, flags>, CharT>
    : debug_formatter<CharT> {
   auto
   format(
      basic_str_inplace<char, inline_capacity, null_terminated, flags> const&
         value,
      format_context& context
   ) const -> scaredy_format<void> {
      return detail::format_char_string(this->debug, value, context);
   }
};

}  // namespace cat
