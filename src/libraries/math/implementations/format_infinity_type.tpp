// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/math>

// `<cat/math>` is on `<cat/format>`'s include graph, so this specialization
// cannot be defined under `infinity_type`. It is declared there and defined
// here after `formatter_base` exists.

namespace cat {

template <typename CharT>
struct formatter<detail::infinity_type, CharT> : formatter_base<CharT> {
   auto
   format(detail::infinity_type value, format_context& context) const
      -> scaredy_format<void> {
      float8 const as_float = value;
      return context.append(as_float < 0. ? str_view("-inf") : str_view("inf"));
   }
};

}  // namespace cat
