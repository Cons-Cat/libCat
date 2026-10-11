// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/dyn>
#include <cat/maybe>

namespace cat::detail {

// Erased signatures pass small trivially-copyable arguments in registers,
// and others by reference so that they are only moved into the map itself.
template <typename T>
inline constexpr bool dyn_map_by_value =
   is_trivially_copyable<T> && sizeof(T) <= 2u * sizeof(void*);

template <typename T>
using dyn_map_in = conditional<dyn_map_by_value<T>, T, T const&>;

template <typename T>
using dyn_map_sink = conditional<dyn_map_by_value<T>, T, T&&>;

// `cat::dyn` methods shared by `map_view` and `dyn_map`. Lookups are always
// `const`, so that `const` and mutable views share one vtable. Views which
// may mutate values only bind mutable maps, so they can `const_cast` the
// result.
template <typename Key, typename T>
struct dyn_map_lookup {
   using signature = auto(dyn_map_in<Key>) const -> maybe<T const&>;

   template <typename Map>
   [[gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   do_invoke(Map const& self, dyn_map_in<Key> key) -> maybe<T const&> {
      return self[key];
   }
};

template <typename Key>
struct dyn_map_contains {
   using signature = auto(dyn_map_in<Key>) const -> bool;

   template <typename Map>
   [[gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   do_invoke(Map const& self, dyn_map_in<Key> key) -> bool {
      return self.contains(key);
   }
};

struct dyn_map_size {
   using signature = auto() const -> idx;

   template <typename Map>
   [[gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   do_invoke(Map const& self) -> idx {
      return self.size();
   }
};

// A map and a `cat::dyn` vtable of its `Methods`. Unlike `dyn_ref`, this has
// no type id, so it is two pointers and is passed in two registers.
template <typename... Methods>
struct dyn_map_ref {
   vtable<Methods...> const* _Nonnull p_vtable;
   // This is only written through by non-`const` methods, which are only
   // bound to non-`const` maps.
   void* _Nonnull p_map;

   template <typename Map>
   [[gnu::always_inline, gnu::nodebug]]
   constexpr dyn_map_ref(Map const& map [[clang::lifetimebound]])
       : p_vtable(p_vtable_for<Map, Methods...>),
         p_map(const_cast<Map*>(__builtin_addressof(map))) {
   }

   // `Methods...` must be a contiguous subset of `Superset...`.
   template <typename... Superset>
   [[gnu::always_inline, gnu::nodebug]]
   constexpr dyn_map_ref(dyn_map_ref<Superset...> other)
       : p_vtable(narrow_vtable<Methods...>(other.p_vtable)),
         p_map(other.p_map) {
   }

   template <typename Method, typename... Args>
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   invoke(Args&&... arguments) const {
      return p_vtable->template fun<Method>()(p_map, $fwd(arguments)...);
   }
};

// Give mutable access to a value found by a `const` lookup.
template <typename T>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
dyn_map_unconst(maybe<remove_const<T> const&> found) -> maybe<T&> {
   if constexpr (is_const<T>) {
      return found;
   } else {
      if (!found.has_value()) {
         return nullopt;
      }
      return const_cast<T&>(found.value());
   }
}

}  // namespace cat::detail

namespace cat::inline manual {

template <typename Key, typename T>
class dyn_map;

template <
   typename Key, typename T, typename Hash, typename KeyEqual,
   typename Container>
class map_ref;

// A non-owning, type-erased reference to any non-`const` map of `Key` to `T`.
// This erases its hash, key equality, storage, and `map_flags`. It exposes no
// insertion or other mutating methods.
template <typename Key, typename T>
class [[gsl::Pointer]] map_view {
   template <typename, typename>
   friend class map_view;

   using value_type = remove_const<T>;

   detail::dyn_map_ref<
      detail::dyn_map_lookup<Key, value_type>, detail::dyn_map_contains<Key>,
      detail::dyn_map_size>
      m_map;

 public:
   template <typename Map>
      requires(
         (is_const<T> || !is_const<Map>)
         && is_neither<
            remove_const<Map>, map_view<Key, value_type>,
            map_view<Key, value_type const>, dyn_map<Key, value_type>>
         // A `map_ref` binds the map that it references.
         && !is_specialization<remove_const<Map>, map_ref>
      )
   [[gnu::always_inline, gnu::nodebug]] constexpr map_view(
      Map& map [[clang::lifetimebound]]
   )
       : m_map(map) {
   }

   // These narrow the vtable rather than viewing the `dyn_map` or
   // `map_view` itself.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr map_view(dyn_map<Key, value_type> map)
       : m_map(map.m_map) {
   }

   template <typename U>
      requires(is_const<T> && is_same<U, value_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr map_view(map_view<Key, U> view)
       : m_map(view.m_map) {
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator[](Key const& key) const -> maybe<T&> {
      return detail::dyn_map_unconst<T>(
         m_map.template invoke<detail::dyn_map_lookup<Key, value_type>>(key)
      );
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
