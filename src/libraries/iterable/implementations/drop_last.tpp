// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base>
struct drop_last_view_impl : iterable_interface<> {
   Base m_base;
   idx m_count;

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

         auto const result = incoming_context.run_while(
            [this, &loop_body](auto&& element) -> bool {
               if (taken >= limit) {
                  return false;
               }
               ++taken;
               return loop_body($fwd(element));
            }
         );
         return taken >= limit ? iteration_result::complete : result;
      }
   };

 private:
   template <is_collection Collection>
   static constexpr auto
   suffix_begin(Collection& collection, idx count)
      -> position_type<Collection> {
      auto const begin = begin_pos(collection);
      auto const last = end_pos(collection);

      if constexpr (is_bidirectional_collection<Collection>) {
         auto first = last;
         for (idx i = 0u; i < count && begin < first; ++i) {
            dec_pos(collection, first);
         }
         return first;
      } else {
         auto first = begin;
         auto probe = begin;
         for (idx i = 0u; i < count && probe < last; ++i) {
            inc_pos(collection, probe);
         }
         while (probe < last) {
            inc_pos(collection, first);
            inc_pos(collection, probe);
         }
         return first;
      }
   }

 public:
   constexpr auto
   iterate() {
      auto& collection = unwrap_ref(m_base);
      if constexpr (is_collection<decltype(collection)>) {
         using collection_type = remove_reference<decltype(collection)>;
         return collection_iteration_context<collection_type>{
            __builtin_addressof(collection),
            begin_pos(collection),
            suffix_begin(collection, m_count),
         };
      } else {
         idx const count = collection.size();
         idx const limit = count > m_count ? idx(count - m_count) : 0u;
         using incoming_context = iterable_iteration_context_type<Base>;
         return context_type<incoming_context>{
            cat::iterate(m_base),
            limit,
            0u,
         };
      }
   }

   constexpr auto
   iterate() const
      requires(
         is_collection<decltype(unwrap_ref(m_base))>
         || has_size<decltype(unwrap_ref(m_base))>
      )
   {
      auto& collection = unwrap_ref(m_base);
      if constexpr (is_collection<decltype(collection)>) {
         using collection_type = remove_reference<decltype(collection)>;
         return collection_iteration_context<collection_type>{
            __builtin_addressof(collection),
            begin_pos(collection),
            suffix_begin(collection, m_count),
         };
      } else {
         idx const count = collection.size();
         idx const limit = count > m_count ? idx(count - m_count) : 0u;
         using incoming_context = iterable_iteration_context_type<Base const>;
         return context_type<incoming_context>{
            cat::iterate(m_base),
            limit,
            0u,
         };
      }
   }
};

template <typename Base>
using drop_last_view = drop_last_view_impl<Base>;

struct drop_last_impl : view_interface<drop_last_impl> {
   idx count;

   template <is_borrow_acceptable Iterable>
      requires(
         is_collection<unwrap_ref_decay<Iterable>>
         || has_size<unwrap_ref_decay<Iterable>>
      )
   constexpr auto
   apply(Iterable&& incoming) const -> drop_last_view<Iterable> {
      return drop_last_view<Iterable>{{}, $fwd(incoming), count};
   }
};
}  // namespace detail

// Lazily omit the last `count` elements. This view adaptor requires a
// collection or a sized iterable so it can locate the suffix without buffering.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
drop_last(idx count) -> detail::drop_last_impl {
   return detail::drop_last_impl{{}, count};
}

}  // namespace cat
