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
   auto
   format(bit_reference<Storage> const& value, format_context& context) const
      -> scaredy_format<void> {
      return formatter<bool, CharT>{}.format(value.is_set(), context);
   }
};

}  // namespace cat
