// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename CharT>
struct formatter<char, CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, char value, format_context context
   ) const -> scaredy_format<void> {
      if (this->debug) {
         return detail::append_escaped(context, str_view(&value, 1u), '\'');
      }
      return context.append(value);
   }

   auto
   format(allocator_dyn allocator, char value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <typename CharT>
struct formatter<char8_t, CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, char8_t value, format_context context
   ) const -> scaredy_format<void> {
      char const code_unit = static_cast<char>(value);
      if (this->debug) {
         return detail::append_escaped(context, str_view(&code_unit, 1u), '\'');
      }
      return context.append(code_unit);
   }

   auto
   format(allocator_dyn allocator, char8_t value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
