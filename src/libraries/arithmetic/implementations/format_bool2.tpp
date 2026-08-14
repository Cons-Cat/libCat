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
   auto
   format(bool2 value, format_context& context) const -> scaredy_format<void> {
      return formatter<bool, CharT>{}.format(static_cast<bool>(value), context);
   }
};

}  // namespace cat
