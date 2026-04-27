// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat::detail {
template <typename T>
struct slice_view : iterable_interface {
   T* m_p_collection;
   position_type<T> m_begin;
   position_type<T> m_end;

   constexpr auto
   begin_pos() const -> position_type<T> {
      return m_begin;
   }

   constexpr auto
   end_pos() const -> position_type<T> {
      return m_end;
   }

   constexpr auto
   inc_pos(position_type<T>& position) const -> void {
      m_p_collection->inc_pos(position);
   }

   constexpr auto
   dec_pos(position_type<T>& position) const -> void
      requires(is_bidirectional_collection<T>)
   {
      m_p_collection->dec_pos(position);
   }

   constexpr auto
   inc_pos(position_type<T>& position, iword count) const -> void
      requires(is_random_access_collection<T>)
   {
      m_p_collection->inc_pos(position, count);
   }

   constexpr auto
   distance(position_type<T> const& left, position_type<T> const& right) const
      -> iword
      requires(is_random_access_collection<T>)
   {
      return m_p_collection->distance(left, right);
   }

   constexpr auto
   read_at_unchecked(position_type<T> const& position) const -> decltype(auto) {
      return m_p_collection->read_at_unchecked(position);
   }
};
}  // namespace cat::detail
