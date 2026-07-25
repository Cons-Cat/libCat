// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base, typename Callback>
struct drop_last_while_view_impl : iterable_interface<> {
   Base m_base;
   Callback m_callback;

 private:
   template <is_collection Collection, typename Predicate>
   static constexpr auto
   suffix_begin(Collection& collection, Predicate& predicate)
      -> position_type<Collection> {
      auto const first = begin_pos(collection);
      auto const last = end_pos(collection);

      if constexpr (is_bidirectional_collection<Collection>) {
         auto boundary = last;
         while (first < boundary) {
            auto candidate = boundary;
            dec_pos(collection, candidate);
            if (!predicate(read_at_unchecked(collection, candidate))) {
               break;
            }
            boundary = candidate;
         }
         return boundary;
      } else {
         auto position = first;
         auto boundary = first;
         while (position < last) {
            auto const current = position;
            inc_pos(collection, position);
            if (!predicate(read_at_unchecked(collection, current))) {
               boundary = position;
            }
         }
         return boundary;
      }
   }

 public:
   constexpr auto
   iterate() {
      auto& collection = unwrap_ref(m_base);
      using collection_type = remove_reference<decltype(collection)>;
      return collection_iteration_context<collection_type>{
         __builtin_addressof(collection),
         begin_pos(collection),
         suffix_begin(collection, m_callback),
      };
   }

   constexpr auto
   iterate() const
      requires(
         is_collection<decltype(unwrap_ref(m_base))>
         && is_predicate<
            Callback const, iterable_element_type<decltype(unwrap_ref(m_base))>>
      )
   {
      auto& collection = unwrap_ref(m_base);
      using collection_type = remove_reference<decltype(collection)>;
      return collection_iteration_context<collection_type>{
         __builtin_addressof(collection),
         begin_pos(collection),
         suffix_begin(collection, m_callback),
      };
   }
};

template <typename Base, typename Callback>
using drop_last_while_view = drop_last_while_view_impl<Base, Callback>;

template <typename Callback>
struct drop_last_while_impl : view_interface<drop_last_while_impl<Callback>> {
   Callback callback;

   template <is_borrow_acceptable Iterable>
      requires(
         is_collection<unwrap_ref_decay<Iterable>>
         && is_predicate<
            Callback, iterable_element_type<unwrap_ref_decay<Iterable>>>
      )
   constexpr auto
   apply(
      Iterable&& incoming
   ) const& -> drop_last_while_view<Iterable, Callback> {
      return drop_last_while_view<Iterable, Callback>{
         {},
         $fwd(incoming),
         callback,
      };
   }

   template <is_borrow_acceptable Iterable>
      requires(
         is_collection<unwrap_ref_decay<Iterable>>
         && is_predicate<
            Callback, iterable_element_type<unwrap_ref_decay<Iterable>>>
      )
   constexpr auto
   apply(Iterable&& incoming) && -> drop_last_while_view<Iterable, Callback> {
      return drop_last_while_view<Iterable, Callback>{
         {},
         $fwd(incoming),
         move(callback),
      };
   }
};
}  // namespace detail

// Lazily omit the trailing elements accepted by `callback`. This view adaptor
// requires a collection so it can locate the suffix without buffering elements.
template <typename Callback>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
drop_last_while(Callback callback) -> detail::drop_last_while_impl<Callback> {
   return detail::drop_last_while_impl<Callback>{{}, move(callback)};
}

}  // namespace cat
