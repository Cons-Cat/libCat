// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

// Given a pack of types, `common_reference` picks the type that every `T` in
// `Ts...` can bind to, preferring a reference when one fits without a copy. The
// full rule table lives at
// https://en.cppreference.com/w/cpp/types/common_reference.

namespace cat {
namespace detail {

// The type that a ternary `true ? T : U` would deduce, or fails to deduce if no
// such type exists.
template <typename T, typename U>
using conditionally_resolve_common_reference =
   decltype(true ? declval<T (&)()>()() : declval<U (&)()>()());

// `common_reference_detail` resolves the common reference for the four
// ref-qualifier combinations of `T` and `U`.

// Anything not covered by the specializations below falls through to this
// catch-all and fails to deduce:
template <
   typename T, typename U, typename TQual = remove_reference<T>,
   typename UQual = remove_reference<U>>
struct common_reference_detail_trait;

template <typename T, typename U>
using common_reference_detail = common_reference_detail_trait<T, U>::type;

template <typename T, typename U>
using cv_conditional_resolve = conditionally_resolve_common_reference<
   copy_cv_from<T, U>&, copy_cv_from<U, T>&>;

// T&, U&
// The cv-aware ternary result, only when that result is itself a reference.
template <typename T, typename U, typename TQual, typename UQual>
   requires(requires {
               typename cv_conditional_resolve<TQual, UQual>;
            } && is_reference<cv_conditional_resolve<TQual, UQual>>)
struct common_reference_detail_trait<T&, U&, TQual, UQual> {
   using type = cv_conditional_resolve<TQual, UQual>;
};

template <typename T, typename U>
using common_reference_detail_c =
   remove_reference<common_reference_detail<T&, U&>>&&;

// T&&, U&&
// Take the lvalue common reference and collapse it to an
// rvalue reference, but only if both `T&&` and `U&&` convert to it.
template <typename T, typename U, typename TQual, typename UQual>
   requires(
      requires { typename common_reference_detail_c<TQual, UQual>; }
      && is_convertible<T &&, common_reference_detail_c<TQual, UQual>>
      && is_convertible<U &&, common_reference_detail_c<TQual, UQual>>
   )
struct common_reference_detail_trait<T&&, U&&, TQual, UQual> {
   using type = common_reference_detail_c<TQual, UQual>;
};

template <typename T, typename U>
using common_reference_detail_d = common_reference_detail<T const&, U&>;

// T&&, U&
// Borrow `COMMON-REF(T const&, U&)`, but only if `T&&` still
// converts to it.
template <typename T, typename U, typename TQual, typename UQual>
   requires(requires {
               typename common_reference_detail_d<TQual, UQual>;
            } && is_convertible<T &&, common_reference_detail_d<TQual, UQual>>)
struct common_reference_detail_trait<T&&, U&, TQual, UQual> {
   using type = common_reference_detail_d<TQual, UQual>;
};

// T&, U&&
// The mirror of `(T&&, U&)`. Just forward.
template <typename T, typename U, typename TQual, typename UQual>
struct common_reference_detail_trait<T&, U&&, TQual, UQual>
    : common_reference_detail_trait<U&&, T&> {};

// Catch-all: anything else fails to deduce a `type` member.
template <typename T, typename U, typename TQual, typename UQual>
struct common_reference_detail_trait {};

// The 2-argument case walks a chain of sub-bullets. Each level falls
// through to the next via inheritance when its own constraint does not
// hold.
template <typename T, typename U>
struct common_reference_sub_bullet_3;

template <typename T, typename U>
struct common_reference_sub_bullet_2 : common_reference_sub_bullet_3<T, U> {};

template <typename T, typename U>
struct common_reference_sub_bullet_1 : common_reference_sub_bullet_2<T, U> {};

// Sub-bullet 1
// Both sides are references, `COMMON-REF(T, U)` resolves, and
// `add_pointer<T>` / `add_pointer<U>` both convert to its pointer form -
// the last two checks make sure the result is reachable from each side.
template <typename T, typename U>
   requires(
      is_reference<T> && is_reference<U>
      && requires { typename common_reference_detail<T, U>; }
      && is_convertible<
         add_pointer<T>, add_pointer<common_reference_detail<T, U>>>
      && is_convertible<
         add_pointer<U>, add_pointer<common_reference_detail<T, U>>>
   )
struct common_reference_sub_bullet_1<T, U> {
   using type = common_reference_detail<T, U>;
};

// Customization point for sub-bullet 2. The std equivalent
// (`basic_common_reference`) takes template-template parameters so a
// specialization can write `TQual<X>` to re-apply the source cv-ref. Cat
// passes the qualified types directly and specializations call
// `copy_cvref_from<...>` where `TQual<X>` would have gone.
template <typename, typename, typename, typename>
struct basic_common_reference_trait {};

template <typename T, typename U>
using basic_common_reference =
   basic_common_reference_trait<remove_cvref<T>, remove_cvref<U>, T, U>::type;

// Sub-bullet 2
// Whatever a `basic_common_reference` specialization produced for this pair.
template <typename T, typename U>
   requires(requires { typename basic_common_reference<T, U>; })
struct common_reference_sub_bullet_2<T, U> {
   using type = basic_common_reference<T, U>;
};

// Sub-bullet 3
// The plain ternary `true ? T : U` result.
template <typename T, typename U>
   requires(requires { typename conditionally_resolve_common_reference<T, U>; })
struct common_reference_sub_bullet_3<T, U> {
   using type = conditionally_resolve_common_reference<T, U>;
};

// Sub-bullet 3 fallback
// Inherit from `common_type_trait<T, U>`. Its `type` member only exists
// when `__builtin_common_type` found a common type, so the inheritance
// fails to deduce by itself if there is none.
template <typename T, typename U>
struct common_reference_sub_bullet_3 : common_type_trait<T, U> {};

// Catch-all primary. Any pack that no specialization below matches lands
// here with no `type` member, including a 3+ pack whose inner pair has no
// common reference.
template <typename...>
struct common_reference_trait {};

// 1 argument: itself.
template <typename T>
struct common_reference_trait<T> {
   using type = T;
};

// 2 arguments: run the sub-bullet chain.
template <typename T, typename U>
struct common_reference_trait<T, U> : common_reference_sub_bullet_1<T, U> {};

// 3+ arguments: left-fold over pairs. The `requires` guard catches an inner
// pair with no common reference and falls back to the catch-all instead of
// hitting `::type` on an incomplete trait.
template <typename T, typename U, typename V, typename... Remaining>
   requires(requires { typename common_reference_trait<T, U>::type; })
struct common_reference_trait<T, U, V, Remaining...>
    : common_reference_trait<
         typename common_reference_trait<T, U>::type, V, Remaining...> {};

// `cat::reference_wrapper<W>` participates as if it were `W&`. `TQuals` and
// `UQuals` carry the original qualified types, so each specialization can
// re-apply the source cv-ref via `copy_cvref_from<...>` at the spots where
// the std spec would write `TQual<X>`.
template <typename W, typename U, typename TQuals, typename UQuals>
   requires(
      requires {
         typename common_reference_trait<W&, copy_cvref_from<UQuals, U>>::type;
      }
      && is_convertible<
         copy_cvref_from<TQuals, reference_wrapper<W>>,
         typename common_reference_trait<W&, copy_cvref_from<UQuals, U>>::type>
   )
struct basic_common_reference_trait<reference_wrapper<W>, U, TQuals, UQuals> {
   using type = common_reference_trait<W&, copy_cvref_from<UQuals, U>>::type;
};

template <typename T, typename W, typename TQuals, typename UQuals>
   requires(
      requires {
         typename common_reference_trait<copy_cvref_from<TQuals, T>, W&>::type;
      }
      && is_convertible<
         copy_cvref_from<UQuals, reference_wrapper<W>>,
         typename common_reference_trait<copy_cvref_from<TQuals, T>, W&>::type>
   )
struct basic_common_reference_trait<T, reference_wrapper<W>, TQuals, UQuals> {
   using type = common_reference_trait<copy_cvref_from<TQuals, T>, W&>::type;
};

template <typename W1, typename W2, typename TQuals, typename UQuals>
   requires(requires { typename common_reference_trait<W1&, W2&>::type; })
struct basic_common_reference_trait<
   reference_wrapper<W1>, reference_wrapper<W2>, TQuals, UQuals> {
   using type = common_reference_trait<W1&, W2&>::type;
};

}  // namespace detail

template <typename... Args>
using common_reference = detail::common_reference_trait<Args...>::type;

}  // namespace cat
