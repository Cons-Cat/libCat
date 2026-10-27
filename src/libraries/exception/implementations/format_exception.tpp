// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/exception>
#include <cat/format>

// `<cat/exception>` is included by `<cat/format>`, so these specializations
// cannot be defined under `exception`. They are declared there and defined here
// after `formatter_base` exists.

namespace cat {

// Formats `exception` as its `what()` string. Matches P3256R0 for the `{}` /
// `{:?}` string cases that libCat supports.
template <typename CharT>
struct formatter<exception, CharT> : formatter<str_view, CharT> {
   auto
   format(exception const& value, format_context& context) const
      -> scaredy_format<void> {
      return formatter<str_view, CharT>::format(value.what(), context);
   }
};

// Formats an erased exception as its dispatched `what()` string.
template <is_allocator Allocator, typename CharT>
struct formatter<basic_exception_dyn<Allocator>, CharT>
    : formatter<str_view, CharT> {
   auto
   format(
      basic_exception_dyn<Allocator> const& value, format_context& context
   ) const -> scaredy_format<void> {
      return formatter<str_view, CharT>::format(
         dyn_invoke<dyn_exception_what>(value), context
      );
   }
};

}  // namespace cat
