// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

// `monostate_type` is declared before `formatter_base` exists. The
// specialization is declared under the class and defined here after
// `<cat/format>` completes `formatter_base`.

namespace cat {

template <typename CharT>
struct formatter<monostate_type, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, monostate_type /*value*/,
      format_context context
   ) const -> scaredy_format<void> {
      return context.append("monostate");
   }

   auto
   format(
      allocator_dyn allocator, monostate_type value, format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
