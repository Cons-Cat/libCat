// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base>
struct take_view_impl : iterable_interface<> {
   Base m_base;
   idx m_count;

   // This works around limitation in recursive `concept`s.
   static constexpr bool is_borrowing_view = !is_view_owned<Base>;

   template <typename BaseContext>
   struct context_type {
      BaseContext incoming_context;
      idx limit;
      idx taken;

      using element_type = BaseContext::element_type;

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
               return loop_body($fwd(element));
            }
         );

         if (taken >= limit) {
            return iteration_result::complete;
         }

         return nested_run_result;
      }
   };

   constexpr auto
   iterate(this auto&& self [[clang::lifetimebound]])
      requires(is_view_owned<Base> && is_iterable<decltype((self.m_base))>)
   {
      return context_type<decltype(cat::iterate(self.m_base))>{
         cat::iterate(self.m_base),
         self.m_count,
         0u,
      };
   }

   constexpr auto
   iterate(this take_view_impl self [[clang::lifetimebound]])
      requires(!is_view_owned<Base>)
   {
      return context_type<decltype(cat::iterate(self.m_base))>{
         cat::iterate(self.m_base),
         self.m_count,
         0u,
      };
   }
};

template <typename Base>
using take_view = take_view_impl<Base>;

struct take_impl : view_interface<take_impl> {
   idx count;

   template <is_borrow_acceptable Iterable>
   constexpr auto
   apply(Iterable&& incoming) const -> take_view<Iterable> {
      return take_view<Iterable>{{}, $fwd(incoming), count};
   }
};
}  // namespace detail

// Lazily yield at most `count` elements. This is a view adaptor.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
take(idx count) -> detail::take_impl {
   return detail::take_impl{{}, count};
}

}  // namespace cat
