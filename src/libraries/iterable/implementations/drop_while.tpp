// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base, typename Callback>
struct drop_while_view_impl : iterable_interface<> {
   Base m_base;
   Callback m_callback;

   template <typename BaseContext, typename Predicate>
   struct context_type {
      BaseContext incoming_context;
      Predicate* _Nonnull p_predicate;
      bool dropping = true;

      using element_type = BaseContext::element_type;

      template <is_predicate<element_type> LoopBody>
      constexpr auto
      run_while(LoopBody&& loop_body) -> iteration_result {
         return incoming_context.run_while(
            [this, &loop_body](auto&& element) -> bool {
               if (dropping && (*p_predicate)(element)) {
                  return true;
               }
               dropping = false;
               return loop_body($fwd(element));
            }
         );
      }
   };

   constexpr auto
   iterate() {
      using incoming_context = iterable_iteration_context_type<Base>;
      return context_type<incoming_context, Callback>{
         cat::iterate(m_base),
         &m_callback,
      };
   }

   constexpr auto
   iterate() const
      requires(
         is_iterable<Base const>
         && is_predicate<Callback const, iterable_element_type<Base const>>
      )
   {
      using incoming_context = iterable_iteration_context_type<Base const>;
      return context_type<incoming_context, Callback const>{
         cat::iterate(m_base),
         &m_callback,
      };
   }
};

template <typename Base, typename Callback>
using drop_while_view = drop_while_view_impl<Base, Callback>;

template <typename Callback>
struct drop_while_impl : view_interface<drop_while_impl<Callback>> {
   Callback callback;

   template <is_borrow_acceptable Iterable>
      requires(is_predicate<Callback, iterable_element_type<Iterable>>)
   constexpr auto
   apply(Iterable&& incoming) const& -> drop_while_view<Iterable, Callback> {
      return drop_while_view<Iterable, Callback>{
         {},
         $fwd(incoming),
         callback,
      };
   }

   template <is_borrow_acceptable Iterable>
      requires(is_predicate<Callback, iterable_element_type<Iterable>>)
   constexpr auto
   apply(Iterable&& incoming) && -> drop_while_view<Iterable, Callback> {
      return drop_while_view<Iterable, Callback>{
         {},
         $fwd(incoming),
         move(callback),
      };
   }
};
}  // namespace detail

// Lazily skip elements while `callback` holds. This is a view adaptor.
template <typename Callback>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
drop_while(Callback callback) -> detail::drop_while_impl<Callback> {
   return detail::drop_while_impl<Callback>{{}, move(callback)};
}

}  // namespace cat
