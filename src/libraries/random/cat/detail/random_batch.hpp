// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

namespace cat::detail {

class random_batch_access {
 public:
   template <typename Abi, typename Generator>
   [[nodiscard]]
   static constexpr auto
   exact(Generator& generator)
      -> decltype(generator.template generate_exact_batch<Abi>()) {
      return generator.template generate_exact_batch<Abi>();
   }

   template <typename Abi, idx chain_count, typename Generator>
   [[nodiscard]]
   static constexpr auto
   exact_bulk(Generator const& generator)
      -> decltype(generator
                     .template make_exact_bulk_session<Abi, chain_count>()) {
      return generator.template make_exact_bulk_session<Abi, chain_count>();
   }

   template <typename Abi, typename Generator>
   [[nodiscard]]
   static constexpr auto
   relaxed(Generator const& generator)
      -> decltype(generator.template make_relaxed_session<Abi>()) {
      return generator.template make_relaxed_session<Abi>();
   }

   template <typename Abi, typename Generator>
   [[nodiscard]]
   static constexpr auto
   relaxed_threshold()
      -> decltype(Generator::template relaxed_threshold<Abi>()) {
      return Generator::template relaxed_threshold<Abi>();
   }
};

template <typename Abi, typename Generator>
[[nodiscard]]
constexpr auto
generate_exact_random_batch(Generator& generator)
   -> decltype(random_batch_access::exact<Abi>(generator)) {
   return random_batch_access::exact<Abi>(generator);
}

template <typename Abi, idx chain_count, typename Generator>
[[nodiscard]]
constexpr auto
make_exact_random_bulk_session(Generator const& generator)
   -> decltype(random_batch_access::exact_bulk<Abi, chain_count>(generator)) {
   return random_batch_access::exact_bulk<Abi, chain_count>(generator);
}

template <typename Abi, typename Generator>
[[nodiscard]]
constexpr auto
make_relaxed_random_bulk_session(Generator const& generator)
   -> decltype(random_batch_access::relaxed<Abi>(generator)) {
   return random_batch_access::relaxed<Abi>(generator);
}

template <typename Abi, typename Generator>
[[nodiscard]]
constexpr auto
relaxed_random_bulk_threshold()
   -> decltype(random_batch_access::relaxed_threshold<Abi, Generator>()) {
   return random_batch_access::relaxed_threshold<Abi, Generator>();
}

class distribution_batch_access {
 public:
   template <typename Distribution>
   [[nodiscard]]
   static constexpr auto
   available(Distribution const& distribution) -> bool {
      if constexpr (requires { distribution.batch_available(); }) {
         return distribution.batch_available();
      } else {
         return true;
      }
   }
};

template <typename Distribution>
[[nodiscard]]
constexpr auto
distribution_batch_available(Distribution const& distribution) -> bool {
   return distribution_batch_access::available(distribution);
}

}  // namespace cat::detail
