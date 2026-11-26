// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/array>
#include <cat/str_literal>

#include "str_span.hpp"

namespace cat {

template <typename CharT, __SIZE_TYPE__ fixed_size>
constexpr basic_str_literal<CharT, fixed_size>::
operator basic_str_span<CharT const, str_flags{}>() const
   [[clang::lifetimebound]] {
   return {data_, fixed_size};
}

template <typename CharT, idx size>
basic_str_literal(array<CharT, size> const&)
   -> basic_str_literal<CharT, size.raw>;

}  // namespace cat
