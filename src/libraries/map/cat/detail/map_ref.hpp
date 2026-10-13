// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator_parameters>
#include <cat/hash>
#include <cat/math>
#include <cat/maybe>
#include <cat/vec>

#include "./dyn_map.hpp"

namespace cat::detail {

// These are defined in `<cat/map>`.
template <typename Key, typename T>
struct map_group;

template <
   typename Key, typename T, typename Hash, typename KeyEqual,
   typename Container>
class map_base;

}  // namespace cat::detail

namespace cat::inline manual {

// A non-owning, type-erased reference to any non-`const` map of `Key` to `T`.
// This erases its hash, key equality, storage, and `map_flags`.
template <
   typename Key, typename T, typename Hash = __typeof_unqual(hash),
   typename KeyEqual = detail::binary_comparator<detail::equal_to_op>,
   typename Container = vec_big<detail::map_group<Key, T>>>
class [[gsl::Pointer]] map_ref {
   using map_type = detail::map_base<Key, T, Hash, KeyEqual, Container>;

   map_type* _Nonnull m_p_map;

 public:
   constexpr map_ref(map_type& map [[clang::lifetimebound]])
       : m_p_map(__builtin_addressof(map)) {
   }

   // These bind the referenced map rather than this `map_ref`.
   [[nodiscard]]
   constexpr
   operator dyn_map<Key, T>() const {
      return *m_p_map;
   }

   [[nodiscard]]
   constexpr
   operator map_view<Key, T>() const {
      return *m_p_map;
   }

   [[nodiscard]]
   constexpr
   operator map_view<Key, T const>() const {
      return *m_p_map;
   }

   [[nodiscard]]
   constexpr auto
   operator[](Key const& key) const -> maybe<T&> {
      return (*m_p_map)[key];
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   insert(allocator_ref<Allocator> allocator, Key key, T value) const
      -> maybe<void> {
      return m_p_map->insert(allocator, move(key), move(value));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   insert(allocator_dyn allocator, Key key, T value) const -> maybe<void> {
      return insert<allocator_dyn>(allocator, move(key), move(value));
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   update(allocator_ref<Allocator> allocator, Key key, T value) const
      -> maybe<void> {
      return m_p_map->update(allocator, move(key), move(value));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   update(allocator_dyn allocator, Key key, T value) const -> maybe<void> {
      return update<allocator_dyn>(allocator, move(key), move(value));
   }

   constexpr auto
   remove(Key const& key) const -> maybe<void> {
      return m_p_map->remove(key);
   }

   [[nodiscard]]
   constexpr auto
   contains(Key const& key) const -> bool {
      return m_p_map->contains(key);
   }

   [[nodiscard]]
   constexpr auto
   size() const -> idx {
      return m_p_map->size();
   }

   [[nodiscard]]
   constexpr auto
   capacity() const -> idx {
      return m_p_map->capacity();
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   rehash(allocator_ref<Allocator> allocator, idx slots) const -> maybe<void> {
      return m_p_map->rehash(allocator, slots);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   rehash(allocator_dyn allocator, idx slots) const -> maybe<void> {
      return rehash<allocator_dyn>(allocator, slots);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   reserve(allocator_ref<Allocator> allocator, idx count) const -> maybe<void> {
      return m_p_map->reserve(allocator, count);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   reserve(allocator_dyn allocator, idx count) const -> maybe<void> {
      return reserve<allocator_dyn>(allocator, count);
   }

   // Deallocate the referenced map's storage from `allocator`.
   template <is_allocator Allocator>
   constexpr void
   free(allocator_ref<Allocator> allocator) const {
      m_p_map->free(allocator);
   }

   // Deallocate the referenced map's storage from `allocator`.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr void
   free(allocator_dyn allocator) const {
      free<allocator_dyn>(allocator);
   }

   // Deallocate the referenced map's storage from `allocator`.
   template <is_allocator Allocator>
   constexpr void
   cfree(allocator_ref<Allocator> allocator) const {
      m_p_map->cfree(allocator);
   }

   // Deallocate the referenced map's storage from `allocator`.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr void
   cfree(allocator_dyn allocator) const {
      cfree<allocator_dyn>(allocator);
   }
};

}  // namespace cat::inline manual
