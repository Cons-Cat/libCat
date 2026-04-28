// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base>
struct as_rvalue_view_impl : iterable_interface {
   Base m_base;

   template <typename BaseContext>
   struct context_type {
      BaseContext incoming_context;

      using element_type =
         remove_reference<typename BaseContext::element_type>&&;

      template <is_predicate<element_type> LoopBody>
      constexpr auto
      run_while(LoopBody&& loop_body) -> iteration_result {
         return incoming_context.run_while(
            [&loop_body](auto&& element) -> bool {
               return loop_body(move(element));  // NOLINT
            });
      }
   };

   constexpr auto
   iterate() {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base>;
      return context_type<incoming_iteration_context_type>{
         ::cat::iterate(m_base)};
   }

   constexpr auto
   iterate() const
      requires(is_iterable<Base const>)
   {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base const>;
      return context_type<incoming_iteration_context_type>{
         ::cat::iterate(m_base)};
   }

   constexpr auto
   reverse_iterate()
      requires(is_reverse_iterable<Base>)
   {
      using incoming_iteration_context_type =
         decltype(::cat::reverse_iterate(m_base));
      return context_type<incoming_iteration_context_type>{
         ::cat::reverse_iterate(m_base)};
   }

   constexpr auto
   reverse_iterate() const
      requires(is_reverse_iterable<Base const>)
   {
      using incoming_iteration_context_type =
         decltype(::cat::reverse_iterate(m_base));
      return context_type<incoming_iteration_context_type>{
         ::cat::reverse_iterate(m_base)};
   }
};

template <typename Base>
using as_rvalue_view = as_rvalue_view_impl<Base>;

struct as_rvalue_impl : view_interface<as_rvalue_impl> {
   template <is_borrow_acceptable Iterable>
   constexpr auto
   apply(Iterable&& incoming) const -> as_rvalue_view<Iterable> {
      return as_rvalue_view<Iterable>{{}, fwd(incoming)};
   }
};
}  // namespace detail

// `as_rvalue()` `cat::move()`s every incoming element so an outgoing terminal
// can accept x-values. Internal iteration makes this safe even after
// `filter | reverse` because the base context stops at its own bounds, so the
// broken `std::views` pipeline
// `views::filter | views::reverse | views::as_rvalue | ranges::to` (example
// from P3725) becomes a single fused loop where the wrapper only ever observes
// elements the base actually yielded.
constexpr auto
as_rvalue() -> detail::as_rvalue_impl {
   return detail::as_rvalue_impl{{}};
}

}  // namespace cat
