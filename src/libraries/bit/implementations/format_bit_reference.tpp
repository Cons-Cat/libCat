// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>
#include <cat/format>

// `<cat/bit>` is on `<cat/format>`'s include graph, so this specialization
// cannot be defined under `bit_reference`. It is declared there and defined
// here after `formatter_base` exists.

namespace cat {

template <is_unsigned_integral Storage, typename CharT>
struct formatter<bit_reference<Storage>, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, bit_reference<Storage> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return formatter<bool, CharT>{}.format(
         allocator, value.is_set(), context
      );
   }

   auto
   format(
      allocator_dyn allocator, bit_reference<Storage> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
