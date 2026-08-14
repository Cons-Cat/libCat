// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bitset>
#include <cat/format>

// `<cat/bitset>` is on `<cat/format>`'s include graph, so this specialization
// cannot be defined under `bitset`. It is declared there and defined here
// after `formatter_base` exists.

namespace cat {

template <idx bits_count, typename CharT>
struct formatter<bitset<bits_count>, CharT> : formatter_base<CharT> {
   auto
   format(bitset<bits_count> const& value, format_context& context) const
      -> scaredy_format<void> {
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
         result = detail::format_nested(context, value[bit_index]);
         if (result.is_empty()) {
            return result;
         }
      }
      return context.append(']');
   }
};

}  // namespace cat
