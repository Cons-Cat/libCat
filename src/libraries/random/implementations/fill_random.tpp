// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/fill_random.hpp>
#include <cat/detail/iterable_interface.hpp>

#include <cat/iterable>

namespace cat {
namespace detail {

template <typename Generator>
struct fill_random_impl {
   Generator generator;

   template <is_iterable Range>
      requires(!is_const<Range>)
   friend constexpr auto
   operator|(Range&& range, fill_random_impl self) -> decltype(auto) {
      auto&& generator = $fwd(self).generator;
      cat::fill_random(range, $fwd(generator));
      return $fwd(range);
   }
};

template <typename Generator, typename Distribution>
struct fill_random_distribution_impl {
   Generator generator;
   Distribution distribution;

   template <is_iterable Range>
      requires(!is_const<Range>)
   friend constexpr auto
   operator|(Range&& range, fill_random_distribution_impl self)
      -> decltype(auto) {
      auto&& generator = $fwd(self).generator;
      auto&& distribution = $fwd(self).distribution;
      cat::fill_random(range, $fwd(generator), $fwd(distribution));
      return $fwd(range);
   }
};

}  // namespace detail

template <typename Generator>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
fill_random(Generator&& generator) -> detail::fill_random_impl<Generator> {
   return {$fwd(generator)};
}

template <typename Generator, typename Distribution>
   requires(!is_iterable<remove_cvref<Generator>>)
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
fill_random(Generator&& generator, Distribution&& distribution)
   -> detail::fill_random_distribution_impl<Generator, Distribution> {
   return {$fwd(generator), $fwd(distribution)};
}

template <typename Tag>
template <typename Self, typename Generator>
constexpr auto
iterable_interface<Tag>::fill_random(this Self&& self, Generator&& generator)
   -> decltype(auto) {
   return $fwd(self) | cat::fill_random($fwd(generator));
}

template <typename Tag>
template <typename Self, typename Generator, typename Distribution>
constexpr auto
iterable_interface<Tag>::fill_random(
   this Self&& self, Generator&& generator, Distribution&& distribution
) -> decltype(auto) {
   return $fwd(self) | cat::fill_random($fwd(generator), $fwd(distribution));
}

}  // namespace cat
