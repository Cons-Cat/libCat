// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bitset>
#include <cat/format>

// `<cat/bitset>` is on `<cat/format>`'s include graph, so this specialization
// cannot be defined under `bitset`. It is declared there and defined here
// after `formatter_base` exists.

namespace cat {

template <idx bits_count, typename T, typename CharT>
struct formatter<bitset<bits_count, T>, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, bitset<bits_count, T> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      scaredy_format<void> result = context.append('[');
      if (result.is_empty()) {
         return result;
      }
      bool first = true;
      for (idx bit_index; bit_index < value.size(); ++bit_index) {
         if (!first) {
            result = context.append(", ");
            if (result.is_empty()) {
               return result;
            }
         }
         first = false;
         result = formatter<__typeof_unqual(value[bit_index])>{}.format(
            allocator, value[bit_index], context
         );
         if (result.is_empty()) {
            return result;
         }
      }
      return context.append(']');
   }

   auto
   format(
      allocator_dyn allocator, bitset<bits_count, T> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
