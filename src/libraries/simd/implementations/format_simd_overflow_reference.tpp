// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/simd>

// SIMD reference headers are on `<cat/format>`'s include graph, so this
// specialization cannot be defined under `simd_overflow_reference`. It is
// declared there and defined here after `formatter_base` exists.

namespace cat {

template <typename T, typename Abi, overflow_policies policy, typename CharT>
struct formatter<detail::simd_overflow_reference<T, Abi, policy>, CharT>
    : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator,
      detail::simd_overflow_reference<T, Abi, policy> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      auto temporary = *value.m_wrapped;
      return formatter<decltype(temporary)>{}.format(
         allocator, temporary, context
      );
   }

   auto
   format(
      allocator_dyn allocator,
      detail::simd_overflow_reference<T, Abi, policy> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
