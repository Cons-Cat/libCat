// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/simd>

// SIMD reference headers are on `<cat/format>`'s include graph, so this
// specialization cannot be defined under `simd_precision_reference`. It is
// declared there and defined here after `formatter_base` exists.

namespace cat {

template <typename WrappedQual, precision_policies policy, typename CharT>
struct formatter<detail::simd_precision_reference<WrappedQual, policy>, CharT>
    : formatter_base<CharT> {
   auto
   format(
      detail::simd_precision_reference<WrappedQual, policy> const& value,
      format_context& context
   ) const -> scaredy_format<void> {
      auto temporary = *value.m_wrapped;
      return detail::format_nested(context, temporary);
   }
};

}  // namespace cat
