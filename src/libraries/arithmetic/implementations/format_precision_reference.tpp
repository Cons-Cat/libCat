// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/format>

// Arithmetic headers are on `<cat/format>`'s include graph, so this
// specialization cannot be defined under `precision_reference`. It is declared
// there and defined here after `formatter_base` exists.

namespace cat {

template <is_floating_point Float, precision_policies policy, typename CharT>
struct formatter<precision_reference<Float, policy>, CharT>
    : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator,
      precision_reference<Float, policy> const& value, format_context context
   ) const -> scaredy_format<void> {
      auto temporary = value.view();
      return formatter<decltype(temporary)>{}.format(
         allocator, temporary, context
      );
   }

   auto
   format(
      allocator_dyn allocator, precision_reference<Float, policy> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
