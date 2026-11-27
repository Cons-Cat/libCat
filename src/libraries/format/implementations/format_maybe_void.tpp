// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

// `void` isn't a regular type so it can't really get its own formatter, and we
// therefore need this to special-case it.
template <typename CharT>
struct formatter<maybe<void>, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, maybe<void> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      if (value.has_value()) {
         return context.append("[void]");
      }
      return context.append("[]");
   }

   auto
   format(
      allocator_dyn allocator, maybe<void> const& value, format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
