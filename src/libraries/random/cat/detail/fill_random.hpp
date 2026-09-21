// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>
#include <cat/simd_switch>

namespace cat {
namespace detail {

template <typename T>
constexpr auto
random_batch_fill_enabled() -> bool {
   if constexpr (requires { T::enable_batch_fill; }) {
      return T::enable_batch_fill;
   } else {
      return true;
   }
}

template <typename T>
constexpr auto
random_relaxed_bulk_fill_enabled() -> bool {
   if constexpr (requires { T::enable_relaxed_bulk_fill; }) {
      return T::enable_relaxed_bulk_fill;
   } else {
      return false;
   }
}

template <typename T>
constexpr auto
random_exact_bulk_fill_enabled() -> bool {
   if constexpr (requires { T::enable_exact_bulk_fill; }) {
      return T::enable_exact_bulk_fill;
   } else {
      return false;
   }
}

template <typename T>
constexpr auto
random_relaxed_distribution_fill_enabled() -> bool {
   if constexpr (requires { T::enable_relaxed_distribution_fill; }) {
      return T::enable_relaxed_distribution_fill;
   } else {
      return false;
   }
}

template <is_simd Simd, typename Element, typename Generator>
constexpr void
fill_random_batch_contiguous(
   Element* _Nonnull p_data, idx size, Generator& generator
) {
   using memory_lane = Simd::value_type;
   memory_lane* _Nonnull p_lanes = __builtin_bit_cast(memory_lane*, p_data);
   idx index = 0u;
   while (index + Simd::abi_type::lanes <= size) {
      generate_exact_random_batch<typename Simd::abi_type>(generator).store(
         p_lanes + index
      );
      index += Simd::abi_type::lanes;
   }
   while (index < size) {
      p_data[index] = generator();
      ++index;
   }
}

template <typename Abi, typename Generator>
consteval auto
exact_random_bulk_chain_count() -> idx {
   if constexpr (Abi::size >= 64u) {
      return 2u;
   } else {
      return 4u;
   }
}

template <typename Abi, typename Generator>
consteval auto
exact_random_bulk_threshold() -> idx {
   return Abi::size >= 64u ? 256u : 128u;
}

template <is_simd Simd, idx chain_count, typename Element, typename Generator>
constexpr void
fill_random_exact_bulk_contiguous(
   Element* _Nonnull p_data, idx size, Generator& generator
) {
   auto session =
      make_exact_random_bulk_session<typename Simd::abi_type, chain_count>(
         generator
      );
   using memory_lane = Simd::value_type;
   memory_lane* _Nonnull p_lanes = __builtin_bit_cast(memory_lane*, p_data);
   constexpr idx lanes = Simd::abi_type::lanes;
   idx index = 0u;

   while (index + lanes * chain_count <= size) {
      session.template generate<0u>().store(p_lanes + index);
      if constexpr (chain_count >= 2u) {
         session.template generate<1u>().store(p_lanes + index + lanes);
      }
      if constexpr (chain_count == 4u) {
         session.template generate<2u>().store(p_lanes + index + lanes * 2u);
         session.template generate<3u>().store(p_lanes + index + lanes * 3u);
      }
      index += lanes * chain_count;
   }

   auto const consume = [&]<idx chain>() -> bool {
      if (index == size) {
         return true;
      }
      Simd const values = session.template generate<chain>();
      if (index + lanes <= size) {
         values.store(p_lanes + index);
         index += lanes;
         return index == size;
      }
      for (idx lane = 0u; index < size; ++lane) {
         p_data[index] = values[lane];
         ++index;
      }
      return true;
   };

   if (!consume.template operator()<0u>()) {
      if constexpr (chain_count >= 2u) {
         if (!consume.template operator()<1u>()) {
            if constexpr (chain_count == 4u) {
               if (!consume.template operator()<2u>()) {
                  static_cast<void>(consume.template operator()<3u>());
               }
            }
         }
      }
   }
   generator.discard(uint8(size));
}

template <typename Element, typename Generator>
constexpr auto
fill_random_exact_bulk(Element* _Nonnull p_data, idx size, Generator& generator)
   -> bool {
   if constexpr (random_exact_bulk_fill_enabled<Generator>()) {
      if !consteval {
         if (size < 128u) {
            return false;
         }
#ifdef CAT_NO_CPUID
         using vector_type = native_simd<typename Generator::result_type>;
         constexpr idx chain_count = exact_random_bulk_chain_count<
            typename vector_type::abi_type, Generator>();
         if constexpr (requires {
                          make_exact_random_bulk_session<
                             typename vector_type::abi_type, chain_count>(
                             generator
                          );
                       }) {
            if (
               size >= exact_random_bulk_threshold<
                  typename vector_type::abi_type, Generator>()
            ) {
               fill_random_exact_bulk_contiguous<vector_type, chain_count>(
                  p_data, size, generator
               );
               return true;
            }
         }
#else
         return $simd_switch($abi((sse2, avx2), {
            using vector_type = native_simd<typename Generator::result_type>;
            constexpr idx chain_count = exact_random_bulk_chain_count<
               typename vector_type::abi_type, Generator>();
            if constexpr (requires {
                             make_exact_random_bulk_session<
                                typename vector_type::abi_type, chain_count>(
                                generator
                             );
                          }) {
               if (
                  size >= exact_random_bulk_threshold<
                     typename vector_type::abi_type, Generator>()
               ) {
                  fill_random_exact_bulk_contiguous<vector_type, chain_count>(
                     p_data, size, generator
                  );
                  return true;
               }
            }
            return false;
         }));
#endif
      }
   }
   return false;
}

template <is_simd Simd, typename Element, typename Generator>
constexpr void
fill_random_relaxed_bulk_contiguous(
   Element* _Nonnull p_data, idx size, Generator& generator
) {
   auto session =
      make_relaxed_random_bulk_session<typename Simd::abi_type>(generator);
   using memory_lane = Simd::value_type;
   memory_lane* _Nonnull p_lanes = __builtin_bit_cast(memory_lane*, p_data);
   idx index = 0u;
   constexpr idx lanes = Simd::abi_type::lanes;
   while (index + lanes * 2u <= size) {
      session.first().store(p_lanes + index);
      session.second().store(p_lanes + index + lanes);
      index += lanes * 2u;
   }
   bool used_first = false;
   if (index + lanes <= size) {
      session.first().store(p_lanes + index);
      index += lanes;
      used_first = true;
   }
   if (index < size) {
      Simd const values = used_first ? session.second() : session.first();
      for (idx lane = 0u; index < size; ++lane) {
         p_data[index] = values[lane];
         ++index;
      }
   }
   generator.discard(uint8(size));
}

template <typename Batch, typename Element, typename Generate>
constexpr void
fill_random_distribution_batch_contiguous(
   Element* _Nonnull p_data, idx size, Generate&& generate
) {
   idx index = 0u;
   if constexpr (
      is_simd<Batch> && is_same<Element, typename Batch::value_type>
   ) {
      using memory_lane = Batch::value_type;
      memory_lane* _Nonnull p_lanes = __builtin_bit_cast(memory_lane*, p_data);
      while (index + Batch::abi_type::lanes <= size) {
         generate().store(p_lanes + index);
         index += Batch::abi_type::lanes;
      }
   } else {
      while (index + Batch::abi_type::lanes <= size) {
         Batch const values = generate();
         for (idx lane = 0u; lane < Batch::abi_type::lanes; ++lane) {
            p_data[index + lane] = values[lane];
         }
         index += Batch::abi_type::lanes;
      }
   }
   if (index < size) {
      Batch const values = generate();
      for (idx lane = 0u; index < size; ++lane) {
         p_data[index] = values[lane];
         ++index;
      }
   }
}

template <
   typename Batch, typename Element, typename Generator, typename Distribution,
   typename Abi>
constexpr void
fill_random_relaxed_distribution_contiguous(
   Element* _Nonnull p_data, idx size, Generator& generator,
   Distribution& distribution
) {
   auto session = make_relaxed_random_bulk_session<Abi>(generator);
   fill_random_distribution_batch_contiguous<Batch>(p_data, size, [&] {
      return generate_random_distribution_batch<Abi>(distribution, session);
   });
   generator.discard(uint8(session.scalar_draws()));
}

template <is_iterable Range, typename Generator>
constexpr auto
fill_random_scalar_batch(Range&& range, Generator& generator) -> bool {
   auto&& unwrapped = unwrap_ref(range);
   if constexpr (
      is_random_access_collection<decltype(unwrapped)> && requires {
                                                             unwrapped.data();
                                                             unwrapped.size();
                                                          }
   ) {
      using value_type = __typeof_unqual(*unwrapped.data());
      using result_type = Generator::result_type;
      if constexpr (is_same<value_type, result_type>) {
         if (
            fill_random_exact_bulk(
               unwrapped.data(), unwrapped.size(), generator
            )
         ) {
            return true;
         }
         if constexpr (requires {
                          make_relaxed_random_bulk_session<
                             typename native_simd<result_type>::abi_type>(
                             generator
                          );
                       } && random_relaxed_bulk_fill_enabled<Generator>()) {
            if !consteval {
#ifdef CAT_NO_CPUID
               using vector_type = native_simd<result_type>;
               if (
                  unwrapped.size() >= relaxed_random_bulk_threshold<
                     typename vector_type::abi_type, Generator>()
               ) {
                  fill_random_relaxed_bulk_contiguous<vector_type>(
                     unwrapped.data(), unwrapped.size(), generator
                  );
                  return true;
               }
#else
               bool const filled = $simd_switch($abi((sse2, avx2), {
                  using vector_type = native_simd<result_type>;
                  if (
                     unwrapped.size() >= relaxed_random_bulk_threshold<
                        typename vector_type::abi_type, Generator>()
                  ) {
                     fill_random_relaxed_bulk_contiguous<vector_type>(
                        unwrapped.data(), unwrapped.size(), generator
                     );
                     return true;
                  }
                  return false;
               }));
               if (filled) {
                  return true;
               }
#endif
            }
         }
         if constexpr (!random_batch_fill_enabled<Generator>()) {
            return false;
         }
         if consteval {
            return false;
         } else {
#ifdef CAT_NO_CPUID
            using vector_type = native_simd<result_type>;
            if constexpr (requires {
                             generate_exact_random_batch<
                                typename vector_type::abi_type>(generator);
                          }) {
               fill_random_batch_contiguous<vector_type>(
                  unwrapped.data(), unwrapped.size(), generator
               );
               return true;
            }
#else
            return $simd_switch($abi((sse2, avx2), {
               using vector_type = native_simd<result_type>;
               if constexpr (requires {
                                generate_exact_random_batch<
                                   typename vector_type::abi_type>(generator);
                             }) {
                  fill_random_batch_contiguous<vector_type>(
                     unwrapped.data(), unwrapped.size(), generator
                  );
                  return true;
               } else {
                  return false;
               }
            }));
#endif
         }
      }
   }
   return false;
}

template <is_iterable Range, typename Generator, typename Distribution>
constexpr auto
fill_random_scalar_distribution_batch(
   Range&& range, Generator& generator, Distribution& distribution
) -> bool {
   auto&& unwrapped = unwrap_ref(range);
   if constexpr (
      is_random_access_collection<decltype(unwrapped)> && requires {
                                                             unwrapped.data();
                                                             unwrapped.size();
                                                          }
   ) {
      using value_type = __typeof_unqual(*unwrapped.data());
      if constexpr (!random_batch_fill_enabled<Distribution>()) {
         return false;
      }
      if (!distribution_batch_available(distribution)) {
         return false;
      }
      if consteval {
         return false;
      } else {
         if constexpr (
            requires {
               make_relaxed_random_bulk_session<typename native_simd<
                  typename Generator::result_type>::abi_type>(generator);
            } && random_relaxed_bulk_fill_enabled<Generator>()
            && random_relaxed_distribution_fill_enabled<Generator>()
         ) {
#ifdef CAT_NO_CPUID
            using abi_type =
               typename native_simd<typename Generator::result_type>::abi_type;
            if (
               unwrapped.size()
               >= relaxed_random_bulk_threshold<abi_type, Generator>()
            ) {
               using session_type = __typeof_unqual(
                  make_relaxed_random_bulk_session<abi_type>(generator)
               );
               if constexpr (requires(session_type& session) {
                                generate_random_distribution_batch<abi_type>(
                                   distribution, session
                                );
                             }) {
                  using batch_type = remove_cvref<
                     decltype(generate_random_distribution_batch<abi_type>(
                        distribution, declval<session_type&>()
                     ))>;
                  if constexpr (
                     (is_simd<batch_type>
                      && is_same<value_type, typename batch_type::value_type>)
                     || (is_simd_mask<batch_type> && is_bool<value_type>)
                  ) {
                     fill_random_relaxed_distribution_contiguous<
                        batch_type, value_type, Generator, Distribution,
                        abi_type>(
                        unwrapped.data(), unwrapped.size(), generator,
                        distribution
                     );
                     return true;
                  }
               }
            }
#else
            bool const filled = $simd_switch($abi((sse2, avx2), {
               using abi_type = typename native_simd<
                  typename Generator::result_type>::abi_type;
               if (
                  unwrapped.size()
                  < relaxed_random_bulk_threshold<abi_type, Generator>()
               ) {
                  return false;
               }
               using session_type = __typeof_unqual(
                  make_relaxed_random_bulk_session<abi_type>(generator)
               );
               if constexpr (requires(session_type& session) {
                                generate_random_distribution_batch<abi_type>(
                                   distribution, session
                                );
                             }) {
                  using batch_type = remove_cvref<
                     decltype(generate_random_distribution_batch<abi_type>(
                        distribution, declval<session_type&>()
                     ))>;
                  if constexpr (
                     (is_simd<batch_type>
                      && is_same<value_type, typename batch_type::value_type>)
                     || (is_simd_mask<batch_type> && is_bool<value_type>)
                  ) {
                     fill_random_relaxed_distribution_contiguous<
                        batch_type, value_type, Generator, Distribution,
                        abi_type>(
                        unwrapped.data(), unwrapped.size(), generator,
                        distribution
                     );
                     return true;
                  }
               }
               return false;
            }));
            if (filled) {
               return true;
            }
#endif
         }
#ifdef CAT_NO_CPUID
         using abi_type =
            typename native_simd<typename Generator::result_type>::abi_type;
         if constexpr (requires {
                          generate_random_distribution_batch<abi_type>(
                             distribution, generator
                          );
                       }) {
            using batch_type =
               __typeof_unqual(generate_random_distribution_batch<abi_type>(
                  distribution, generator
               ));
            if constexpr (
               (is_simd<batch_type>
                && is_same<value_type, typename batch_type::value_type>)
               || (is_simd_mask<batch_type> && is_bool<value_type>)
            ) {
               fill_random_distribution_batch_contiguous<batch_type>(
                  unwrapped.data(), unwrapped.size(), [&] {
                     return generate_random_distribution_batch<abi_type>(
                        distribution, generator
                     );
                  }
               );
               return true;
            }
         }
#else
         return $simd_switch($abi((sse2, avx2), {
            using abi_type =
               typename native_simd<typename Generator::result_type>::abi_type;
            if constexpr (requires {
                             generate_random_distribution_batch<abi_type>(
                                distribution, generator
                             );
                          }) {
               using batch_type =
                  __typeof_unqual(generate_random_distribution_batch<abi_type>(
                     distribution, generator
                  ));
               if constexpr (
                  (is_simd<batch_type>
                   && is_same<value_type, typename batch_type::value_type>)
                  || (is_simd_mask<batch_type> && is_bool<value_type>)
               ) {
                  fill_random_distribution_batch_contiguous<batch_type>(
                     unwrapped.data(), unwrapped.size(), [&] {
                        return generate_random_distribution_batch<abi_type>(
                           distribution, generator
                        );
                     }
                  );
                  return true;
               }
            }
            return false;
         }));
#endif
      }
   }
   return false;
}

template <typename Simd, is_iterable Range>
constexpr void
fill_random_simd_scattered(Range&& range, auto&& generate) {
   Simd values;
   idx lane = Simd::abi_type::lanes;
   iterate(range).run_while([&](auto&& value) -> bool {
      if (lane == Simd::abi_type::lanes) {
         values = generate();
         lane = 0u;
      }
      value = values[lane];
      ++lane;
      return true;
   });
}

template <typename Simd, typename Element>
constexpr void
fill_random_simd_contiguous(
   Element* _Nonnull p_data, idx size, auto&& generate
) {
   using simd_result = __typeof_unqual(generate());
   simd_result source_values;
   idx source_lane = simd_result::abi_type::lanes;
   idx index = 0u;
   using memory_lane = Simd::value_type;
   memory_lane* _Nonnull p_lanes = __builtin_bit_cast(memory_lane*, p_data);

   if constexpr (is_same<simd_result, Simd>) {
      while (index + Simd::abi_type::lanes <= size) {
         generate().store(p_lanes + index);
         index += Simd::abi_type::lanes;
      }
      if (index < size) {
         source_values = generate();
         source_lane = 0u;
      }
   }

   while (index + Simd::abi_type::lanes <= size) {
      Simd output;
      for (idx output_lane = 0u; output_lane < Simd::abi_type::lanes;
           ++output_lane) {
         if (source_lane == simd_result::abi_type::lanes) {
            source_values = generate();
            source_lane = 0u;
         }
         output.set_lane(output_lane, source_values[source_lane]);
         ++source_lane;
      }
      output.store(p_lanes + index);
      index += Simd::abi_type::lanes;
   }

   while (index < size) {
      if (source_lane == simd_result::abi_type::lanes) {
         source_values = generate();
         source_lane = 0u;
      }
      p_data[index] = source_values[source_lane];
      ++source_lane;
      ++index;
   }
}

template <typename Simd, is_iterable Range>
constexpr void
fill_random_simd(Range&& range, auto&& generate) {
   auto&& unwrapped = unwrap_ref(range);
   if constexpr (
      is_random_access_collection<decltype(unwrapped)> && requires {
                                                             unwrapped.data();
                                                             unwrapped.size();
                                                          }
   ) {
      using value_type = __typeof_unqual(*unwrapped.data());
      if constexpr (is_arithmetic<value_type> && !is_bool<value_type>) {
         if consteval {
            fill_random_simd_scattered<Simd>($fwd(range), $fwd(generate));
         } else {
#ifdef CAT_NO_CPUID
            fill_random_simd_contiguous<native_simd<value_type>>(
               unwrapped.data(), unwrapped.size(), $fwd(generate)
            );
#else
            $simd_switch($abi((sse2, avx2), {
               fill_random_simd_contiguous<native_simd<value_type>>(
                  unwrapped.data(), unwrapped.size(), $fwd(generate)
               );
            }));
#endif
         }
         return;
      }
   }
   fill_random_simd_scattered<Simd>($fwd(range), $fwd(generate));
}

}  // namespace detail

// Inspired by P1068R11's explicit engine range generation, adapted to libCat
// iterables and SIMD results.
// https://open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1068r11.pdf
template <is_iterable Range, typename Generator>
constexpr auto
fill_random(Range&& range, Generator&& generator) -> iteration_result {
   if constexpr (requires { generator.fill_random($fwd(range)); }) {
      generator.fill_random($fwd(range));
   } else if constexpr (is_simd_random_bit_generator<remove_cvref<Generator>>) {
      using simd_result = remove_cvref<Generator>::result_type;
      detail::fill_random_simd<simd_result>($fwd(range), [&generator] {
         return generator();
      });
   } else if (detail::fill_random_scalar_batch(range, generator)) {
   } else {
      iterate(range).run_while([&](auto&& value) -> bool {
         value = generator();
         return true;
      });
   }
   return iteration_result::complete;
}

template <is_iterable Range, typename Generator, typename Distribution>
constexpr auto
fill_random(Range&& range, Generator&& generator, Distribution&& distribution)
   -> iteration_result {
   if constexpr (requires {
                    distribution.fill_random($fwd(range), generator);
                 }) {
      distribution.fill_random($fwd(range), generator);
   } else if constexpr (is_simd_or_mask<decltype(distribution(generator))>) {
      using simd_result = decltype(distribution(generator));
      detail::fill_random_simd<simd_result>(
         $fwd(range), [&distribution, &generator] {
            return distribution(generator);
         }
      );
   } else if (
      detail::fill_random_scalar_distribution_batch(
         range, generator, distribution
      )
   ) {
   } else {
      iterate(range).run_while([&](auto&& value) -> bool {
         value = distribution(generator);
         return true;
      });
   }
   return iteration_result::complete;
}

}  // namespace cat
