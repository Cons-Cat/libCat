// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator>
#include <cat/format>

// `<cat/allocator>` is on `<cat/format>`'s include graph, so allocator
// formatters cannot be defined under `allocator_interface`. Helpers and the
// specialization live here after `formatter_base` exists.

namespace cat {

namespace detail {
template <typename Allocator>
concept has_const_bytes_usage = requires(Allocator const& allocator) {
                                   {
                                      allocator.bytes_used()
                                   } -> is_convertible<idx>;
                                   {
                                      allocator.bytes_capacity()
                                   } -> is_convertible<idx>;
                                };

inline auto
format_allocator_usage(
   format_context& context, idx used, idx capacity, str_view name
) -> scaredy_format<void> {
   scaredy_format<void> result = context.append("allocator(");
   if (result.is_empty()) {
      return result;
   }
   result = format_nested(context, used);
   if (result.is_empty()) {
      return result;
   }
   result = context.append('/');
   if (result.is_empty()) {
      return result;
   }
   result = format_nested(context, capacity);
   if (result.is_empty()) {
      return result;
   }
   if (name.size() > 0u) {
      result = context.append(' ');
      if (result.is_empty()) {
         return result;
      }
      result = append_escaped(context, name, '"');
      if (result.is_empty()) {
         return result;
      }
   }
   return context.append(')');
}
}  // namespace detail

template <typename Allocator, typename CharT>
   requires(
      detail::has_const_bytes_usage<Allocator> && is_same<CharT, char>
      && !is_stepanov_iterable<Allocator>
   )
struct formatter<Allocator, CharT> : formatter_base<CharT> {
   auto
   format(Allocator const& value, format_context& context) const
      -> scaredy_format<void> {
      str_view name;
      if constexpr (requires { value.name(); }) {
         name = value.name();
      }
      return detail::format_allocator_usage(
         context, value.bytes_used(), value.bytes_capacity(), name
      );
   }
};

}  // namespace cat
