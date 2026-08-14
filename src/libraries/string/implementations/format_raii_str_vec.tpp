// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/string>

// String headers are included by `<cat/format>`, so this specialization cannot
// be defined under `raii::basic_str_vec`. It is declared there and defined
// here after `formatter_base` exists.

namespace cat {

template <bool null_terminated, is_allocator Allocator, typename CharT>
   requires(is_same<CharT, char>)
struct formatter<raii::basic_str_vec<char, null_terminated, Allocator>, CharT>
    : debug_formatter<CharT> {
   auto
   format(
      raii::basic_str_vec<char, null_terminated, Allocator> const& value,
      format_context& context
   ) const -> scaredy_format<void> {
      return detail::format_char_string(this->debug, value, context);
   }
};

}  // namespace cat
