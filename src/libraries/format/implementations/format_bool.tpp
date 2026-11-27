// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/format>

// Arithmetic headers are on `<cat/format>`'s include graph, so the `bool2` and
// `bool4` specializations cannot be defined under those types. They are
// declared there and defined here after `formatter_base` exists.

namespace cat {

template <typename CharT>
struct formatter<bool, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, bool value, format_context context
   ) const -> scaredy_format<void> {
      return context.append(value ? str_view("true") : str_view("false"));
   }

   auto
   format(allocator_dyn allocator, bool value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <typename CharT>
struct formatter<bool2, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, bool2 value, format_context context
   ) const -> scaredy_format<void> {
      return formatter<bool, CharT>{}.format(
         allocator, static_cast<bool>(value), context
      );
   }

   auto
   format(allocator_dyn allocator, bool2 value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <typename CharT>
struct formatter<bool4, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, bool4 value, format_context context
   ) const -> scaredy_format<void> {
      return formatter<bool, CharT>{}.format(
         allocator, static_cast<bool>(value), context
      );
   }

   auto
   format(allocator_dyn allocator, bool4 value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
