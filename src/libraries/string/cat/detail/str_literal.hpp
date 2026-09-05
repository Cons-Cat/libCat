// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/array>
#include <cat/str_literal>

#include "str_span.hpp"

namespace cat {

template <typename CharT, __SIZE_TYPE__ fixed_size>
constexpr auto
basic_str_literal<CharT, fixed_size>::view() const [[clang::lifetimebound]]
-> basic_str_literal<CharT, fixed_size>::view_type {
   return {data_, fixed_size};
}

template <typename CharT, __SIZE_TYPE__ fixed_size>
constexpr basic_str_literal<CharT, fixed_size>::
operator typename basic_str_literal<CharT, fixed_size>::view_type() const
   [[clang::lifetimebound]] {
   return view();
}

template <typename CharT, __SIZE_TYPE__ fixed_size>
template <typename Predicate>
constexpr auto
basic_str_literal<CharT, fixed_size>::filter(Predicate&& predicate) const {
   return view().filter(static_cast<Predicate&&>(predicate));
}

template <typename CharT, idx size>
basic_str_literal(array<CharT, size> const&)
   -> basic_str_literal<CharT, size.raw>;

}  // namespace cat
