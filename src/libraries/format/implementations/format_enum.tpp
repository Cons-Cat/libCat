// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename T, typename CharT>
   requires(is_enum<T> && is_same<CharT, char>)
struct formatter<T, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, T value, format_context context
   ) const -> scaredy_format<void> {
      return formatter<underlying_type<T>, CharT>{}.format(
         allocator, static_cast<underlying_type<T>>(value), context
      );
   }

   auto
   format(allocator_dyn allocator, T value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
