// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base>
struct drop_view_impl : iterable_interface<> {
   Base m_base;
   idx m_count;

   template <typename BaseContext>
   struct context_type {
      BaseContext incoming_context;
      idx count;
      idx dropped;

      using element_type = BaseContext::element_type;

      template <is_predicate<element_type> LoopBody>
      constexpr auto
      run_while(LoopBody&& loop_body) -> iteration_result {
         return incoming_context.run_while(
            [this, &loop_body](auto&& element) -> bool {
               if (dropped < count) {
                  ++dropped;
                  return true;
               }
               return loop_body($fwd(element));
            }
         );
      }
   };

   constexpr auto
   iterate() {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base>;
      return context_type<incoming_iteration_context_type>{
         cat::iterate(m_base),
         m_count,
         0u,
      };
   }

   constexpr auto
   iterate() const
      requires(is_iterable<Base const>)
   {
      using incoming_iteration_context_type =
         iterable_iteration_context_type<Base const>;
      return context_type<incoming_iteration_context_type>{
         cat::iterate(m_base),
         m_count,
         0u,
      };
   }
};

template <typename Base>
using drop_view = drop_view_impl<Base>;

struct drop_impl : view_interface<drop_impl> {
   idx count;

   template <is_borrow_acceptable Iterable>
   constexpr auto
   apply(Iterable&& incoming) const -> drop_view<Iterable> {
      return drop_view<Iterable>{{}, $fwd(incoming), count};
   }
};
}  // namespace detail

// Lazily skip the first `count` elements. This is a view adaptor.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
drop(idx count) -> detail::drop_impl {
   return detail::drop_impl{{}, count};
}

}  // namespace cat
