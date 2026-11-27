// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/format>

// Arithmetic headers are on `<cat/format>`'s include graph, so this
// specialization cannot be defined under `bool2`. It is declared there and
// defined here after `formatter_base` exists.

namespace cat {

template <typename CharT>
struct formatter<bool2, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, bool2 value, format_context context
   ) const -> scaredy_format<void> {
      return formatter<bool, CharT>{}.format(
         allocator, static_cast<bool>(value), context
      );
   }

   auto
   format(allocator_dyn allocator, bool2 value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
