#include <cat/allocator_null>
#include <cat/allocator_page>
#include <cat/raii_set>
#include <cat/set>

#include "../allocator_failing.hpp"
#include "../unit_tests.hpp"

namespace {

consteval auto
set_consteval() -> bool {
   cat::set_big<int4> set;
   for (int4 i = 0; i < 100; ++i) {
      if (!set.insert<cat::allocator_page>(pager, i).has_value()) {
         return false;
      }
   }
   bool const found = set.contains(42) && !set.contains(100);
   set.free<cat::allocator_page>(pager);
   return found;
}

}  // namespace

$test(set) {
   // Values take no space in a slot.
   static_assert(sizeof(cat::tuple<int4, cat::monostate_type>) == sizeof(int4));
   static_assert(
      sizeof(cat::set_big<int4>) == sizeof(cat::map_big<int4, int4>)
   );

   cat::set<int4> set;
   cat::verify(!set.contains(0));
   cat::verify(!set.remove(0).has_value());

   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(set.insert<cat::allocator_page>(pager, i).has_value());
   }
   cat::verify(set.size() == 1'000u);
   cat::verify(!set.insert<cat::allocator_page>(pager, 7).has_value());
   cat::verify(set.size() == 1'000u);

   for (int4 i = 0; i < 1'000; i += 2) {
      cat::verify(set.remove(i).has_value());
   }
   for (int4 i = 0; i < 1'000; ++i) {
      cat::verify(set.contains(i) == (i % 2 == 1));
   }
   cat::verify(set.size() == 500u);

   // `allocator_dyn` overloads.
   cat::verify(set.insert(pager, 0).has_value());
   cat::verify(set.reserve(pager, 2'000u).has_value());
   cat::verify(set.contains(0) && set.contains(999));

   set.free(pager);
   cat::verify(set.size() == 0u);
   cat::verify(!set.contains(1));

   static_assert(set_consteval());
}

$test(raii_set) {
   cat::allocator_dyn allocator(pager);
   cat::raii::set_big<int4> set(allocator);
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(set.insert(i).has_value());
   }
   cat::verify(!set.insert(7).has_value());
   cat::verify(set.remove(7).has_value());
   cat::verify(!set.contains(7) && set.contains(8));
   cat::verify(set.size() == 99u);

   // `release()` hands the storage to a manual `set`, which must be freed.
   cat::set_big<int4> released = set.release();
   cat::verify(released.contains(8));
   released.free(allocator);

   cat::raii::set_big<int4> other(allocator);
   cat::verify(other.insert(1).has_value());
   cat::verify(set.insert(2).has_value());
   set = cat::move(other);
   cat::verify(set.contains(1) && !set.contains(2) && other.size() == 0u);

   struct throwing_destructor {
      ~throwing_destructor() noexcept(false) {
      }
   };

   static_assert(cat::is_nothrow_destructible<typeof(set)>);
   static_assert(cat::is_nothrow_move_constructible<cat::set_big<int4>>);
   static_assert(cat::is_nothrow_move_assignable<cat::set_big<int4>>);
   static_assert(cat::is_nothrow_move_constructible<typeof(set)>);
   static_assert(cat::is_nothrow_move_assignable<typeof(set)>);
   static_assert(!cat::is_nothrow_destructible<throwing_destructor>);
   static_assert(
      cat::is_nothrow_destructible<cat::raii::set_big<throwing_destructor>>
   );
   static_assert(
      cat::is_nothrow_destructible<cat::set_big<throwing_destructor>>
   );
}

namespace {

auto
dyn_set_insert(cat::dyn_set<int4> set, int4 key) -> bool {
   return set.insert(pager, key).has_value();
}

auto
set_view_count(cat::set_view<int4> view, int4 count) -> int4 {
   int4 found = 0;
   for (int4 i = 0; i < count; ++i) {
      if (view.contains(i)) {
         ++found;
      }
   }
   return found;
}

}  // namespace

$test(set_ref) {
   cat::set<int4> small;
   cat::set_big<int4> big;

   // Sets with different `map_flags` bind to the same `set_ref`.
   for (cat::set_ref<int4> ref :
        {cat::set_ref<int4>(small), cat::set_ref<int4>(big)}) {
      for (int4 i = 0; i < 50; ++i) {
         cat::verify(ref.insert(pager, i).has_value());
      }
      cat::verify(!ref.insert<cat::allocator_page>(pager, 0).has_value());
      cat::verify(ref.remove(0).has_value() && !ref.contains(0));
      cat::verify(ref.size() == 49u && ref.capacity() >= 49u);
      cat::verify(ref.reserve(pager, 200u).has_value());
      cat::verify(ref.rehash(pager, 0u).has_value() && ref.contains(49));
   }
   cat::verify(small.contains(1) && big.contains(1));

   // A `set_ref` converts to type-erased references of the same set.
   cat::set_ref<int4> ref = small;
   cat::verify(dyn_set_insert(ref, 0) && small.contains(0));
   cat::verify(set_view_count(ref, 50) == 50);

   small.free(pager);
   big.free(pager);
   static_assert(sizeof(cat::set_ref<int4>) == sizeof(void*));
}

$test(dyn_set_and_view) {
   cat::set<int4> small;
   cat::set_big<int4> big;
   for (int4 i = 0; i < 100; ++i) {
      cat::verify(dyn_set_insert(small, i));
      cat::verify(dyn_set_insert(big, i));
   }
   cat::verify(!dyn_set_insert(small, 0));

   cat::dyn_set<int4> dyn = big;
   cat::verify(dyn.remove(5).has_value() && !dyn.remove(5).has_value());
   cat::verify(!dyn.contains(5) && dyn.size() == 99u);

   // A `dyn_set` narrows into a `set_view` of the same set, not of itself.
   cat::set_view<int4> view = cat::dyn_set<int4>(big);
   cat::verify(view.size() == 99u && !view.contains(5));

   // `const` sets can be viewed, but not bound by a `dyn_set`.
   cat::set<int4> const& const_set = small;
   cat::verify(set_view_count(const_set, 100) == 100);
   static_assert(!cat::is_convertible<typeof(const_set), cat::dyn_set<int4>>);

   static_assert(sizeof(cat::dyn_set<int4>) == 2u * sizeof(void*));
   static_assert(sizeof(cat::set_view<int4>) == 2u * sizeof(void*));

   small.free(pager);
   big.free(pager);
}

$test(make_set) {
   cat::set<int4> empty = cat::make_set<int4>();
   cat::verify(empty.size() == 0u && empty.capacity() == 0u);

   cat::set<int4> set = cat::make_set<int4>(pager, {1, 2, 1}).verify();
   cat::verify(set.size() == 2u && set.contains(1) && set.contains(2));
   set.free(pager);

   cat::set<int4> reserved = cat::make_set_reserved<int4>(pager, 100u).verify();
   cat::verify(reserved.capacity() >= 100u);
   reserved.free(pager);

   cat::allocator_dyn allocator(pager);
   auto raii_empty = cat::raii::make_set<int4>(allocator).verify();
   cat::verify(raii_empty.insert(1).has_value());

   auto raii_set = cat::raii::make_set<int4>(allocator, {3, 3, 4}).verify();
   cat::verify(raii_set.size() == 2u && raii_set.contains(4));

   auto raii_reserved =
      cat::raii::make_set_reserved<int4>(allocator, 100u).verify();
   cat::verify(raii_reserved.capacity() >= 100u);
}

$test(set_clear) {
   cat::set_big<int4> set;
   cat::verify(set.insert(pager, 1).has_value());
   idx const capacity = set.capacity();
   set.clear();
   cat::verify(set.is_empty() && set.capacity() == capacity);
   cat::verify(set.insert(pager, 1).has_value());
   set.free(pager);

   cat::allocator_dyn allocator(pager);
   cat::raii::set_big<int4> raii_set(allocator);
   cat::verify(raii_set.insert(1).has_value());
   raii_set.free();
   cat::verify(raii_set.is_empty() && raii_set.capacity() == 0u);
}

$test(set_alignment) {
   using group = cat::detail::map_group<int4, cat::monostate_type>;
   using aligned_set = cat::set<
      int4, __typeof_unqual(cat::hash),
      cat::detail::binary_comparator<cat::detail::equal_to_op>,
      cat::vec_big<group>, cat::container_flags::alignment(64u)>;
   // A set's alignment reaches the `basic_vec` that stores its groups.
   static_assert(cat::is_base_of<
                 cat::detail::set_base<
                    int4, __typeof_unqual(cat::hash),
                    cat::detail::binary_comparator<cat::detail::equal_to_op>,
                    cat::vec_big<group, cat::container_flags::alignment(64u)>>,
                 aligned_set>);

   aligned_set set;
   cat::verify(set.insert<cat::allocator_page>(pager, 1).has_value());
   cat::verify(set.contains(1));
   set.free<cat::allocator_page>(pager);
}

$test(set_allocation_failure) {
   cat::allocator_null null_alloc;
   cat::set_big<int4> values;
   cat::verify(!values.insert(null_alloc, 1).has_value());
   cat::verify(!values.reserve(null_alloc, 1u).has_value());
   cat::verify(values.size() == 0u);

   allocator_failing allocator;
   int4 inserted = 0;
   auto holds_inserted = [&] {
      if (values.size() != idx(inserted)) {
         return false;
      }

      for (int4 key = 0; key < inserted; ++key) {
         if (!values.contains(key)) {
            return false;
         }
      }

      return !values.contains(inserted);
   };

   // Growing many times keeps every key inserted before a failure.
   sweep_allocation_failures(
      allocator,
      [&] {
         for (; inserted < 2'000; ++inserted) {
            if (!values.insert(allocator, inserted).has_value()) {
               return false;
            }
         }

         return true;
      },
      [&] {
         cat::verify(holds_inserted());
         cat::verify(allocator.live_allocations <= 1u);
      }
   );

   cat::verify(holds_inserted());
   allocator.fail_after(0u);
   cat::verify(
      !values.reserve(allocator, idx(values.capacity() * 2u)).has_value()
   );
   allocator.never_fail();
   cat::verify(holds_inserted());

   // A set clones with its layout, or frees the copy.
   sweep_allocation_failures(
      allocator,
      [&] {
         cat::maybe<cat::set_big<int4>> copy = values.clone(allocator);
         if (!copy.has_value()) {
            return false;
         }

         cat::verify(copy.value().size() == values.size());
         cat::verify(copy.value().contains(0) && copy.value().contains(1'999));
         copy.value().remove(0).verify();
         cat::verify(values.contains(0));
         copy.value().free(allocator);
         return true;
      },
      [&] {
         cat::verify(holds_inserted() && allocator.live_allocations == 1u);
      }
   );

   values.free(allocator);
   cat::verify(allocator.live_allocations == 0u);

   // The destructor of a `raii::set` frees its table.
   {
      cat::raii::set_small<int4, allocator_failing, 0u> managed(allocator);
      for (int4 key = 0; key < 100; ++key) {
         managed.insert(key).verify();
      }

      allocator.fail_after(0u);
      cat::verify(!managed.reserve(idx(managed.capacity() * 2u)).has_value());
      allocator.never_fail();
      cat::verify(managed.size() == 100u && managed.contains(99));

      allocator.fail_after(0u);
      cat::verify(!managed.clone().has_value());
      allocator.never_fail();
      cat::raii::set_small<int4, allocator_failing, 0u> copy =
         managed.clone().verify();
      auto dynamic_copy = managed.clone(g_tests_alloc).verify();
      cat::verify(copy.contains(99) && dynamic_copy.contains(99));
      cat::verify(allocator.live_allocations == 2u);
   }

   cat::verify(allocator.live_allocations == 0u);
}
