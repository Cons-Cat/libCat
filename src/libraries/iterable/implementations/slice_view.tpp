// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
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
      ::cat::inc_pos(*m_p_collection, position);
   }

   constexpr auto
   dec_pos(position_type<T>& position) const -> void
      requires(is_bidirectional_collection<T>)
   {
      ::cat::dec_pos(*m_p_collection, position);
   }

   constexpr auto
   offset_pos(position_type<T>& position, iword offset) const -> void
      requires(is_random_access_collection<T>)
   {
      ::cat::offset_pos(*m_p_collection, position, offset);
   }

   constexpr auto
   distance(position_type<T> const& left, position_type<T> const& right) const
      -> iword
      requires(is_random_access_collection<T>)
   {
      return ::cat::distance(*m_p_collection, left, right);
   }

   constexpr auto
   read_at_unchecked(position_type<T> const& position) const -> decltype(auto) {
      return ::cat::read_at_unchecked(*m_p_collection, position);
   }
};
}  // namespace detail
}  // namespace cat
