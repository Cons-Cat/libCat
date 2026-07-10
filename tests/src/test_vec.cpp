#include <cat/iterable>
#include <cat/linear_allocator>
#include <cat/null_allocator>
#include <cat/page_allocator>
#include <cat/vec>

#include "../unit_tests.hpp"

namespace {

// NOLINTNEXTLINE
inline constinit idx destructor_count = 0u;

struct foo {
   bool live = true;

   foo() = default;
   foo(foo const&) = delete;

   foo(foo&& other) {
      other.live = false;
   }

   auto
   operator=(foo const&) -> foo& = delete;

   auto
   operator=(foo&& other) -> foo& {
      other.live = false;
      return *this;
   }

   ~foo() {
      if (live) {
         ++destructor_count;
         live = false;
      }
   }
};

consteval auto
const_func() -> int4 {
   cat::vec<int> vector;
   auto _ = vector.resize<cat::page_allocator>(pager, 8);

   vector[0] = 1;
   vector[1] = 2;
   vector[7] = 10;
   auto _ = vector.push_back<cat::page_allocator>(pager, 10);
   int4 result = vector[8];
   vector.free<cat::page_allocator>(pager);
   return result;
}

void
verify_all_ones(auto const& vector) {
   for (auto value : vector) {
      cat::verify(value == 1);
   }
}

// Boilerplate for tests that need a fast scratch arena: a page-backed
// linear allocator that frees its page when the helper destructs.
struct linear_arena {
   cat::span<cat::byte> page = pager.alloc_multi<cat::byte>(4_uki).verify();
   cat::linear_allocator alloc = cat::make_linear_allocator(page);

   ~linear_arena() {
      pager.free(page);
   }
};

}  // namespace

$test(vec_maybe_niche) {
   // `maybe<vec<T>>` packs the disengaged state into the
   // `data == nullptr && size != 0` niche, so it stays the size of
   // `vec<T>` itself, matching `maybe<idx>` and friends.
   static_assert(sizeof(cat::maybe<cat::vec<int4>>) == sizeof(cat::vec<int4>));
   cat::maybe<cat::vec<int4>> empty_vec;
   cat::verify(!empty_vec.has_value());
}

$test(raii_vec_maybe_niche) {
   // `maybe<raii::vec<T>>` reuses the manual core's niche, so it stays the
   // size of `raii::vec<T>` itself just like `maybe<vec<T>>`.
   static_assert(
      sizeof(cat::maybe<cat::raii::vec<int4>>) == sizeof(cat::raii::vec<int4>)
   );
   cat::maybe<cat::raii::vec<int4>> empty_vec;
   cat::verify(!empty_vec.has_value());

   linear_arena arena;
   auto engaged = cat::raii::make_vec<int4>(arena.alloc);
   cat::verify(engaged.has_value());
}

$test(vec_iterator_typedefs) {
   using test_vec_t = cat::vec<int4>;
   using iterator = test_vec_t::iterator;
   using const_iterator = test_vec_t::const_iterator;
   using reverse_iterator = test_vec_t::reverse_iterator;
   using const_reverse_iterator = test_vec_t::const_reverse_iterator;

   static_assert(cat::is_same<iterator::value_type, int4>);
   static_assert(cat::is_same<iterator::reference, int4&>);
   static_assert(cat::is_same<const_iterator::value_type, int4 const>);
   static_assert(cat::is_same<const_iterator::reference, int4 const&>);
   static_assert(cat::is_same<reverse_iterator::value_type, int4>);
   static_assert(cat::is_same<reverse_iterator::reference, int4&>);
   static_assert(cat::is_same<const_reverse_iterator::value_type, int4 const>);
   static_assert(cat::is_same<const_reverse_iterator::reference, int4 const&>);
   static_assert(cat::is_same<int, cat::vec<int>::value_type>);

   cat::vec<int4> v;
   static_assert(cat::is_same<iterator, decltype(v.begin())>);
   static_assert(cat::is_same<iterator, decltype(v.end())>);
   static_assert(cat::is_same<const_iterator, decltype(v.cbegin())>);
   static_assert(cat::is_same<const_iterator, decltype(v.cend())>);
   static_assert(cat::is_same<reverse_iterator, decltype(v.rbegin())>);
   static_assert(cat::is_same<reverse_iterator, decltype(v.rend())>);
   static_assert(cat::is_same<const_reverse_iterator, decltype(v.crbegin())>);
   static_assert(cat::is_same<const_reverse_iterator, decltype(v.crend())>);
}

$test(vec_constexpr_usage) {
   static_assert(const_func() == 10);
}

$test(vec_default_construct) {
   linear_arena arena;
   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() == 0);
   cat::verify(v.is_empty());
}

$test(vec_push_back) {
   linear_arena arena;
   cat::allocator_ref allocator_reference = arena.alloc;

   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };

   // Each `push_back` flavor: a bare allocator, an `allocator_ref`,
   // an explicit-template-allocator dispatch.
   v.push_back(arena.alloc, 1_i4).verify();
   v.push_back(allocator_reference, 2_i4).verify();
   v.push_back<cat::linear_allocator>(arena.alloc, 3_i4).verify();
   cat::verify(v.size() == 3);
   cat::verify(v.capacity() >= 4);

   // Push past the initial growth threshold to force another doubling.
   for (idx i = 0u; i < 5u; ++i) {
      v.push_back(arena.alloc, 0_i4).verify();
   }
   cat::verify(v.size() == 8);
   cat::verify(v.capacity() >= 8);
   cat::verify(v.data() != nullptr);
}

$test(vec_reset_preserves_capacity) {
   linear_arena arena;
   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };
   v.push_back(arena.alloc, 1_i4).verify();
   v.push_back(arena.alloc, 2_i4).verify();
   idx const cap_before = v.capacity();

   v.reset();
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() == cap_before);
}

$test(vec_resize) {
   linear_arena arena;
   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };

   v.resize<cat::linear_allocator>(arena.alloc, 4).verify();
   cat::verify(v.size() == 4);
   cat::verify(v.capacity() >= 4);

   // Shrinking via `resize` keeps capacity.
   idx const cap_after_grow = v.capacity();
   v.resize(arena.alloc, 2).verify();
   cat::verify(v.size() == 2);
   cat::verify(v.capacity() == cap_after_grow);

   // Growing past capacity reallocates and reaches `is_full`.
   v.resize(arena.alloc, cap_after_grow + 1u).verify();
   cat::verify(v.size() == cap_after_grow + 1u);
   cat::verify(v.is_full());
}

$test(vec_reserve) {
   linear_arena arena;
   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };
   v.reserve(arena.alloc, 128).verify();
   cat::verify(v.is_empty());
   cat::verify(v.capacity() >= 128);
}

$test(vec_compare_trivial_equality) {
   static_assert(cat::is_trivially_equality_comparable<int>);

   linear_arena arena;
   cat::vec left = cat::make_vec<int>(arena.alloc, {1, 2, 3, 4}).verify();
   cat::vec same = cat::make_vec<int>(arena.alloc, {1, 2, 3, 4}).verify();
   cat::vec different = cat::make_vec<int>(arena.alloc, {1, 2, 9, 4}).verify();
   cat::vec shorter = cat::make_vec<int>(arena.alloc, {1, 2, 3}).verify();
   cat::vec bytes =
      cat::make_vec<unsigned char>(arena.alloc, {1u, 2u, 200u}).verify();
   cat::vec bigger_bytes =
      cat::make_vec<unsigned char>(arena.alloc, {1u, 2u, 201u}).verify();
   $defer {
      left.free(arena.alloc);
      same.free(arena.alloc);
      different.free(arena.alloc);
      shorter.free(arena.alloc);
      bytes.free(arena.alloc);
      bigger_bytes.free(arena.alloc);
   };

   cat::verify(left == same);
   cat::verify(!(left == different));
   cat::verify(left != shorter);
   cat::verify((left <=> same) == 0);
   cat::verify((left <=> different) < 0);
   cat::verify((different <=> left) > 0);
   cat::verify((left <=> shorter) > 0);
   cat::verify((shorter <=> left) < 0);
   cat::verify((bytes <=> bigger_bytes) < 0);
}

$test(vec_make_factories) {
   linear_arena arena;

   // `make_vec` with the default empty initializer list returns an
   // engaged but empty vec.
   cat::vec empty = cat::make_vec<int4>(arena.alloc).verify();
   $defer {
      empty.free(arena.alloc);
   };
   cat::verify(empty.size() == 0);

   // `make_vec` populated with an initializer list reserves and pushes.
   cat::vec list_vec =
      cat::make_vec<int4>(arena.alloc, {1_i4, 2_i4, 3_i4}).verify();
   $defer {
      list_vec.free(arena.alloc);
   };
   cat::verify(list_vec.size() == 3);
   cat::verify(list_vec[2] == 3);

   cat::vec reserved = cat::make_vec_reserved<int4>(arena.alloc, 6).verify();
   $defer {
      reserved.free(arena.alloc);
   };
   cat::verify(reserved.capacity() >= 6);
   cat::verify(reserved.is_empty());

   // The static-dispatch overload accepts an explicit allocator type.
   cat::vec static_reserved =
      cat::make_vec_reserved<int4, cat::linear_allocator>(arena.alloc, 7)
         .verify();
   $defer {
      static_reserved.free(arena.alloc);
   };
   cat::verify(static_reserved.capacity() >= 7);

   cat::vec filled = cat::make_vec_filled(arena.alloc, 8, 1_i4).verify();
   $defer {
      filled.free(arena.alloc);
   };
   cat::verify(filled.size() == 8);
   verify_all_ones(filled);

   cat::vec static_filled =
      cat::make_vec_filled<int4, cat::linear_allocator>(arena.alloc, 3, 9_i4)
         .verify();
   $defer {
      static_filled.free(arena.alloc);
   };
   cat::verify(static_filled.size() == 3);
   cat::verify(static_filled[2] == 9);

   // Factories that allocate at all start with a minimum capacity of 4
   // so that the first few `push_back`s do not immediately reallocate.
   cat::verify(list_vec.capacity() >= 4);
   cat::verify(static_filled.capacity() >= 4);

   cat::vec tiny_reserved =
      cat::make_vec_reserved<int4>(arena.alloc, 1).verify();
   $defer {
      tiny_reserved.free(arena.alloc);
   };
   cat::verify(tiny_reserved.capacity() >= 4);

   // A 0-capacity request stays allocation-free.
   cat::verify(empty.capacity() == 0);
}

$test(vec_clone) {
   linear_arena arena;
   cat::vec source = cat::make_vec_filled(arena.alloc, 8, 1_i4).verify();
   $defer {
      source.free(arena.alloc);
   };

   cat::vec cloned = source.clone(arena.alloc).verify();
   $defer {
      cloned.free(arena.alloc);
   };
   cat::verify(cloned.size() == 8);
   verify_all_ones(cloned);

   cat::vec static_cloned =
      source.clone<cat::linear_allocator>(arena.alloc).verify();
   $defer {
      static_cloned.free(arena.alloc);
   };
   cat::verify(static_cloned.size() == 8);
   verify_all_ones(static_cloned);
}

$test(vec_swap) {
   linear_arena arena;
   cat::vec left = cat::make_vec_filled(arena.alloc, 2, 1_i4).verify();
   $defer {
      left.free(arena.alloc);
   };
   cat::vec right = cat::make_vec_filled(arena.alloc, 3, 3_i4).verify();
   $defer {
      right.free(arena.alloc);
   };
   left[1] = 2_i4;
   right[1] = 4_i4;
   right[2] = 5_i4;

   cat::swap(left, right);
   cat::verify(left.size() == 3);
   cat::verify(left[0] == 3);
   cat::verify(left[2] == 5);
   cat::verify(right.size() == 2);
   cat::verify(right[0] == 1);
   cat::verify(right[1] == 2);
}

$test(vec_move_construct) {
   linear_arena arena;
   cat::vec source = cat::make_vec_filled(arena.alloc, 2, 7_i4).verify();
   $defer {
      source.free(arena.alloc);
   };

   cat::vec moved = cat::move(source);
   $defer {
      moved.free(arena.alloc);
   };
   cat::verify(moved.size() == 2);
   cat::verify(moved[0] == 7);
   // The moved-from vec is left empty so its destructor is a no-op.
   cat::verify(source.size() == 0);
   cat::verify(source.data() == nullptr);
}

$test(vec_move_assign) {
   linear_arena arena;
   cat::vec source = cat::make_vec_filled(arena.alloc, 2, 8_i4).verify();
   $defer {
      source.free(arena.alloc);
   };
   cat::vec<int4> sink;
   $defer {
      sink.free(arena.alloc);
   };
   sink = cat::move(source);
   cat::verify(sink.size() == 2);
   cat::verify(sink[1] == 8);
   cat::verify(source.data() == nullptr);
}

$test(vec_self_move_assign) {
   linear_arena arena;
   cat::vec v = cat::make_vec_filled(arena.alloc, 3, 9_i4).verify();
   $defer {
      v.free(arena.alloc);
   };

   int4 const* const p_original_data = v.data();
   idx const original_size = v.size();
   idx const original_capacity = v.capacity();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
   v = cat::move(v);
#pragma clang diagnostic pop

   cat::verify(v.data() == p_original_data);
   cat::verify(v.size() == original_size);
   cat::verify(v.capacity() == original_capacity);
   cat::verify(v[2] == 9);
}

$test(vec_container_algorithms) {
   linear_arena arena;
   cat::vec origin = cat::make_vec_filled(arena.alloc, 6, 1).verify();
   $defer {
      origin.free(arena.alloc);
   };
   auto copies = cat::make_vec_filled(arena.alloc, 6, 0).verify();
   $defer {
      copies.free(arena.alloc);
   };
   auto moved = cat::make_vec_filled(arena.alloc, 6, 0).verify();
   $defer {
      moved.free(arena.alloc);
   };
   auto relocated = cat::make_vec_filled(arena.alloc, 6, 0).verify();
   $defer {
      relocated.free(arena.alloc);
   };

   cat::copy(origin.begin(), origin.end(), copies.begin());
   cat::verify(copies[5] == 1);
   copies[5] = 0;
   origin.copy_to(copies);
   cat::verify(copies[5] == 1);

   cat::move(origin.begin(), origin.end(), moved.begin());
   cat::verify(moved[5] == 1);
   moved[5] = 0;
   origin.move_to(moved);
   cat::verify(moved[5] == 1);

   cat::relocate(origin.begin(), origin.end(), relocated.begin());
   cat::verify(relocated[5] == 1);
   relocated[5] = 0;
   origin.relocate_to(relocated);
   cat::verify(relocated[5] == 1);
}

$test(vec_null_allocator_failure) {
   cat::null_allocator null_alloc;
   cat::vec<int4> v;
   $defer {
      v.free(null_alloc);
   };
   cat::verify(!v.reserve(null_alloc, 1).has_value());
}

$test(vec_free_resets_state) {
   linear_arena arena;
   cat::vec<int4> v;
   v.push_back(arena.alloc, 11_i4).verify();
   cat::verify(v.size() == 1);

   v.free(arena.alloc);
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() == 0);
   cat::verify(v.data() == nullptr);
}

$test(vec_abandon_outlives_allocator) {
   // An unmanaged `cat::vec` may fall out of scope without `.free()`
   // when its allocator outlives it. The destructor only invokes
   // element destructors, so the buffer stays owned by the surrounding
   // allocator and is reclaimed when that allocator itself is torn
   // down.
   linear_arena arena;
   {
      cat::vec<int4> abandoned;
      abandoned.push_back(arena.alloc, 13_i4).verify();
      abandoned.push_back(arena.alloc, 17_i4).verify();
      cat::verify(abandoned.size() == 2);
   }
   cat::vec<int4> post = cat::make_vec_reserved<int4>(arena.alloc, 1).verify();
   $defer {
      post.free(arena.alloc);
   };
   post.push_back(arena.alloc, 19_i4).verify();
   cat::verify(post[0] == 19);
}

// When the wrapped allocator supports in-place `reallocate`, `shrink_to_fit`
// must reuse the existing buffer instead of allocating-then-copying. The
// downward-bump `linear_allocator` honors shrinks in place, so the address
// returned by `data()` must match before and after the shrink.
$test(vec_shrink_to_fit_in_place_via_reallocate) {
   linear_arena arena;
   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };
   v.reserve(arena.alloc, 32u).verify();
   v.push_back(arena.alloc, 1_i4).verify();
   v.push_back(arena.alloc, 2_i4).verify();
   v.push_back(arena.alloc, 3_i4).verify();
   cat::verify(v.capacity() >= 32);

   int4 const* const p_before = v.data();
   v.shrink_to_fit(arena.alloc).verify();

   cat::verify(v.data() == p_before);
   cat::verify(v.capacity() == v.size());
   cat::verify(v.size() == 3);
   cat::verify(v[0] == 1);
   cat::verify(v[1] == 2);
   cat::verify(v[2] == 3);
}

$test(vec_shrink_to_fit) {
   // `shrink_to_fit` releases excess capacity left behind by growth
   // patterns and downward `resize`s. Without it, vecs propagate their
   // worst-case capacity forever, which is the libCat analog of the
   // Rust `collect::<Vec<_>>()` footgun.
   linear_arena arena;
   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };
   v.reserve(arena.alloc, 64).verify();
   v.push_back(arena.alloc, 21_i4).verify();
   v.push_back(arena.alloc, 22_i4).verify();
   cat::verify(v.capacity() >= 64);

   v.shrink_to_fit(arena.alloc).verify();
   cat::verify(v.size() == 2);
   cat::verify(v.capacity() == 2);
   cat::verify(v[0] == 21);
   cat::verify(v[1] == 22);

   // Idempotent when capacity already matches size.
   v.shrink_to_fit(arena.alloc).verify();
   cat::verify(v.capacity() == 2);

   // Shrinking an emptied vec reallocates to a zero-length buffer but
   // does not free. Use `.free(allocator)` to release the storage.
   v.reset();
   v.shrink_to_fit(arena.alloc).verify();
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() == 0);

   // Shrinking a fresh empty vec is a no-op.
   cat::vec<int4> empty_vec;
   empty_vec.shrink_to_fit(arena.alloc).verify();
   cat::verify(empty_vec.capacity() == 0);
   cat::verify(empty_vec.data() == nullptr);
}

$test(vec_element_destructors) {
   linear_arena arena;
   destructor_count = 0u;
   {
      cat::vec<foo> foo_vec;
      $defer {
         foo_vec.free(arena.alloc);
      };
      foo_vec.push_back(arena.alloc, foo{}).verify();
      foo_vec.push_back(arena.alloc, foo{}).verify();
      foo_vec.push_back(arena.alloc, foo{}).verify();
      destructor_count = 0u;
   }
   cat::assert(destructor_count == 3);
}

$test(raii_vec_make_and_push_back) {
   linear_arena arena;
   cat::raii::vec v = cat::raii::make_vec<int4>(arena.alloc).verify();
   cat::verify(v.is_empty());
   cat::verify(v.allocator() == v.allocator());

   v.push_back(1_i4).verify();
   v.push_back(2_i4).verify();
   v.push_back(3_i4).verify();
   cat::verify(v.size() == 3);
   cat::verify(v.capacity() >= 4);
   cat::verify(v.data() != nullptr);
}

$test(raii_vec_resize) {
   linear_arena arena;
   cat::raii::vec v = cat::raii::make_vec<int4>(arena.alloc).verify();
   v.push_back(1_i4).verify();
   v.push_back(2_i4).verify();
   v.push_back(3_i4).verify();

   v.resize(0).verify();
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() >= 4);

   v.resize(4).verify();
   cat::verify(v.size() == 4);
}

$test(raii_vec_reserve) {
   linear_arena arena;
   cat::raii::vec v = cat::raii::make_vec<int4>(arena.alloc).verify();
   v.reserve(128).verify();
   cat::verify(v.capacity() >= 128);
}

$test(raii_vec_reset_releases_storage) {
   linear_arena arena;
   cat::raii::vec v = cat::raii::make_vec<int4>(arena.alloc).verify();
   v.push_back(1_i4).verify();
   v.push_back(2_i4).verify();
   // raii's `reset()` deallocates (unlike unmanaged), since the wrapper
   // owns the allocator.
   v.reset();
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() == 0);
}

$test(raii_vec_factories) {
   linear_arena arena;
   cat::raii::vec reserved =
      cat::raii::make_vec_reserved<int4>(arena.alloc, 6).verify();
   cat::verify(reserved.capacity() >= 6);

   cat::raii::vec static_reserved =
      cat::raii::make_vec_reserved<int4, cat::linear_allocator>(arena.alloc, 7)
         .verify();
   cat::verify(static_reserved.capacity() >= 7);

   cat::raii::vec filled =
      cat::raii::make_vec_filled(arena.alloc, 8, 1_i4).verify();
   cat::verify(filled.size() == 8);
   verify_all_ones(filled);

   cat::raii::vec static_filled =
      cat::raii::make_vec_filled<int4, cat::linear_allocator>(
         arena.alloc, 3, 9_i4
      )
         .verify();
   cat::verify(static_filled.size() == 3);
   cat::verify(static_filled[2] == 9);
}

$test(raii_vec_clone) {
   linear_arena arena;
   cat::raii::vec source =
      cat::raii::make_vec_filled(arena.alloc, 8, 1_i4).verify();

   cat::raii::vec cloned = source.clone(arena.alloc).verify();
   cat::verify(cloned.size() == 8);
   verify_all_ones(cloned);

   cat::raii::vec static_cloned =
      source.clone<cat::linear_allocator>(arena.alloc).verify();
   cat::verify(static_cloned.size() == 8);
   verify_all_ones(static_cloned);
}

$test(raii_vec_swap) {
   linear_arena arena;
   cat::raii::vec left =
      cat::raii::make_vec<int4>(arena.alloc, {1_i4, 2_i4}).verify();
   cat::raii::vec right =
      cat::raii::make_vec<int4>(arena.alloc, {3_i4, 4_i4, 5_i4}).verify();
   cat::swap(left, right);
   cat::verify(left.size() == 3);
   cat::verify(left[0] == 3);
   cat::verify(left[2] == 5);
   cat::verify(right.size() == 2);
   cat::verify(right[0] == 1);
   cat::verify(right[1] == 2);
}

$test(raii_vec_move_construct) {
   linear_arena arena;
   cat::raii::vec source =
      cat::raii::make_vec_filled(arena.alloc, 2, 7_i4).verify();
   cat::raii::vec moved = cat::move(source);
   cat::verify(moved.size() == 2);
   cat::verify(moved[0] == 7);
}

$test(raii_vec_move_assign) {
   linear_arena arena;
   cat::raii::vec source =
      cat::raii::make_vec_filled(arena.alloc, 2, 8_i4).verify();
   cat::raii::vec sink = cat::raii::make_vec<int4>(arena.alloc).verify();
   sink = cat::move(source);
   cat::verify(sink.size() == 2);
   cat::verify(sink[1] == 8);
}

$test(raii_vec_self_move_assign) {
   linear_arena arena;
   cat::raii::vec v = cat::raii::make_vec_filled(arena.alloc, 3, 9_i4).verify();

   int4 const* const p_original_data = v.data();
   idx const original_size = v.size();
   idx const original_capacity = v.capacity();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
   v = cat::move(v);  // NOLINT
#pragma clang diagnostic pop

   cat::verify(v.data() == p_original_data);
   cat::verify(v.size() == original_size);
   cat::verify(v.capacity() == original_capacity);
   cat::verify(v[2] == 9);
}

$test(raii_vec_release) {
   linear_arena arena;
   cat::raii::vec source =
      cat::raii::make_vec<int4>(arena.alloc, {4_i4, 5_i4}).verify();
   cat::vec released = cat::move(source).release();
   $defer {
      released.free(arena.alloc);
   };
   cat::verify(released.size() == 2);
   cat::verify(released[0] == 4);
}

$test(raii_vec_collection_trait) {
   using raii_vec_t = cat::raii::vec<int4, cat::page_allocator>;
   static_assert(cat::is_random_access_collection<raii_vec_t>);
}

$test(raii_vec_shrink_to_fit) {
   linear_arena arena;
   cat::raii::vec<int4, cat::linear_allocator> v(arena.alloc);
   v.reserve(64).verify();
   v.push_back(31_i4).verify();
   v.push_back(32_i4).verify();
   cat::verify(v.capacity() >= 64);

   v.shrink_to_fit().verify();
   cat::verify(v.size() == 2);
   cat::verify(v.capacity() == 2);
   cat::verify(v[0] == 31);

   v.reset();
   v.shrink_to_fit().verify();
   cat::verify(v.capacity() == 0);
   cat::verify(v.data() == nullptr);
}

$test(raii_vec_element_destructors) {
   linear_arena arena;
   destructor_count = 0u;
   {
      cat::raii::vec<foo, cat::linear_allocator> raii_foos(arena.alloc);
      raii_foos.push_back(foo{}).verify();
      raii_foos.push_back(foo{}).verify();
      raii_foos.push_back(foo{}).verify();
   }
   cat::assert(destructor_count == 3);
}

$test(vec_collection) {
   using flux_test_vec = cat::vec<int>;
   static_assert(cat::is_random_access_collection<flux_test_vec>);

   auto vec_values = cat::make_vec_filled(pager, 3, 0).verify();
   $defer {
      vec_values.free(pager);
   };
   vec_values[0] = 5;
   vec_values[1] = 6;
   vec_values[2] = 7;
   cat::verify((vec_values | cat::sum()) == 18);
   cat::verify(cat::read_at(vec_values, 1u) == 6);
   auto vec_tail = cat::ref(vec_values) | cat::reverse() | cat::take(2u);
   cat::verify(vec_tail.sum() == 13);
   vec_values | cat::reverse_inplace();
   cat::verify(vec_values[0] == 7);
   cat::verify(vec_values[1] == 6);
   cat::verify(vec_values[2] == 5);
   vec_values.reverse_inplace();
   cat::verify(vec_values[0] == 5);
   cat::verify(vec_values[1] == 6);
   cat::verify(vec_values[2] == 7);

   cat::raii::vec managed_values =
      cat::raii::make_vec_filled(pager, 3u, 0).verify();
   managed_values[0] = 8;
   managed_values[1] = 9;
   managed_values[2] = 10;
   managed_values | cat::reverse_inplace();
   cat::verify(managed_values[0] == 10);
   cat::verify(managed_values[1] == 9);
   cat::verify(managed_values[2] == 8);
   managed_values.reverse_inplace();
   cat::verify(managed_values[0] == 8);
   cat::verify(managed_values[1] == 9);
   cat::verify(managed_values[2] == 10);

   auto transformed_tail = cat::ref(vec_values)
                              .filter([](int value) -> bool {
                                 return value > 5;
                              })
                              .transform([](int value) -> int {
                                 return value * 10;
                              });
   cat::verify(transformed_tail.sum() == 130);
   cat::verify(cat::as_span(vec_values).size() == 3u);
}

$test(vec_reverse_inplace_simd) {
   cat::vec<int> values;
   values.resize(pager, 65u, 0).verify();
   $defer {
      values.free(pager);
   };

   for (cat::idx i = 0u; i < values.size(); ++i) {
      values[i] = static_cast<int>(i);
   }

   values | cat::reverse_inplace();
   for (cat::idx i = 0u; i < values.size(); ++i) {
      cat::verify(values[i] == static_cast<int>(64u - i));
   }

   values.reverse_inplace();
   for (cat::idx i = 0u; i < values.size(); ++i) {
      cat::verify(values[i] == static_cast<int>(i));
   }

   cat::vec<cat::int4> wrapped_values;
   wrapped_values.resize(pager, 65u, 0_i4).verify();
   $defer {
      wrapped_values.free(pager);
   };
   for (cat::idx i = 0u; i < wrapped_values.size(); ++i) {
      wrapped_values[i] = cat::int4(i);
   }

   wrapped_values | cat::reverse_inplace();
   for (cat::idx i = 0u; i < wrapped_values.size(); ++i) {
      cat::verify(wrapped_values[i] == cat::int4(64u - i));
   }
}
