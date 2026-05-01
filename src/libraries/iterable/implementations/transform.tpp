// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base, typename Callback>
struct transform_view_impl : iterable_interface {
   Base m_base;
   Callback m_callback;

   template <typename BaseContext, typename HeldCallback>
   struct context_type {
      BaseContext incoming_context;
      HeldCallback* callback_pointer;

      using element_type = decltype(declval<HeldCallback&>()(
         declval<typename BaseContext::element_type>()));

      template <is_predicate<element_type> LoopBody>
      constexpr auto
      run_while(LoopBody&& loop_body) -> iteration_result {
         return incoming_context.run_while(
            [this, &loop_body](auto&& element) -> bool {
               return loop_body((*callback_pointer)($fwd(element)));
            });
      }
   };

   constexpr auto
   iterate() {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base>;
      return context_type<incoming_iteration_context_type, Callback>{
         ::cat::iterate(m_base), &m_callback};
   }

   constexpr auto
   iterate() const
      requires(is_iterable<Base const>)
   {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base const>;
      return context_type<incoming_iteration_context_type, Callback const>{
         ::cat::iterate(m_base), &m_callback};
   }

   constexpr auto
   reverse_iterate()
      requires(is_reverse_iterable<Base>)
   {
      using incoming_iteration_context_type =
         decltype(::cat::reverse_iterate(m_base));
      return context_type<incoming_iteration_context_type, Callback>{
         ::cat::reverse_iterate(m_base), &m_callback};
   }

   constexpr auto
   reverse_iterate() const
      requires(is_reverse_iterable<Base const>)
   {
      using incoming_iteration_context_type =
         decltype(::cat::reverse_iterate(m_base));
      return context_type<incoming_iteration_context_type, Callback const>{
         ::cat::reverse_iterate(m_base), &m_callback};
   }
};

template <typename Base, typename Callback>
using transform_view = transform_view_impl<Base, Callback>;

template <typename Callback>
struct transform_impl : view_interface<transform_impl<Callback>> {
   Callback callback;

   template <is_borrow_acceptable Iterable>
   constexpr auto
   apply(Iterable&& incoming) const& -> transform_view<Iterable, Callback> {
      return transform_view<Iterable, Callback>{{}, $fwd(incoming), callback};
   }

   template <is_borrow_acceptable Iterable>
   constexpr auto
   apply(Iterable&& incoming) && -> transform_view<Iterable, Callback> {
      return transform_view<Iterable, Callback>{
         {}, $fwd(incoming), move(callback)};
   }
};
}  // namespace detail

// Lazy element-wise transform.
template <typename Callback>
constexpr auto
transform(Callback callback) -> detail::transform_impl<Callback> {
   return detail::transform_impl<Callback>{{}, move(callback)};
}

}  // namespace cat
