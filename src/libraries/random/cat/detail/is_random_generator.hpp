// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/simd_fwd.hpp>

#include <cat/meta>

namespace cat {

// Holds for a uniform random bit generator, or a reference to one, so that it
// can constrain a forwarded `Generator&&` parameter.
template <typename Generator>
concept is_random_generator =
   (is_integral<typename remove_reference<Generator>::result_type>
    || is_simd_integral<typename remove_reference<Generator>::result_type>)
   && requires(Generator& generator) {
         {
            generator()
         } -> is_same<typename remove_reference<Generator>::result_type>;
         remove_reference<Generator>::min();
         remove_reference<Generator>::max();
      };

}  // namespace cat
