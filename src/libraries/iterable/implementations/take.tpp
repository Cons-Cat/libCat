// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat::detail {
template <typename Base>
struct take_view_impl : iterable_interface {
   Base m_base;
   idx m_count;

   template <typename BaseContext>
   struct context_type {
      BaseContext incoming_context;
      idx limit;
      idx taken;

      using element_type = typename BaseContext::element_type;

      template <is_predicate<element_type> LoopBody>
      constexpr auto
      run_while(LoopBody&& loop_body) -> iteration_result {
         if (taken >= limit) {
            return iteration_result::complete;
         }

         auto const nested_run_result = incoming_context.run_while(
            [this, &loop_body](auto&& element) -> bool {
               if (taken >= limit) {
                  return false;
               }
               ++taken;
               return loop_body(fwd(element));
            });

         if (taken >= limit) {
            return iteration_result::complete;
         }

         return nested_run_result;
      }
   };

   constexpr auto
   iterate() {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base>;
      return context_type<incoming_iteration_context_type>{
         ::cat::iterate(m_base), m_count, 0u};
   }

   constexpr auto
   iterate() const
      requires(is_iterable<Base const>)
   {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base const>;
      return context_type<incoming_iteration_context_type>{
         ::cat::iterate(m_base), m_count, 0u};
   }
};

template <typename Base>
using take_view = take_view_impl<Base>;

struct take_impl : view_interface<take_impl> {
   idx count;

   template <is_borrow_acceptable Iterable>
   constexpr auto
   apply(Iterable&& incoming) const -> take_view<Iterable> {
      return take_view<Iterable>{{}, fwd(incoming), count};
   }
};
}  // namespace cat::detail

namespace cat {

// Access a fixed number of elements from the input. This is a terminal
// algorithm.
constexpr auto
take(idx count) -> detail::take_impl {
   return detail::take_impl{{}, count};
}

}  // namespace cat
