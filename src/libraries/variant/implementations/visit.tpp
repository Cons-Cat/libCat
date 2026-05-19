// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/tuple>
#include <cat/variant>

namespace cat {

namespace detail {

// Single-variant switch dispatcher in batches of 32. `is_valid` short-circuits
// the `__builtin_unreachable` branches for cases outside the variant's actual
// size so the optimizer can prune them.
//
// The dispatcher always invokes the callback as
// `callback.template operator()<I>(v.template get<I>())`, exposing the
// resolved alternative index as a non-type template parameter. The
// value-only `cat::visit` adapts to this contract by wrapping its
// callback in a generic lambda that ignores the index; the indexed
// `cat::visit_indexed` calls it directly. This keeps the 32-way
// `switch` body in exactly one place rather than duplicated per call
// style.
template <bool is_valid, typename Result>
struct visit_dispatcher;

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
   dispatch_case(Variant&& v, Callback&& callback) -> Result {
      return $fwd(callback).template operator()<idx(case_index)>(
         $fwd(v).template get<idx(case_index)>());
   }

   template <unsigned int base, typename Variant, typename Callback>
   [[gnu::always_inline]]
   static constexpr auto
   dispatch_switch(Variant&& v, Callback&& callback) -> Result {
      using variant_type = remove_cvref<Variant>;
      constexpr unsigned int size =
         static_cast<unsigned int>(variant_type::variant_size.raw);

      // 32 fall-through cases per `switch` block. Each case checks at
      // compile time whether it is in range before recursing.
#define CAT_VARIANT_CASE(offset)                                    \
   case base + (offset):                                            \
      return visit_dispatcher<((base + (offset)) < size), Result>:: \
         template dispatch_case<base + (offset)>($fwd(v), $fwd(callback))

      // `.value_or_niche()` reads the raw discriminant bits. On an empty
      // variant it yields the niche, which never matches a valid `case`
      // label, so the default arm routes through the unreachable false
      // specialization.
      switch (static_cast<unsigned int>(v.discriminant.value_or_niche().raw)) {
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
               template dispatch_switch<base + 32u>($fwd(v), $fwd(callback));
      }

#undef CAT_VARIANT_CASE
   }
};

// Adapter that converts an indexed-style callable
// `callback.template operator()<I>(value)` into a value-only call to the
// underlying `Callback`. Also re-introduces the per-alternative
// same-return-type `static_assert` that the previous value-only
// dispatcher had baked into its `dispatch_case`. The compiler inlines
// the adapter away through `[[gnu::always_inline]]`.
template <typename Result, typename Callback>
struct visit_value_adapter {
   Callback&& callback;

   template <idx /*case_index*/, typename Value>
   [[gnu::always_inline]]
   constexpr auto
   operator()(Value&& value) -> Result {
      using actual = decltype(invoke($fwd(callback), $fwd(value)));
      static_assert(is_same<Result, actual>,
                    "`cat::visit` requires every alternative to have the "
                    "same callback return type.");
      return invoke($fwd(callback), $fwd(value));
   }
};

}  // namespace detail

template <typename Callback, is_variant_like Variant>
[[gnu::always_inline]]
constexpr auto
visit(Callback&& callback, Variant&& v) -> decltype(auto) {
   using result_type =
      decltype(invoke($fwd(callback), $fwd(v).template get<idx(0u)>()));
   detail::visit_value_adapter<result_type, Callback> adapter{$fwd(callback)};
   return detail::visit_dispatcher<true, result_type>::template dispatch_switch<
      0u>($fwd(v), adapter);
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
            $fwd(rest)...);
      },
      $fwd(first));
}

// `cat::visit_indexed(callback, v)` is the index-aware counterpart of
// `cat::visit`. The callback's templated `operator()` is invoked as
// `callback.template operator()<I>(v.template get<I>())` where `I` is
// the active alternative's index. Used by the P3561R2 helpers below.
// Shares its switch-case dispatcher with `cat::visit`; only the call
// shape at each case differs.
template <typename Callback, is_variant_like Variant>
[[gnu::always_inline]]
constexpr auto
visit_indexed(Callback&& callback, Variant&& v) -> decltype(auto) {
   using result_type = decltype($fwd(callback).template operator()<idx{0u}>(
      $fwd(v).template get<idx(0u)>()));
   return detail::visit_dispatcher<true, result_type>::template dispatch_switch<
      0u>($fwd(v), $fwd(callback));
}

// P3561R2 `cat::visit_invoke_cases(v, fs...)`. The `I`th callable in
// `fs...` is invoked on the active alternative's value, where `I` is
// the resolved index of `v`. Number of callables must match the
// variant's alternative count.
template <is_variant_like Variant, typename... Fs>
   requires(sizeof...(Fs) == remove_cvref<Variant>::variant_size)
constexpr auto
visit_invoke_cases(Variant&& v, Fs&&... fs) -> decltype(auto) {
   return visit_indexed(
      [fs_tuple = forward_as_tuple($fwd(fs)...)]<idx index_value>(
         auto&& value) -> decltype(auto) {
         return invoke(fs_tuple.template get<index_value>(), $fwd(value));
      },
      $fwd(v));
}

// P3561R2 `cat::invoke_cases(fs...)`. Returns a callable that, when
// invoked on a variant, performs the dispatch described above. Captures
// the `fs...` by decayed value so the returned closure has independent
// lifetime.
template <typename... Fs>
constexpr auto
invoke_cases(Fs&&... fs) {
   return [fs_tuple = make_tuple($fwd(fs)...)](auto&& v) -> decltype(auto) {
      using variant_type = remove_cvref<decltype(v)>;
      static_assert(sizeof...(Fs) == variant_type::variant_size,
                    "`cat::invoke_cases` requires one callable per "
                    "alternative.");
      return visit_indexed(
         [&fs_tuple]<idx index_value>(auto&& value) -> decltype(auto) {
            return invoke(fs_tuple.template get<index_value>(), $fwd(value));
         },
         $fwd(v));
   };
}

// P3561R2 `cat::visit_apply_cases(v, fs...)`. Each alternative of `v`
// must be a `cat::tuple`. The `I`th callable is invoked via
// `cat::apply`, so a function `f_i(x, y, z)` matches a tuple-of-three
// alternative.
template <is_variant_like Variant, typename... Fs>
   requires(sizeof...(Fs) == remove_cvref<Variant>::variant_size)
constexpr auto
visit_apply_cases(Variant&& v, Fs&&... fs) -> decltype(auto) {
   return visit_indexed(
      [fs_tuple = forward_as_tuple($fwd(fs)...)]<idx index_value>(
         auto&& tup) -> decltype(auto) {
         return cat::apply(fs_tuple.template get<index_value>(), $fwd(tup));
      },
      $fwd(v));
}

// P3561R2 `cat::apply_cases(fs...)`. Curried form of `visit_apply_cases`.
template <typename... Fs>
constexpr auto
apply_cases(Fs&&... fs) {
   return [fs_tuple = make_tuple($fwd(fs)...)](auto&& v) -> decltype(auto) {
      using variant_type = remove_cvref<decltype(v)>;
      static_assert(sizeof...(Fs) == variant_type::variant_size,
                    "`cat::apply_cases` requires one callable per "
                    "alternative.");
      return visit_indexed(
         [&fs_tuple]<idx index_value>(auto&& tup) -> decltype(auto) {
            return cat::apply(fs_tuple.template get<index_value>(), $fwd(tup));
         },
         $fwd(v));
   };
}

// P3561R2 `cat::visit_apply(v, fs_tuple)`. Like `visit_apply_cases` but
// takes the callables in a `cat::tuple` rather than as a variadic pack.
template <is_variant_like Variant, typename FsTuple>
   requires(is_tuple<remove_cvref<FsTuple>>
            && remove_cvref<FsTuple>::types::size()
                  == remove_cvref<Variant>::variant_size)
constexpr auto
visit_apply(Variant&& v, FsTuple&& fs_tuple) -> decltype(auto) {
   return visit_indexed(
      [&fs_tuple]<idx index_value>(auto&& tup) -> decltype(auto) {
         return cat::apply($fwd(fs_tuple).template get<index_value>(),
                           $fwd(tup));
      },
      $fwd(v));
}

// Out-of-line definition of `variant<Alternatives...>::visit`.
template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <typename Callback>
constexpr auto
variant<Alternatives...>::visit(this auto&& self, Callback&& callback)
   -> decltype(auto) {
   return cat::visit($fwd(callback), $fwd(self).as_base());
}

}  // namespace cat
