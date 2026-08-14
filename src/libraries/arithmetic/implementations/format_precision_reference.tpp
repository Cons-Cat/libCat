// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/format>

// Arithmetic headers are on `<cat/format>`'s include graph, so this
// specialization cannot be defined under `precision_reference`. It is declared
// there and defined here after `formatter_base` exists.

namespace cat {

template <typename WrappedQual, precision_policies policy, typename CharT>
struct formatter<precision_reference<WrappedQual, policy>, CharT>
    : formatter_base<CharT> {
   auto
   format(
      precision_reference<WrappedQual, policy> const& value,
      format_context& context
   ) const -> scaredy_format<void> {
      auto temporary = value.view();
      return detail::format_nested(context, temporary);
   }
};

}  // namespace cat
