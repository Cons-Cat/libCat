// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/exception>
#include <cat/format>

// `<cat/exception>` is included by `<cat/format>`, so these specializations
// cannot be defined under `exception`. They are declared there and defined here
// after `formatter_base` exists.

namespace cat {

// Formats `exception` as its `what()` string. Matches P3256R0 for the `{}` /
// `{:?}` string cases that libCat supports.
template <typename CharT>
struct formatter<exception, CharT> : formatter<str_view, CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, exception const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return formatter<str_view, CharT>::format(
         allocator, value.what(), context
      );
   }

   auto
   format(
      allocator_dyn allocator, exception const& value, format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

// Formats an erased exception as its dispatched `what()` string.
template <is_allocator Allocator, typename CharT>
struct formatter<basic_exception_dyn<Allocator>, CharT>
    : formatter<str_view, CharT> {
   template <is_allocator FormatAllocator>
   auto
   format(
      allocator_ref<FormatAllocator> allocator,
      basic_exception_dyn<Allocator> const& value, format_context context
   ) const -> scaredy_format<void> {
      return formatter<str_view, CharT>::format(
         allocator, dyn_invoke<dyn_exception_what>(value), context
      );
   }

   auto
   format(
      allocator_dyn allocator, basic_exception_dyn<Allocator> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
