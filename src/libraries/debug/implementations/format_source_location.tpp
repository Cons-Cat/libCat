// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/debug>
#include <cat/format>

// `<cat/debug>` is on `<cat/format>`'s include graph, so this specialization
// cannot be defined under `source_location`. It is declared there and defined
// here after `formatter_base` exists.

namespace cat {

template <typename CharT>
struct formatter<source_location, CharT> : formatter_base<CharT> {
   auto
   format(source_location const& value, format_context& context) const
      -> scaredy_format<void> {
      scaredy_format<void> result = context.append(value.file_name());
      if (result.is_empty()) {
         return result;
      }
      result = context.append(':');
      if (result.is_empty()) {
         return result;
      }
      result = detail::format_nested(context, value.line());
      if (result.is_empty()) {
         return result;
      }
      result = context.append(':');
      if (result.is_empty()) {
         return result;
      }
      result = detail::format_nested(context, value.column());
      if (result.is_empty()) {
         return result;
      }
      result = context.append(':');
      if (result.is_empty()) {
         return result;
      }
      return context.append(value.function_name());
   }
};

}  // namespace cat
