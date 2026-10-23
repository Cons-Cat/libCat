#include <cat/allocator_linear>
#include <cat/allocator_page>
#include <cat/map>

#include "../unit_tests.hpp"

namespace {

int4 destructor_count = 0;

struct tracked {
   int4 value;
   bool live = true;

   tracked(int4 in_value) : value(in_value) {
   }

   tracked(tracked&& other) : value(other.value) {
      other.live = false;
   }

   // `.update()` requires this.
   auto
   operator=(tracked&& other) -> tracked& {
      value = other.value;
      other.live = false;
      return *this;
   }

   ~tracked() {
      if (live) {
         ++destructor_count;
      }
   }
};

// Every key collides, which exercises probing and overflow bits.
struct constant_hash {
   static constexpr auto
   operator()(int4) -> uint8 {
      return 0u;
   }
};

// Keys are equal when they are equal modulo 100.
struct modulo_hash {
   static constexpr auto
   operator()(int4 key) -> uint8 {
      return cat::hash(key % 100);
   }
};

struct modulo_equal {
   static constexpr auto
   operator()(int4 left, int4 right) -> bool {
      return left % 100 == right % 100;
   }
};

struct seeded_hash {
   uint8 seed;

   constexpr auto
   operator()(int4 key) const -> uint8 {
      return cat::hash(key, seed);
   }
};

consteval auto
map_consteval_insert_lookup() -> bool {
   cat::map_big<int4, int4> map;
   for (int4 i = 0; i < 100; ++i) {
      if (!map.insert<cat::allocator_page>(pager, i, i * 2).has_value()) {
         return false;
      }
   }
   bool const found = map[42].value() == 84 && !map[100].has_value();
   map.free<cat::allocator_page>(pager);
   return found;
}

}  // namespace

$test(map_insert_lookup) {
   cat::map_big<int4, int4> map;
   cat::verify(!map[0].has_value());
   cat::verify(!map.contains(0));

   // Enough entries to grow several times.
   constexpr int4 count = 5'000;
   for (int4 i = 0; i < count; ++i) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, i * 3).has_value());
   }
   cat::verify(map.size() == count);

   for (int4 i = 0; i < count; ++i) {
      cat::verify(map[i].value() == i * 3);
      cat::verify(map.contains(i));
   }
   cat::verify(!map[count].has_value());
   cat::verify(!map[-1].has_value());

   // A key that is already present is not inserted again.
   cat::verify(!map.insert<cat::allocator_page>(pager, 7, 0).has_value());
   cat::verify(map[7].value() == 21);
   cat::verify(map.size() == count);

   // Lookups give a mutable reference.
   map[7].value() = 100;
   cat::verify(map[7].value() == 100);

   auto const& const_map = map;
   cat::verify(const_map[7].value() == 100);

   map.free<cat::allocator_page>(pager);
   cat::verify(map.size() == 0);
   cat::verify(!map[7].has_value());
}

$test(map_nontrivial_values) {
   destructor_count = 0;
   {
      cat::map_big<int4, tracked> map;
      for (int4 i = 0; i < 1'000; ++i) {
         cat::verify(
            map.insert<cat::allocator_page>(pager, i, tracked(i)).has_value()
         );
      }
      // Rehashing moves entries, which must not destroy their values.
      cat::verify(destructor_count == 0);
      cat::verify(map[500].value().value == 500);
      map.free<cat::allocator_page>(pager);
   }
   cat::verify(destructor_count == 1'000);
}

$test(map_hash_and_key_equal) {
   // Groups, their size index and mask, the size, and the maximum load. The
   // default hash and equality take no space.
   static_assert(sizeof(cat::map_big<int4, int4>) == 56);

   cat::map_big<int4, int4, constant_hash> colliding;
   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(
         colliding.insert<cat::allocator_page>(pager, i, i + 1).has_value()
      );
   }
   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(colliding[i].value() == i + 1);
   }
   cat::verify(!colliding[1'000].has_value());
   colliding.free<cat::allocator_page>(pager);

   cat::map_big<int4, int4, modulo_hash, modulo_equal> modulo;
   cat::verify(modulo.insert<cat::allocator_page>(pager, 5, 1).has_value());
   cat::verify(!modulo.insert<cat::allocator_page>(pager, 105, 2).has_value());
   cat::verify(modulo[205].value() == 1);
   cat::verify(!modulo[6].has_value());
   modulo.free<cat::allocator_page>(pager);

   cat::map_big<int4, int4, seeded_hash> seeded(seeded_hash{12'345u});
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(seeded.insert<cat::allocator_page>(pager, i, -i).has_value());
   }
   cat::verify(seeded[99].value() == -99);
   seeded.free<cat::allocator_page>(pager);
}

$test(map_growth_matches_boost) {
   cat::map_big<int4, int4> map;
   cat::verify(map.capacity() == 0u);
   auto insert_until = [&](int4 count) {
      for (int4 i = int4(map.size()); i < count; ++i) {
         cat::verify(map.insert<cat::allocator_page>(pager, i, i).has_value());
      }
   };

   // Tables of up to 2 groups may be completely full.
   insert_until(1);
   cat::verify(map.capacity() == 29u);
   insert_until(29);
   cat::verify(map.capacity() == 29u);
   insert_until(30);
   cat::verify(map.capacity() == 59u);

   // Larger tables are loaded to at most 0.875.
   insert_until(51);
   cat::verify(map.capacity() == 59u);
   insert_until(52);
   cat::verify(map.capacity() == 119u);
   map.free<cat::allocator_page>(pager);
}

$test(map_remove) {
   cat::map_big<int4, int4> map;
   cat::verify(!map.remove(0).has_value());
   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, i * 2).has_value());
   }
   for (int4 i = 0; i < 1'000; i += 2) {
      cat::verify(map.remove(i).has_value());
   }
   cat::verify(map.size() == 500u);
   cat::verify(!map.remove(0).has_value());
   cat::verify(!map.remove(1'000).has_value());
   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(map.contains(i) == (i % 2 == 1));
   }
   cat::verify(map[999].value() == 1'998);

   // Removed keys can be inserted again.
   for (int4 i = 0; i < 1'000; i += 2) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, -i).has_value());
   }
   cat::verify(map.size() == 1'000u);
   cat::verify(map[500].value() == -500);
   map.free<cat::allocator_page>(pager);

   destructor_count = 0;
   cat::map_big<int4, tracked> tracked_map;
   for (int4 i = 0; i < 10; ++i) {
      cat::verify(tracked_map.insert<cat::allocator_page>(pager, i, tracked(i))
                     .has_value());
   }
   cat::verify(tracked_map.remove(3).has_value());
   cat::verify(destructor_count == 1);
   tracked_map.free<cat::allocator_page>(pager);
   cat::verify(destructor_count == 10);
}

$test(map_remove_colliding) {
   // Every key collides, so entries sit past emptied slots, and every removal
   // lowers the maximum load.
   cat::map_big<int4, int4, constant_hash> map;
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, i).has_value());
   }
   for (int4 i = 0; i < 100; i += 3) {
      cat::verify(map.remove(i).has_value());
   }
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(map.contains(i) == (i % 3 != 0));
   }

   // Repeatedly inserting and removing must keep working as anti-drift
   // lowers the maximum load and eventually forces a rehash.
   for (int4 i = 100; i < 1'100; ++i) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, i).has_value());
      cat::verify(map.remove(i).has_value());
      cat::verify(!map.contains(i));
   }
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(map.contains(i) == (i % 3 != 0));
   }
   map.free<cat::allocator_page>(pager);
}

$test(map_update) {
   cat::map_big<int4, int4> map;
   // A missing key is inserted.
   cat::verify(map.update<cat::allocator_page>(pager, 1, 10).has_value());
   cat::verify(map[1].value() == 10);
   // A present key has its value replaced.
   cat::verify(map.update<cat::allocator_page>(pager, 1, 20).has_value());
   cat::verify(map[1].value() == 20);
   cat::verify(map.size() == 1u);

   // Updates grow the map like insertions.
   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(map.update<cat::allocator_page>(pager, i, i).has_value());
   }
   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(map.update<cat::allocator_page>(pager, i, i + 1).has_value());
   }
   cat::verify(map.size() == 1'000u);
   cat::verify(map[999].value() == 1'000);
   map.cfree<cat::allocator_page>(pager);
   cat::verify(map.size() == 0u);
   cat::verify(map.capacity() == 0u);
   cat::verify(!map[1].has_value());
}

$test(map_reserve_rehash) {
   cat::map_big<int4, int4> map;
   // `reserve()` makes room without growing later.
   cat::verify(map.reserve<cat::allocator_page>(pager, 100u).has_value());
   cat::verify(map.capacity() == 119u);
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, i).has_value());
   }
   cat::verify(map.capacity() == 119u);
   map.free<cat::allocator_page>(pager);

   for (int4 i = 0; i < 10; ++i) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, i * 7).has_value());
   }
   cat::verify(map.reserve<cat::allocator_page>(pager, 1'000u).has_value());
   cat::verify(map.capacity() == 1'919u);

   // `rehash()` shrinks to the smallest capacity that holds every entry.
   cat::verify(map.rehash<cat::allocator_page>(pager, 0u).has_value());
   cat::verify(map.capacity() == 29u);
   cat::verify(map.size() == 10u);
   for (int4 i = 0; i < 10; ++i) {
      cat::verify(map[i].value() == i * 7);
   }

   // An empty map's storage is freed.
   for (int4 i = 0; i < 10; ++i) {
      cat::verify(map.remove(i).has_value());
   }
   cat::verify(map.rehash<cat::allocator_page>(pager, 0u).has_value());
   cat::verify(map.capacity() == 0u);
   cat::verify(!map[0].has_value());
   cat::verify(map.insert<cat::allocator_page>(pager, 1, 1).has_value());
   cat::verify(map.capacity() == 29u);
   map.free<cat::allocator_page>(pager);
}

$test(map_consteval) {
   static_assert(map_consteval_insert_lookup());
}

namespace {

auto
dyn_map_insert(cat::dyn_map<int4, int4> map, int4 key) -> bool {
   return map.insert(pager, key, key * 2).has_value();
}

auto
map_view_sum(cat::map_view<int4, int4 const> view, int4 count) -> int4 {
   int4 sum = 0;
   for (int4 i = 0; i < count; ++i) {
      sum += view[i].value_or(0);
   }
   return sum;
}

}  // namespace

namespace {

template <typename Map>
concept can_change_keys = requires(Map map) {
                             map.insert(pager, 5, 5);
                             map.update(pager, 5, 5);
                             map.remove(5);
                          };

// Drive every `dyn_map` operation on a `Map` through type erasure, and check
// that the erased map agrees with the concrete one.
template <typename Map>
void
verify_dyn_map() {
   Map map;
   cat::dyn_map<int4, int4> dyn = map;
   cat::verify(dyn.size() == 0u);
   cat::verify(!dyn.contains(0));
   cat::verify(!dyn[0].has_value());
   cat::verify(!dyn.remove(0).has_value());

   // Grow past every inline capacity under test, several times over. The
   // earliest entry must survive each reallocation.
   constexpr int4 count = 300;
   for (int4 i = 0; i < count; ++i) {
      cat::verify(dyn.insert(pager, i, i * 2).has_value());
      cat::verify(dyn.size() == i + 1);
      cat::verify(dyn[i].value() == i * 2);
      cat::verify(dyn[0].value() == 0);
   }
   cat::verify(map.size() == dyn.size());

   // Duplicates are rejected and keep their value.
   cat::verify(!dyn.insert(pager, 7, -1).has_value());
   cat::verify(dyn[7].value() == 14);

   // `update()` replaces present keys and inserts missing ones.
   cat::verify(dyn.update(pager, 7, -7).has_value());
   cat::verify(dyn.update(pager, count, -count).has_value());
   cat::verify(dyn.size() == count + 1);
   cat::verify(map[7].value() == -7);
   cat::verify(map[count].value() == -count);

   // Lookups give a mutable reference into the concrete map.
   dyn[8].value() = 80;
   cat::verify(map[8].value() == 80);

   // Writes through the concrete map are seen through the `dyn_map`.
   cat::verify(
      map.template insert<cat::allocator_page>(pager, -1, 1).has_value()
   );
   cat::verify(dyn.contains(-1));
   cat::verify(dyn.remove(-1).has_value());

   // Remove every even key, then shrink back down to only odd keys.
   for (int4 i = 0; i <= count; i += 2) {
      cat::verify(dyn.remove(i).has_value());
      cat::verify(!dyn.remove(i).has_value());
   }
   cat::verify(dyn.size() == count / 2);
   for (int4 i = 0; i <= count; ++i) {
      cat::verify(dyn.contains(i) == (i % 2 == 1));
      cat::verify(map.contains(i) == dyn.contains(i));
   }

   // Removed keys can be inserted again.
   for (int4 i = 0; i <= count; i += 2) {
      cat::verify(dyn.insert(pager, i, i).has_value());
   }
   cat::verify(dyn.size() == count + 1);
   cat::verify(dyn[count].value() == count);

   map.template free<cat::allocator_page>(pager);
   cat::verify(dyn.size() == 0u);
   cat::verify(!dyn.contains(1));
}

// A minimal map that is not a `basic_map`, to show that `dyn_map` erases the
// kind of map and not only its configuration.
struct assoc_array {
   cat::array<int4, 8u> keys;
   cat::array<int4, 8u> values;
   idx count = 0u;

   auto
   find(int4 key) const -> cat::maybe<idx> {
      for (idx i = 0u; i < count; ++i) {
         if (keys[i] == key) {
            return i;
         }
      }
      return cat::nullopt;
   }

   auto
   operator[](int4 key) -> cat::maybe<int4&> {
      cat::maybe<idx> const i = find(key);
      if (!i.has_value()) {
         return cat::nullopt;
      }
      return values[i.value()];
   }

   auto
   operator[](int4 key) const -> cat::maybe<int4 const&> {
      cat::maybe<idx> const i = find(key);
      if (!i.has_value()) {
         return cat::nullopt;
      }
      return values[i.value()];
   }

   auto
   insert(cat::allocator_dyn, int4 key, int4 value) -> cat::maybe<void> {
      if (find(key).has_value() || count == 8u) {
         return cat::nullopt;
      }
      keys[count] = key;
      values[count] = value;
      ++count;
      return cat::monostate;
   }

   auto
   update(cat::allocator_dyn allocator, int4 key, int4 value)
      -> cat::maybe<void> {
      if (cat::maybe<int4&> found = (*this)[key]; found.has_value()) {
         found.value() = value;
         return cat::monostate;
      }
      return insert(allocator, key, value);
   }

   auto
   remove(int4 key) -> cat::maybe<void> {
      cat::maybe<idx> const i = find(key);
      if (!i.has_value()) {
         return cat::nullopt;
      }
      count = idx(count - 1u);
      keys[i.value()] = keys[count];
      values[i.value()] = values[count];
      return cat::monostate;
   }

   auto
   contains(int4 key) const -> bool {
      return find(key).has_value();
   }

   auto
   size() const -> idx {
      return count;
   }
};

}  // namespace

$test(dyn_map_and_view) {
   // Maps with different `map_flags` bind to the same `dyn_map`.
   cat::map<int4, int4> small;
   cat::map_big<int4, int4> big;
   static_assert(!cat::is_same<decltype(small), decltype(big)>);
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(dyn_map_insert(small, i));
      cat::verify(dyn_map_insert(big, i));
   }
   cat::verify(!dyn_map_insert(small, 0));
   cat::verify(small[99].value() == 198);
   cat::verify(map_view_sum(small, 100) == map_view_sum(big, 100));
   big.free<cat::allocator_page>(pager);
   small.free<cat::allocator_page>(pager);
}

$test(map_view) {
   cat::map_big<int4, int4> map;
   cat::map_view<int4, int4> view = map;
   cat::verify(view.size() == 0u);
   cat::verify(!view.contains(0));
   cat::verify(!view[0].has_value());

   // A view sees insertions made after it was created, including growth.
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(map.insert<cat::allocator_page>(pager, i, i).has_value());
   }
   cat::verify(view.size() == 100u);
   cat::verify(view.contains(5));
   cat::verify(!view.contains(100));

   // A mutable view mutates values but not keys.
   view[5].value() = -1;
   cat::verify(map[5].value() == -1);
   static_assert(!can_change_keys<cat::map_view<int4, int4>>);
   static_assert(can_change_keys<cat::dyn_map<int4, int4>>);

   // A `const` map only gives a `const` view.
   auto const& const_map = map;
   cat::map_view<int4, int4 const> const_view = const_map;
   cat::verify(const_view[5].value() == -1);
   static_assert(
      cat::is_same<decltype(const_view[5]), cat::maybe<int4 const&>>
   );
   static_assert(
      !cat::is_convertible<decltype(const_map), cat::map_view<int4, int4>>
   );

   // A mutable map also gives a `const` view.
   cat::map_view<int4, int4 const> also_const = map;
   cat::verify(also_const[6].value() == 6);

   map.free<cat::allocator_page>(pager);
   cat::verify(view.size() == 0u);
   cat::verify(!const_view.contains(5));
}

$test(map_view_configurations) {
   // Every kind and configuration of map is viewed through one type.
   cat::map<int4, int4> small;
   cat::map_small<int4, int4, 1u> tiny;
   cat::map_big<int4, int4, constant_hash> colliding;
   cat::map<int4, int4, modulo_hash, modulo_equal> modulo;
   assoc_array array;
   for (int4 i = 0; i < 8; ++i) {
      cat::verify(small.insert<cat::allocator_page>(pager, i, i).has_value());
      cat::verify(tiny.insert<cat::allocator_page>(pager, i, i).has_value());
      cat::verify(
         colliding.insert<cat::allocator_page>(pager, i, i).has_value()
      );
      cat::verify(modulo.insert<cat::allocator_page>(pager, i, i).has_value());
      cat::verify(array.insert(pager, i, i).has_value());
   }
   for (cat::map_view<int4, int4 const> view :
        {cat::map_view<int4, int4 const>(small),
         cat::map_view<int4, int4 const>(tiny),
         cat::map_view<int4, int4 const>(colliding),
         cat::map_view<int4, int4 const>(modulo),
         cat::map_view<int4, int4 const>(array)}) {
      cat::verify(view.size() == 8u);
      cat::verify(map_view_sum(view, 8) == 28);
      cat::verify(!view.contains(8));
   }
   // Key equality is respected through the view.
   cat::verify(cat::map_view<int4, int4 const>(modulo)[107].value() == 7);

   small.free<cat::allocator_page>(pager);
   tiny.free<cat::allocator_page>(pager);
   colliding.free<cat::allocator_page>(pager);
   modulo.free<cat::allocator_page>(pager);
}

$test(map_view_from_dyn_map) {
   cat::map<int4, int4> map;
   cat::dyn_map<int4, int4> dyn = map;
   cat::verify(dyn.insert(pager, 1, 10).has_value());

   // A `dyn_map` narrows into a `map_view` of the same map, not of itself.
   // Viewing a temporary `dyn_map` is only sound because of this.
   cat::map_view<int4, int4> view = cat::dyn_map<int4, int4>(map);
   cat::verify(view.size() == 1u);
   view[1].value() = 11;
   cat::verify(map[1].value() == 11);

   // `const` views narrow from `dyn_map`s and mutable views alike.
   cat::map_view<int4, int4 const> const_view = cat::dyn_map<int4, int4>(map);
   cat::verify(const_view[1].value() == 11);
   cat::verify(const_view.size() == 1u);
   cat::map_view<int4, int4 const> from_view = view;
   cat::verify(from_view[1].value() == 11);
   static_assert(!cat::is_convertible<
                 cat::map_view<int4, int4 const>, cat::map_view<int4, int4>>);

   // Views are two pointers, like `dyn_map`.
   static_assert(sizeof(cat::map_view<int4, int4>) == 2u * sizeof(void*));
   static_assert(sizeof(cat::map_view<int4, int4 const>) == 2u * sizeof(void*));
   static_assert(cat::is_trivially_copyable<cat::map_view<int4, int4>>);
   map.free<cat::allocator_page>(pager);
}

$test(dyn_map_configurations) {
   // `inline_storage` is not implemented yet, so these only differ by type.
   // `dyn_map` must still treat all of them alike.
   static_assert(cat::map<int4, int4>::flags.inline_storage_count == 4u);
   static_assert(
      cat::map_small<int4, int4, 64u>::flags.inline_storage_count == 64u
   );
   static_assert(!cat::is_same<cat::map<int4, int4>, cat::map_big<int4, int4>>);

   verify_dyn_map<cat::map<int4, int4>>();
   verify_dyn_map<cat::map_small<int4, int4, 1u>>();
   verify_dyn_map<cat::map_small<int4, int4, 64u>>();
   verify_dyn_map<cat::map_big<int4, int4>>();
   verify_dyn_map<cat::map_big<int4, int4, constant_hash>>();
   verify_dyn_map<cat::map<int4, int4, seeded_hash>>();
}

$test(dyn_map_key_equal) {
   cat::map<int4, int4, modulo_hash, modulo_equal> map;
   cat::dyn_map<int4, int4> dyn = map;
   cat::verify(dyn.insert(pager, 5, 1).has_value());
   cat::verify(!dyn.insert(pager, 105, 2).has_value());
   cat::verify(dyn[205].value() == 1);
   cat::verify(dyn.update(pager, 305, 3).has_value());
   cat::verify(dyn.size() == 1u);
   cat::verify(map[5].value() == 3);
   cat::verify(dyn.remove(405).has_value());
   cat::verify(!dyn.contains(5));
   map.free<cat::allocator_page>(pager);
}

$test(dyn_map_nontrivial_values) {
   destructor_count = 0;
   cat::map_small<int4, tracked, 2u> map;
   cat::dyn_map<int4, tracked> dyn = map;
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(dyn.insert(pager, i, tracked(i)).has_value());
   }
   // Moving values through the vtable and growing destroy nothing.
   cat::verify(destructor_count == 0);
   cat::verify(dyn[50].value().value == 50);

   // A rejected insertion destroys only its argument.
   cat::verify(!dyn.insert(pager, 0, tracked(-1)).has_value());
   cat::verify(destructor_count == 1);

   cat::verify(dyn.update(pager, 0, tracked(-1)).has_value());
   cat::verify(dyn[0].value().value == -1);
   // Assigning the new value destroys nothing.
   cat::verify(destructor_count == 1);
   cat::verify(dyn.remove(1).has_value());
   cat::verify(destructor_count == 2);

   map.free<cat::allocator_page>(pager);
   cat::verify(destructor_count == 101);
}

$test(dyn_map_allocation_failure) {
   cat::span<cat::byte> page = pager.alloc_multi<cat::byte>(4_uki).verify();
   cat::allocator_linear arena = cat::make_allocator_linear(page);

   cat::map<int4, int4> map;
   cat::dyn_map<int4, int4> dyn = map;
   int4 inserted = 0;
   while (dyn.insert(arena, inserted, inserted).has_value()) {
      ++inserted;
      cat::verify(inserted < 10'000);
   }

   // A failed insertion leaves the map unchanged.
   cat::verify(inserted > 0);
   cat::verify(dyn.size() == inserted);
   cat::verify(!dyn.contains(inserted));
   for (int4 i = 0; i < inserted; ++i) {
      cat::verify(dyn[i].value() == i);
   }

   // Updating present keys never allocates, so it still succeeds.
   cat::verify(dyn.update(arena, 0, -1).has_value());
   cat::verify(dyn[0].value() == -1);
   cat::verify(!dyn.update(arena, inserted, 0).has_value());
   cat::verify(dyn.size() == inserted);

   pager.free(page);
}

$test(dyn_map_handle) {
   // A `dyn_map` is two pointers, the map and its vtable, so it is passed in
   // registers.
   static_assert(sizeof(cat::dyn_map<int4, int4>) == 2u * sizeof(void*));
   static_assert(cat::is_trivially_copyable<cat::dyn_map<int4, int4>>);

   // Only mutable lvalue maps of the same `Key` and `T` bind.
   using map_type = cat::map<int4, int4>;
   static_assert(cat::is_convertible<map_type&, cat::dyn_map<int4, int4>>);
   static_assert(
      !cat::is_convertible<map_type const&, cat::dyn_map<int4, int4>>
   );
   static_assert(!cat::is_convertible<map_type&&, cat::dyn_map<int4, int4>>);

   map_type first;
   map_type second;
   cat::dyn_map<int4, int4> dyn = first;

   // Copies refer to the same map.
   cat::dyn_map<int4, int4> copy = dyn;
   cat::verify(copy.insert(pager, 1, 1).has_value());
   cat::verify(dyn.contains(1));
   cat::verify(first.contains(1));

   // Assignment rebinds to another map.
   dyn = cat::dyn_map<int4, int4>(second);
   cat::verify(!dyn.contains(1));
   cat::verify(dyn.insert(pager, 2, 2).has_value());
   cat::verify(!first.contains(2));
   cat::verify(second.contains(2));
   cat::verify(copy.size() == 1u);

   first.free<cat::allocator_page>(pager);
   second.free<cat::allocator_page>(pager);
}

$test(dyn_map_other_map_kinds) {
   assoc_array array;
   cat::map_big<int4, int4> hash;
   for (cat::dyn_map<int4, int4> dyn :
        {cat::dyn_map<int4, int4>(array), cat::dyn_map<int4, int4>(hash)}) {
      for (int4 i = 0; i < 8; ++i) {
         cat::verify(dyn.insert(pager, i, i * i).has_value());
      }
      cat::verify(dyn.update(pager, 3, -3).has_value());
      cat::verify(dyn.remove(4).has_value());
      cat::verify(dyn.size() == 7u);
      cat::verify(dyn[3].value() == -3);
      cat::verify(dyn[7].value() == 49);
      cat::verify(!dyn.contains(4));
   }
   // The array is full, so it rejects a new key, unlike the hash map.
   cat::verify(cat::dyn_map<int4, int4>(array).insert(pager, 4, 0).has_value());
   cat::verify(
      !cat::dyn_map<int4, int4>(array).insert(pager, 9, 0).has_value()
   );
   cat::verify(cat::dyn_map<int4, int4>(hash).insert(pager, 9, 0).has_value());
   hash.free<cat::allocator_page>(pager);
}

namespace {

// A value that can be moved, but not move-assigned.
struct unassignable {
   int4 value;

   unassignable(int4 in_value) : value(in_value) {
   }

   unassignable(unassignable&&) = default;
   auto
   operator=(unassignable&&) -> unassignable& = delete;
};

template <typename Map>
concept can_update = requires(Map map) { map.update(pager, 0, 0); };

}  // namespace

$test(dyn_map_unassignable_values) {
   // Binding a map does not require that values are move-assignable, only
   // calling `.update()` does.
   cat::map<int4, unassignable> map;
   cat::dyn_map<int4, unassignable> dyn = map;
   static_assert(!can_update<cat::dyn_map<int4, unassignable>>);
   static_assert(can_update<cat::dyn_map<int4, int4>>);
   for (int4 i = 0; i < 50; ++i) {
      cat::verify(dyn.insert(pager, i, unassignable(i)).has_value());
   }
   cat::verify(dyn[49].value().value == 49);
   cat::verify(dyn.remove(49).has_value());
   cat::verify(dyn.size() == 49u);
   cat::map_view<int4, unassignable const> view = dyn;
   cat::verify(view[48].value().value == 48);
   map.free<cat::allocator_page>(pager);
}

namespace {

// Insert through a `map_ref` with static allocator dispatch.
auto
map_ref_fill(cat::map_ref<int4, int4> map, int4 count) -> bool {
   for (int4 i = 0; i < count; ++i) {
      if (!map.insert<cat::allocator_page>(pager, i, i * 3).has_value()) {
         return false;
      }
   }
   return true;
}

auto
map_ref_view(cat::map_ref<int4, int4> map) -> cat::map_view<int4, int4> {
   return map;
}

consteval auto
map_ref_consteval() -> bool {
   cat::map<int4, int4> map;
   cat::map_ref<int4, int4> ref = map;
   for (int4 i = 0; i < 50; ++i) {
      if (!ref.insert<cat::allocator_page>(pager, i, i).has_value()) {
         return false;
      }
   }
   bool const found = ref[49].value() == 49 && map.size() == 50u;
   ref.free<cat::allocator_page>(pager);
   return found;
}

}  // namespace

$test(map_ref) {
   // Maps of every `map_flags` bind to the same `map_ref`.
   cat::map<int4, int4> small;
   cat::map_small<int4, int4, 1u> tiny;
   cat::map_big<int4, int4> big;
   cat::verify(map_ref_fill(small, 300));
   cat::verify(map_ref_fill(tiny, 300));
   cat::verify(map_ref_fill(big, 300));
   cat::verify(small[299].value() == 897);
   cat::verify(tiny.size() == 300u);

   // A `map_ref` is a single pointer, unlike `dyn_map`.
   static_assert(sizeof(cat::map_ref<int4, int4>) == sizeof(void*));
   static_assert(cat::is_trivially_copyable<cat::map_ref<int4, int4>>);

   // Only mutable lvalue maps with the same hash, key equality, and storage
   // bind.
   using ref_type = cat::map_ref<int4, int4>;
   static_assert(cat::is_convertible<cat::map<int4, int4>&, ref_type>);
   static_assert(!cat::is_convertible<cat::map<int4, int4> const&, ref_type>);
   static_assert(!cat::is_convertible<cat::map<int4, int4>&&, ref_type>);
   static_assert(
      !cat::is_convertible<cat::map<int4, int4, constant_hash>&, ref_type>
   );
   static_assert(cat::is_convertible<
                 cat::map<int4, int4, constant_hash>&,
                 cat::map_ref<int4, int4, constant_hash>>);

   // Every operation of `basic_map` works through it.
   cat::map_ref<int4, int4> ref = small;
   cat::verify(!ref.insert(pager, 0, 0).has_value());
   cat::verify(ref.update(pager, 0, -1).has_value());
   cat::verify(small[0].value() == -1);
   ref[1].value() = -2;
   cat::verify(small[1].value() == -2);
   cat::verify(ref.remove(2).has_value());
   cat::verify(!ref.contains(2));
   cat::verify(ref.size() == 299u);
   cat::verify(ref.reserve<cat::allocator_page>(pager, 1'000u).has_value());
   cat::verify(ref.capacity() == small.capacity());
   cat::verify(ref.capacity() >= 1'000u);
   cat::verify(ref.rehash(pager, 0u).has_value());
   cat::verify(ref.capacity() == 479u);
   cat::verify(ref[299].value() == 897);

   // Copies refer to the same map.
   cat::map_ref<int4, int4> copy = ref;
   cat::verify(copy.insert(pager, 1'000, 1).has_value());
   cat::verify(small.contains(1'000));

   ref.cfree(pager);
   cat::verify(small.size() == 0u);
   cat::verify(small.capacity() == 0u);
   cat::map_ref<int4, int4>(tiny).free<cat::allocator_page>(pager);
   cat::map_ref<int4, int4>(big).free<cat::allocator_page>(pager);
   cat::verify(tiny.size() == 0u);
}

$test(map_ref_conversions) {
   cat::map<int4, int4> map;
   cat::verify(map.insert<cat::allocator_page>(pager, 1, 1).has_value());

   // Views and `dyn_map`s bind the referenced map, so they outlive the
   // `map_ref` they were made from.
   cat::map_view<int4, int4> view = map_ref_view(map);
   cat::dyn_map<int4, int4> dyn = cat::map_ref<int4, int4>(map);
   cat::map_view<int4, int4 const> const_view = cat::map_ref<int4, int4>(map);
   view[1].value() = 2;
   cat::verify(dyn.insert(pager, 2, 2).has_value());
   cat::verify(const_view[1].value() == 2);
   cat::verify(const_view.size() == 2u);
   cat::verify(map[2].value() == 2);

   map.free<cat::allocator_page>(pager);
}

$test(map_ref_consteval) {
   static_assert(map_ref_consteval());
}
