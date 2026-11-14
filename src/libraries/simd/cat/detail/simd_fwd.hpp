// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// `cat::simd` concepts that headers below `<cat/simd>` can depend on without
// including it.

#include <cat/meta>

namespace cat {

template <typename T, typename Abi>
   requires(is_same<typename Abi::scalar_type, T>)
class simd;

namespace detail {

template <typename Simd>
inline constexpr bool is_cat_simd = false;

template <typename T, typename Abi>
inline constexpr bool is_cat_simd<simd<T, Abi>> = true;

}  // namespace detail

template <typename Simd>
concept is_simd = detail::is_cat_simd<__typeof_unqual(Simd)>;

template <typename Simd>
concept is_simd_integral =
   is_simd<Simd> && is_integral<typename Simd::value_type>;

}  // namespace cat
