// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/tuple>
#include <cat/variant>

namespace cat {

namespace detail {

// Single-`variant` recursive `switch` dispatcher in batches of 32. Michael Park
// determined this is the optimal batch size.
template <bool is_valid, typename Result>
struct visit_dispatcher;

// These recursive base cases let the compiler optimize unreachable `case`s.
template <typename Result>
struct visit_dispatcher<false, Result> {
   template <unsigned int /*case_index*/, typename Variant, typename Callback>
   [[gnu::always_inline]]
   static constexpr auto
   dispatch_case(Variant&& /*variant*/, Callback&& /*callback*/) -> Result {
      __builtin_unreachable();
   }

   template <unsigned int /*base*/, typename Variant, typename Callback>
   [[gnu::always_inline]]
   static constexpr auto
   dispatch_switch(Variant&& /*variant*/, Callback&& /*callback*/) -> Result {
      __builtin_unreachable();
   }
};

template <typename Result>
struct visit_dispatcher<true, Result> {
   template <unsigned int case_index, typename Variant, typename Callback>
   [[gnu::always_inline]]
   static constexpr auto
   dispatch_case(Variant&& variant, Callback&& callback) -> Result {
      return $fwd(callback).template operator()<idx(case_index)>(
         $fwd(variant).template get<idx(case_index)>()
      );
   }

   template <unsigned int base, typename Variant, typename Callback>
   [[gnu::always_inline]]
   static constexpr auto
   dispatch_switch(Variant&& variant, Callback&& callback) -> Result {
      using variant_type = remove_cvref<Variant>;
      constexpr unsigned int size =
         static_cast<unsigned int>(variant_type::variant_size.raw);

      // 32 fall-through cases per `switch` block. Each case checks at
      // compile time whether it is in range before recursing.
#define CAT_VARIANT_CASE(offset)                                    \
   case base + (offset):                                            \
      return visit_dispatcher<((base + (offset)) < size), Result>:: \
         template dispatch_case<base + (offset)>(                   \
            $fwd(variant), $fwd(callback)                           \
         )

      // `.value_or_niche()` reads the raw discriminant bits. On an empty
      // variant it yields the niche, which never matches a valid `case`
      // label, so the `default` branch matches the `false` partial
      // template specializations.
      switch (static_cast<unsigned int>(variant.index().value_or_niche().raw)) {
         CAT_VARIANT_CASE(0u);
         CAT_VARIANT_CASE(1u);
         CAT_VARIANT_CASE(2u);
         CAT_VARIANT_CASE(3u);
         CAT_VARIANT_CASE(4u);
         CAT_VARIANT_CASE(5u);
         CAT_VARIANT_CASE(6u);
         CAT_VARIANT_CASE(7u);
         CAT_VARIANT_CASE(8u);
         CAT_VARIANT_CASE(9u);
         CAT_VARIANT_CASE(10u);
         CAT_VARIANT_CASE(11u);
         CAT_VARIANT_CASE(12u);
         CAT_VARIANT_CASE(13u);
         CAT_VARIANT_CASE(14u);
         CAT_VARIANT_CASE(15u);
         CAT_VARIANT_CASE(16u);
         CAT_VARIANT_CASE(17u);
         CAT_VARIANT_CASE(18u);
         CAT_VARIANT_CASE(19u);
         CAT_VARIANT_CASE(20u);
         CAT_VARIANT_CASE(21u);
         CAT_VARIANT_CASE(22u);
         CAT_VARIANT_CASE(23u);
         CAT_VARIANT_CASE(24u);
         CAT_VARIANT_CASE(25u);
         CAT_VARIANT_CASE(26u);
         CAT_VARIANT_CASE(27u);
         CAT_VARIANT_CASE(28u);
         CAT_VARIANT_CASE(29u);
         CAT_VARIANT_CASE(30u);
         CAT_VARIANT_CASE(31u);
         default:
            return visit_dispatcher<((base + 32u) < size), Result>::
               template dispatch_switch<base + 32u>(
                  $fwd(variant), $fwd(callback)
               );
      }

#undef CAT_VARIANT_CASE
   }
};

}  // namespace detail

template <typename Callback, is_variant_like Variant>
[[gnu::always_inline]]
constexpr auto
visit(Callback&& callback, Variant&& variant) noexcept(
   !__has_feature(cxx_exceptions)
) -> decltype(auto) {
   cat::assert(variant.has_value());
   using result_type =
      decltype(invoke($fwd(callback), $fwd(variant).template get<0u>()));
   // The dispatcher calls `.template operator()<index>(value)`, so the index
   // is accepted and ignored.
   return detail::visit_dispatcher<true, result_type>::template dispatch_switch<
      0u>($fwd(variant), [&]<idx>(auto&& value) -> result_type {
      static_assert(
         is_same<result_type, decltype(invoke($fwd(callback), $fwd(value)))>,
         "`cat::visit` requires every alternative to have the same callback "
         "return type!"
      );
      return invoke($fwd(callback), $fwd(value));
   });
}

// `cat::visit(callback, v0, v1, ...)` folds the visitor across the variants.
// Each visit wraps the next in a lambda that captures the already-resolved
// argument by perfect forwarding.
template <typename Callback, is_variant_like First, is_variant_like... Rest>
   requires(sizeof...(Rest) > 0)
[[gnu::always_inline]]
constexpr auto
visit(Callback&& callback, First&& first, Rest&&... rest) -> decltype(auto) {
   return cat::visit(
      [&](auto&& head) -> decltype(auto) {
         return cat::visit(
            [&](auto&&... tail) -> decltype(auto) {
               return invoke($fwd(callback), $fwd(head), $fwd(tail)...);
            },
            $fwd(rest)...
         );
      },
      $fwd(first)
   );
}

// `cat::visit<Result>(callback, variant...)` converts every alternative's
// result to `Result`, like C++20's `std::visit<R>`, or discards it if `Result`
// is `void`.
template <typename Result, typename Callback, is_variant_like... Variants>
   requires(sizeof...(Variants) > 0)
[[gnu::always_inline]]
constexpr auto
visit(Callback&& callback, Variants&&... variants) -> Result {
   return cat::visit(
      [&](auto&&... values) -> Result {
         if constexpr (is_void<Result>) {
            invoke($fwd(callback), $fwd(values)...);
         } else {
            return invoke($fwd(callback), $fwd(values)...);
         }
      },
      $fwd(variants)...
   );
}

// `cat::visit_indexed(callback, variant)` is the index-aware counterpart of
// `cat::visit`. The callback's templated `operator()` is invoked as
// `callback.template operator()<I>(variant.template get<I>())` where `I` is
// the active alternative's index. Used by the P3561R2 helpers below.
// Shares its switch-case dispatcher with `cat::visit`; only the call
// shape at each case differs.
template <typename Callback, is_variant_like Variant>
[[gnu::always_inline]]
constexpr auto
visit_indexed(Callback&& callback, Variant&& variant) noexcept(
   !__has_feature(cxx_exceptions)
) -> decltype(auto) {
   cat::assert(variant.has_value());
   using result_type = decltype($fwd(callback).template operator()<idx{0u}>(
      $fwd(variant).template get<0u>()
   ));
   return detail::visit_dispatcher<true, result_type>::template dispatch_switch<
      0u>($fwd(variant), $fwd(callback));
}

// P3561R2 `cat::visit_invoke_cases(variant, fs...)`. The `I`th callable in
// `fs...` is invoked on the active alternative's value, where `I` is
// the resolved index of `variant`. Number of callables must match the
// variant's alternative count.
template <is_variant_like Variant, typename... Fs>
   requires(sizeof...(Fs) == remove_cvref<Variant>::variant_size)
constexpr auto
visit_invoke_cases(Variant&& variant, Fs&&... fs) -> decltype(auto) {
   return visit_indexed(
      [fs_tuple = forward_as_tuple($fwd(fs)...)]<idx index_value>(auto&& value)
         -> decltype(auto) {
         return invoke(fs_tuple.template get<index_value>(), $fwd(value));
      },
      $fwd(variant)
   );
}

// P3561R2 `cat::invoke_cases(fs...)`. Returns a callable that, when
// invoked on a variant, performs the dispatch described above. Captures
// the `fs...` by decayed value so the returned closure has independent
// lifetime.
template <typename... Fs>
constexpr auto
invoke_cases(Fs&&... fs) {
   return
      [fs_tuple = make_tuple($fwd(fs)...)](auto&& variant) -> decltype(auto) {
         using variant_type = __typeof_unqual(variant);
         static_assert(
            sizeof...(Fs) == variant_type::variant_size,
            "`cat::invoke_cases` requires one callable per "
            "alternative!"
         );
         return visit_indexed(
            [&fs_tuple]<idx index_value>(auto&& value) -> decltype(auto) {
               return invoke(fs_tuple.template get<index_value>(), $fwd(value));
            },
            $fwd(variant)
         );
      };
}

// P3561R2 `cat::visit_apply_cases(variant, fs...)`. Each alternative of
// `variant` must be a `cat::tuple`. The `I`th callable is invoked via
// `cat::apply`, so a function `f_i(x, y, z)` matches a tuple-of-three
// alternative.
template <is_variant_like Variant, typename... Fs>
   requires(sizeof...(Fs) == remove_cvref<Variant>::variant_size)
constexpr auto
visit_apply_cases(Variant&& variant, Fs&&... fs) -> decltype(auto) {
   return visit_indexed(
      [fs_tuple = forward_as_tuple($fwd(fs)...)]<idx index_value>(auto&& tup)
         -> decltype(auto) {
         return cat::apply(fs_tuple.template get<index_value>(), $fwd(tup));
      },
      $fwd(variant)
   );
}

// P3561R2 `cat::apply_cases(fs...)`. Curried form of `visit_apply_cases`.
template <typename... Fs>
constexpr auto
apply_cases(Fs&&... fs) {
   return [fs_tuple =
              make_tuple($fwd(fs)...)](auto&& variant) -> decltype(auto) {
      using variant_type = __typeof_unqual(variant);
      static_assert(
         sizeof...(Fs) == variant_type::variant_size,
         "`cat::apply_cases` requires one callable per "
         "alternative!"
      );
      return visit_indexed(
         [&fs_tuple]<idx index_value>(auto&& tup) -> decltype(auto) {
            return cat::apply(fs_tuple.template get<index_value>(), $fwd(tup));
         },
         $fwd(variant)
      );
   };
}

// P3561R2 `cat::visit_apply(variant, fs_tuple)`. Like `visit_apply_cases` but
// takes the callables in a `cat::tuple` rather than as a variadic pack.
template <is_variant_like Variant, typename FsTuple>
   requires(
      is_tuple<remove_cvref<FsTuple>>
      && remove_cvref<FsTuple>::types::size()
            == remove_cvref<Variant>::variant_size
   )
constexpr auto
visit_apply(Variant&& variant, FsTuple&& fs_tuple) -> decltype(auto) {
   return visit_indexed(
      [&fs_tuple]<idx index_value>(auto&& tup) -> decltype(auto) {
         return cat::apply(
            $fwd(fs_tuple).template get<index_value>(), $fwd(tup)
         );
      },
      $fwd(variant)
   );
}

// Out-of-line definition of `detail::variant_base<Alternatives...>::visit`.
namespace detail {
template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <typename Callback>
constexpr auto
variant_base<Alternatives...>::visit(this auto&& self, Callback&& callback)
   -> decltype(auto) {
   return cat::visit(
      $fwd(callback),
      static_cast<copy_cvref_from<decltype(self), variant<Alternatives...>>>(
         $fwd(self)
      )
   );
}
}  // namespace detail

}  // namespace cat
