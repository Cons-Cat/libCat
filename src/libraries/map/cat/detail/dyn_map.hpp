// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator_parameters>
#include <cat/dyn>
#include <cat/maybe>

#include "./map_view.hpp"

namespace cat::detail {

// `cat::dyn` methods for `dyn_map` beyond those of `map_view`.

template <typename Key, typename T>
struct dyn_map_insert {
   using signature = auto(allocator_dyn&, dyn_map_sink<Key>, dyn_map_sink<T>)
      -> maybe<void>;

   template <typename Map>
   [[gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   do_invoke(
      Map& self, allocator_dyn& allocator, dyn_map_sink<Key> key,
      dyn_map_sink<T> value
   ) -> maybe<void> {
      return self.insert(allocator, move(key), move(value));
   }
};

// Only `dyn_map::update()` requires that `T` is move-assignable, so binding a
// map does not.
template <typename Key, typename T>
struct dyn_map_update {
   using signature = auto(allocator_dyn&, dyn_map_sink<Key>, dyn_map_sink<T>)
      -> maybe<void>;

   template <typename Map>
   [[gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   do_invoke(
      Map& self, allocator_dyn& allocator, dyn_map_sink<Key> key,
      dyn_map_sink<T> value
   ) -> maybe<void> {
      if constexpr (is_move_assignable<T>) {
         return self.update(allocator, move(key), move(value));
      } else {
         __builtin_unreachable();
      }
   }
};

template <typename Key>
struct dyn_map_remove {
   using signature = auto(dyn_map_in<Key>) -> maybe<void>;

   template <typename Map>
   [[gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   do_invoke(Map& self, dyn_map_in<Key> key) -> maybe<void> {
      return self.remove(key);
   }
};

}  // namespace cat::detail

namespace cat::inline manual {

// A non-owning, type-erased reference to any non-`const` map of `Key` to `T`.
// This erases its hash, key equality, storage, and `map_flags`. Insertions
// allocate through a `allocator_dyn`.
template <typename Key, typename T>
class [[gsl::Pointer]] dyn_map {
   template <typename, typename>
   friend class map_view;

   // `map_view`'s methods come first so that converting to it is free.
   detail::dyn_map_ref<
      detail::dyn_map_lookup<Key, T>, detail::dyn_map_contains<Key>,
      detail::dyn_map_size, detail::dyn_map_insert<Key, T>,
      detail::dyn_map_update<Key, T>, detail::dyn_map_remove<Key>>
      m_map;

 public:
   template <typename Map>
      requires(
         !is_const<Map>
         && is_neither<Map, dyn_map, map_view<Key, T>, map_view<Key, T const>>
         // A `map_ref` binds the map that it references.
         && !is_specialization<Map, map_ref>
      )
   [[gnu::always_inline, gnu::nodebug]] constexpr dyn_map(
      Map& map [[clang::lifetimebound]]
   )
       : m_map(map) {
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator[](Key const& key) const -> maybe<T&> {
      return detail::dyn_map_unconst<T>(
         m_map.template invoke<detail::dyn_map_lookup<Key, T>>(key)
      );
   }

   template <typename... Arguments>
      requires(is_constructible<T, Arguments...>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   insert(
      allocator_dyn allocator, Key const& key, Arguments&&... arguments
   ) const -> maybe<void> {
      // The erased call takes a constructed value, so unlike `cat::map`, `T` is
      // constructed even if `key` is already present. TODO: Should we change
      // this?
      return m_map.template invoke<detail::dyn_map_insert<Key, T>>(
         allocator, Key(key), T($fwd(arguments)...)
      );
   }

   template <typename... Arguments>
      requires(is_constructible<T, Arguments...>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   insert(allocator_dyn allocator, Key&& key, Arguments&&... arguments) const
      -> maybe<void> {
      return m_map.template invoke<detail::dyn_map_insert<Key, T>>(
         allocator, move(key), T($fwd(arguments)...)
      );
   }

   template <typename Value>
      requires(is_move_assignable<T> && is_constructible<T, Value>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   update(allocator_dyn allocator, Key const& key, Value&& value) const
      -> maybe<void> {
      return m_map.template invoke<detail::dyn_map_update<Key, T>>(
         allocator, Key(key), T($fwd(value))
      );
   }

   template <typename Value>
      requires(is_move_assignable<T> && is_constructible<T, Value>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   update(allocator_dyn allocator, Key&& key, Value&& value) const
      -> maybe<void> {
      return m_map.template invoke<detail::dyn_map_update<Key, T>>(
         allocator, move(key), T($fwd(value))
      );
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   remove(Key const& key) const -> maybe<void> {
      return m_map.template invoke<detail::dyn_map_remove<Key>>(key);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   contains(Key const& key) const -> bool {
      return m_map.template invoke<detail::dyn_map_contains<Key>>(key);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   size() const -> idx {
      return m_map.template invoke<detail::dyn_map_size>();
   }
};

}  // namespace cat::inline manual
