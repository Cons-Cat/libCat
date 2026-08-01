#include <cat/iterable>
#include <cat/linear_allocator>
#include <cat/null_allocator>
#include <cat/page_allocator>
#include <cat/vec>

#include "../unit_tests.hpp"

namespace {
// NOLINTNEXTLINE
inline constinit idx relocation_move_count = 0u;
}  // namespace

struct relocation_only_probe {
   int4 value = 0;

   relocation_only_probe() = default;
   relocation_only_probe(relocation_only_probe const&) = delete;

   relocation_only_probe(relocation_only_probe&& other) : value(other.value) {
      ++relocation_move_count;
   }

   auto
   operator=(relocation_only_probe const&) -> relocation_only_probe& = delete;
   auto
   operator=(relocation_only_probe&&) -> relocation_only_probe& = delete;
};

template <>
inline constexpr bool cat::is_trivially_relocatable<relocation_only_probe> =
   true;

namespace {

// NOLINTNEXTLINE
inline constinit idx destructor_count = 0u;

// NOLINTNEXTLINE
inline constinit idx lifetime_live_count = 0u;
// NOLINTNEXTLINE
inline constinit idx lifetime_destructor_count = 0u;
// NOLINTNEXTLINE
inline constinit idx lifetime_dead_assignment_count = 0u;

struct lifetime_probe {
   int4 value = 0;
   bool live = true;

   lifetime_probe() {
      ++lifetime_live_count;
   }

   explicit lifetime_probe(int4 new_value) : value(new_value) {
      ++lifetime_live_count;
   }

   lifetime_probe(lifetime_probe const& other) : value(other.value) {
      ++lifetime_live_count;
   }

   lifetime_probe(lifetime_probe&& other) : value(other.value) {
      ++lifetime_live_count;
   }

   auto
   operator=(lifetime_probe const& other) -> lifetime_probe& {
      if (this == &other) {
         return *this;
      }
      if (!live) {
         ++lifetime_dead_assignment_count;
      }
      value = other.value;
      return *this;
   }

   auto
   operator=(lifetime_probe&& other) -> lifetime_probe& {
      if (this == &other) {
         return *this;
      }
      if (!live) {
         ++lifetime_dead_assignment_count;
      }
      value = other.value;
      return *this;
   }

   ~lifetime_probe() {
      if (live) {
         live = false;
         lifetime_live_count = idx(lifetime_live_count - 1u);
         ++lifetime_destructor_count;
      }
   }
};

static_assert(cat::is_trivially_relocatable<relocation_only_probe>);

struct constexpr_lifetime_probe {
   idx* p_live = nullptr;
   int4 value = 0;

   constexpr constexpr_lifetime_probe() = default;

   constexpr constexpr_lifetime_probe(idx& live, int4 new_value)
       : p_live(&live), value(new_value) {
      ++live;
   }

   constexpr constexpr_lifetime_probe(constexpr_lifetime_probe const& other)
       : p_live(other.p_live), value(other.value) {
      if (p_live != nullptr) {
         ++*p_live;
      }
   }

   constexpr auto
   operator=(constexpr_lifetime_probe const& other)
      -> constexpr_lifetime_probe& {
      if (this == &other) {
         return *this;
      }
      if (p_live == nullptr && other.p_live != nullptr) {
         ++*other.p_live;
      }
      p_live = other.p_live;
      value = other.value;
      return *this;
   }

   constexpr ~constexpr_lifetime_probe() {
      if (p_live != nullptr) {
         *p_live = idx(*p_live - 1u);
      }
   }
};

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

consteval auto
vec_consteval_reserve_preserves_values() -> bool {
   cat::vec<int4> values;
   auto _ = values.push_back(pager, 11_i4);
   auto _ = values.push_back(pager, 22_i4);
   auto _ = values.reserve(pager, 16u);
   bool const result =
      values.size() == 2u && values[0u] == 11_i4 && values[1u] == 22_i4;
   values.free(pager);
   return result;
}

consteval auto
vec_consteval_growth_preserves_values() -> bool {
   cat::vec<int4> values;
   for (idx i = 0u; i < 9u; ++i) {
      auto _ = values.push_back(pager, int4(i + 1u));
   }
   bool result = values.size() == 9u;
   for (idx i = 0u; i < values.size(); ++i) {
      result = result && values[i] == int4(i + 1u);
   }
   values.free(pager);
   return result;
}

consteval auto
vec_consteval_resize_lifetimes() -> bool {
   idx live = 0u;
   constexpr_lifetime_probe value(live, 7_i4);
   cat::vec<constexpr_lifetime_probe> values;
   auto _ = values.resize(pager, 3u, value);
   bool result = live == 4u;
   auto _ = values.resize(pager, 1u, value);
   result = result && live == 2u && values[0u].value == 7_i4;
   auto _ = values.resize(pager, 4u, value);
   result = result && live == 5u;
   for (auto const& element : values) {
      result = result && element.value == 7_i4;
   }
   values.free(pager);
   return result && live == 1u;
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

template <typename Vector>
void
verify_vec_niche() {
   static_assert(sizeof(cat::maybe<Vector>) == sizeof(Vector));
   cat::maybe<Vector> empty;
   cat::verify(!empty.has_value());
}

template <typename Vector>
concept can_change_vec_size =
   requires(Vector& vector, cat::linear_allocator& allocator) {
      vector.reserve(allocator, 1u);
      vector.push_back(allocator, 1_i4);
      vector.resize(allocator, 1u);
      vector.clear();
      vector.erase(0u);
   };

template <typename Vector>
concept can_reserve_raii = requires(Vector& vector) { vector.reserve(1u); };

}  // namespace

$test(vec_maybe_niche) {
   verify_vec_niche<cat::vec<int4>>();
   verify_vec_niche<cat::vec<int4, cat::vec_flags::pointer_size_layout>>();
   verify_vec_niche<cat::small_vec<int4>>();
   verify_vec_niche<
      cat::small_vec<int4, 4u, cat::vec_flags::pointer_size_layout>>();
}

$test(raii_vec_maybe_niche) {
   verify_vec_niche<cat::raii::vec<int4>>();
   verify_vec_niche<cat::raii::vec<
      int4, cat::dyn_allocator, cat::vec_flags::pointer_size_layout>>();
   verify_vec_niche<cat::raii::small_vec<int4>>();
   verify_vec_niche<cat::raii::small_vec<
      int4, cat::dyn_allocator, 4u, cat::vec_flags::pointer_size_layout>>();

   linear_arena arena;
   auto engaged = cat::raii::make_vec<int4>(arena.alloc);
   cat::verify(engaged.has_value());
}

$test(vec_flags) {
   constexpr auto flags =
      cat::vec_flags::pointer_size_layout | cat::vec_flags::inline_storage(4u);
   static_assert(flags.uses_pointer_size_layout);
   static_assert(!flags.is_fixed_size);
   static_assert(flags.inline_storage_count == 4u);

   static_assert(sizeof(cat::vec<int4>) == sizeof(void*) * 3u);
   static_assert(
      sizeof(cat::vec<int4, cat::vec_flags::pointer_size_layout>)
      == sizeof(void*) * 3u
   );
   static_assert(
      sizeof(cat::vec<int4, cat::vec_flags::fixed_size>) == sizeof(void*) * 2u
   );
   static_assert(
      sizeof(cat::vec<
             int4,
             cat::vec_flags::fixed_size | cat::vec_flags::pointer_size_layout>)
      == sizeof(void*) * 2u
   );
}

$test(fixed_vec_aliases) {
   static_assert(
      cat::is_same<
         cat::vec_fixed<int4>, cat::vec<int4, cat::vec_flags::fixed_size>>
   );
   static_assert(cat::is_same<
                 cat::small_vec_fixed<int4>,
                 cat::small_vec<int4, 4u, cat::vec_flags::fixed_size>>);
   static_assert(
      cat::is_same<
         cat::raii::vec_fixed<int4>,
         cat::raii::vec<int4, cat::dyn_allocator, cat::vec_flags::fixed_size>>
   );
   static_assert(cat::is_same<
                 cat::raii::small_vec_fixed<int4>,
                 cat::raii::small_vec<
                    int4, cat::dyn_allocator, 4u, cat::vec_flags::fixed_size>>);
   static_assert(!can_change_vec_size<cat::vec_fixed<int4>>);
   static_assert(!can_change_vec_size<cat::small_vec_fixed<int4>>);
   static_assert(!can_reserve_raii<cat::raii::vec_fixed<int4>>);
   static_assert(!can_reserve_raii<cat::raii::small_vec_fixed<int4>>);
}

$test(fixed_size_vec) {
   linear_arena arena;
   using fixed_vec = cat::vec_fixed<int4>;
   static_assert(!can_change_vec_size<fixed_vec>);

   fixed_vec values =
      cat::make_vec<int4, cat::linear_allocator, cat::vec_flags::fixed_size>(
         arena.alloc, {1_i4, 2_i4, 3_i4}
      )
         .verify();
   cat::verify(values.size() == 3u);
   cat::verify(values.capacity() == 3u);
   cat::verify(values[1u] == 2_i4);
   values.free(arena.alloc);

   cat::small_vec_fixed<int4> inline_values =
      cat::make_vec<
         int4, cat::linear_allocator,
         cat::vec_flags::fixed_size | cat::vec_flags::inline_storage(4u)>(
         arena.alloc, {4_i4, 5_i4}
      )
         .verify();
   cat::verify(inline_values.size() == 2u);
   cat::verify(inline_values.capacity() == 2u);
   inline_values.free(arena.alloc);
}

$test(vec_fill_families) {
   linear_arena arena;

   auto small =
      cat::make_small_vec_filled<int4>(arena.alloc, 3u, 1_i4).verify();
   auto fixed =
      cat::make_vec_fixed_filled<int4>(arena.alloc, 3u, 2_i4).verify();
   auto small_fixed =
      cat::make_small_vec_fixed_filled<int4>(arena.alloc, 3u, 3_i4).verify();

   small.fill(4_i4);
   fixed.fill(5_i4);
   small_fixed.fill(6_i4);
   cat::verify(small[2u] == 4_i4);
   cat::verify(fixed[2u] == 5_i4);
   cat::verify(small_fixed[2u] == 6_i4);

   small.free(arena.alloc);
   fixed.free(arena.alloc);
   small_fixed.free(arena.alloc);

   auto raii_small =
      cat::raii::make_small_vec_filled<int4>(arena.alloc, 3u, 1_i4).verify();
   auto raii_fixed =
      cat::raii::make_vec_fixed_filled<int4>(arena.alloc, 3u, 2_i4).verify();
   auto raii_small_fixed =
      cat::raii::make_small_vec_fixed_filled<int4>(arena.alloc, 3u, 3_i4)
         .verify();
   auto ordered =
      cat::raii::make_small_vec_filled<int4, 8u, cat::linear_allocator>(
         arena.alloc, 3u, 7_i4
      )
         .verify();

   raii_small.fill(4_i4);
   raii_fixed.fill(5_i4);
   raii_small_fixed.fill(6_i4);
   cat::verify(raii_small[2u] == 4_i4);
   cat::verify(raii_fixed[2u] == 5_i4);
   cat::verify(raii_small_fixed[2u] == 6_i4);
   cat::verify(ordered[2u] == 7_i4);
}

$test(small_vec_inline_storage) {
   linear_arena arena;
   idx const bytes_before = arena.alloc.bytes_used();

   cat::small_vec<int4, 8u> values;
   for (idx i = 0u; i < 8u; ++i) {
      values.push_back(arena.alloc, int4(i)).verify();
   }
   cat::verify(arena.alloc.bytes_used() == bytes_before);

   values.push_back(arena.alloc, 8_i4).verify();
   cat::verify(arena.alloc.bytes_used() > bytes_before);
   cat::verify(values.size() == 9u);
   values.free(arena.alloc);

   cat::raii::small_vec<int4, cat::linear_allocator, 8u> managed(arena.alloc);
   managed.push_back(1_i4).verify();
   cat::verify(managed[0] == 1_i4);
}

$test(vec_append_range_variants) {
   linear_arena arena;
   cat::array<int4, 3u> source{1_i4, 2_i4, 3_i4};

   auto verify_manual = [&]<typename Vector> {
      Vector values;
      values.append_range(arena.alloc, source).verify();
      values.append_range(arena.alloc, source).verify();
      cat::verify(values.size() == 6u);
      cat::verify(values[4u] == 2_i4);
      values.free(arena.alloc);
   };
   verify_manual.template operator()<cat::vec<int4>>();
   verify_manual.template
   operator()<cat::vec<int4, cat::vec_flags::pointer_size_layout>>();
   verify_manual.template operator()<cat::small_vec<int4>>();
   verify_manual.template
   operator()<cat::small_vec<int4, 4u, cat::vec_flags::pointer_size_layout>>();

   auto verify_raii = [&]<typename Vector> {
      Vector values(cat::dyn_allocator(arena.alloc));
      values.append_range(source).verify();
      values.append_range(source).verify();
      cat::verify(values.size() == 6u);
      cat::verify(values[4u] == 2_i4);
   };
   verify_raii.template operator()<cat::raii::vec<int4>>();
   verify_raii.template operator()<cat::raii::vec<
      int4, cat::dyn_allocator, cat::vec_flags::pointer_size_layout>>();
   verify_raii.template operator()<cat::raii::small_vec<int4>>();
   verify_raii.template operator()<cat::raii::small_vec<
      int4, cat::dyn_allocator, 4u, cat::vec_flags::pointer_size_layout>>();
}

$test(vec_iterable_range_modifiers) {
   linear_arena arena;
   cat::array<int4, 4u> source{1_i4, 2_i4, 3_i4, 4_i4};
   cat::raii::vec<int4, cat::linear_allocator> values(arena.alloc);

   auto evens = cat::ref(source).filter([](int4 value) -> bool {
      return value % 2_i4 == 0_i4;
   });
   values.append_range(evens).verify();
   cat::verify(values.size() == 2u);
   cat::verify(values[0u] == 2_i4);
   cat::verify(values[1u] == 4_i4);

   values.insert_range(1u, source).verify();
   cat::verify(values.size() == 6u);
   cat::verify(values[0u] == 2_i4);
   cat::verify(values[1u] == 1_i4);
   cat::verify(values[4u] == 4_i4);
   cat::verify(values[5u] == 4_i4);

   auto greater_than_two = cat::ref(source).filter([](int4 value) -> bool {
      return value > 2_i4;
   });
   values.replace_with_range(1u, 5u, greater_than_two).verify();
   cat::verify(values.size() == 4u);
   cat::verify(values[0u] == 2_i4);
   cat::verify(values[1u] == 3_i4);
   cat::verify(values[2u] == 4_i4);
   cat::verify(values[3u] == 4_i4);
}

$test(vec_contiguous_range_modifiers) {
   linear_arena arena;
   cat::array<int4, 3u> source{1_i4, 2_i4, 3_i4};
   cat::array<int4, 2u> insertion{4_i4, 5_i4};
   cat::array<int4, 1u> replacement{6_i4};
   cat::raii::vec<int4, cat::linear_allocator> values(arena.alloc);

   values.append_range(source).verify();
   values.insert_range(1u, insertion).verify();
   values.replace_with_range(2u, 4u, replacement).verify();

   cat::verify(values.size() == 4u);
   cat::verify(values[0u] == 1_i4);
   cat::verify(values[1u] == 4_i4);
   cat::verify(values[2u] == 6_i4);
   cat::verify(values[3u] == 3_i4);
}

$test(vec_iterator_typedefs) {
   using test_vec = cat::vec<int4>;
   using iterator = test_vec::iterator;
   using const_iterator = test_vec::const_iterator;
   using reverse_iterator = test_vec::reverse_iterator;
   using const_reverse_iterator = test_vec::const_reverse_iterator;

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

$test(vec_consteval_reserve_preserves_values) {
   static_assert(vec_consteval_reserve_preserves_values());
}

$test(vec_consteval_growth_preserves_values) {
   static_assert(vec_consteval_growth_preserves_values());
}

$test(vec_consteval_resize_lifetimes) {
   static_assert(vec_consteval_resize_lifetimes());
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

$test(vec_push_back_after_pop_restarts_lifetime) {
   linear_arena arena;
   lifetime_dead_assignment_count = 0u;
   cat::vec<lifetime_probe> values;
   values.push_back(arena.alloc, lifetime_probe(1_i4)).verify();
   auto popped = values.pop_back().verify();

   values.push_back(arena.alloc, lifetime_probe(2_i4)).verify();
   cat::verify(lifetime_dead_assignment_count == 0u);
   cat::verify(values[0u].value == 2_i4);
   values.free(arena.alloc);
}

$test(vec_push_back_after_reset_restarts_lifetime) {
   linear_arena arena;
   lifetime_dead_assignment_count = 0u;
   cat::vec<lifetime_probe> values;
   values.push_back(arena.alloc, lifetime_probe(1_i4)).verify();
   values.reset();

   values.push_back(arena.alloc, lifetime_probe(2_i4)).verify();
   cat::verify(lifetime_dead_assignment_count == 0u);
   cat::verify(values[0u].value == 2_i4);
   values.free(arena.alloc);
}

$test(vec_push_back_after_resize_shrink_restarts_lifetime) {
   linear_arena arena;
   lifetime_dead_assignment_count = 0u;
   cat::vec<lifetime_probe> values;
   values.push_back(arena.alloc, lifetime_probe(1_i4)).verify();
   values.push_back(arena.alloc, lifetime_probe(2_i4)).verify();
   values.resize(arena.alloc, 1u).verify();

   values.push_back(arena.alloc, lifetime_probe(3_i4)).verify();
   cat::verify(lifetime_dead_assignment_count == 0u);
   cat::verify(values[1u].value == 3_i4);
   values.free(arena.alloc);
}

$test(vec_relocates_trivial_elements_with_copy_memory) {
   linear_arena arena;
   cat::vec<relocation_only_probe> values;
   values.resize(arena.alloc, 2u).verify();
   values[0u].value = 11_i4;
   values[1u].value = 22_i4;

   relocation_move_count = 0u;
   relocation_only_probe* const p_before = values.data();
   values.reserve(arena.alloc, 16u).verify();

   cat::verify(relocation_move_count == 0u);
   cat::verify(values.data() != p_before);
   cat::verify(values[0u].value == 11_i4);
   cat::verify(values[1u].value == 22_i4);
   values.free(arena.alloc);
}

$test(vec_relocates_nontrivial_elements_individually) {
   linear_arena arena;
   lifetime_live_count = 0u;
   lifetime_destructor_count = 0u;
   cat::vec<lifetime_probe> values;
   values.emplace_back(arena.alloc, 11_i4).verify();
   values.emplace_back(arena.alloc, 22_i4).verify();
   idx const destructors_before = lifetime_destructor_count;

   values.reserve(arena.alloc, 16u).verify();

   cat::verify(lifetime_destructor_count == destructors_before + 2u);
   cat::verify(lifetime_live_count == 2u);
   cat::verify(values[0u].value == 11_i4);
   cat::verify(values[1u].value == 22_i4);
   values.free(arena.alloc);
   cat::verify(lifetime_live_count == 0u);
}

$test(vec_emplace_back_starts_element_lifetime) {
   linear_arena arena;
   lifetime_live_count = 0u;
   lifetime_destructor_count = 0u;
   cat::vec<lifetime_probe> values;
   values.reserve(arena.alloc, 4u).verify();
   idx const live_before = lifetime_live_count;
   idx const destructors_before = lifetime_destructor_count;

   values.emplace_back(arena.alloc, 9_i4).verify();
   cat::verify(lifetime_live_count == live_before + 1u);
   cat::verify(lifetime_destructor_count == destructors_before);
   values.free(arena.alloc);
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

$test(vec_compare_manual_and_raii) {
   linear_arena arena;
   cat::vec<int4> manual;
   cat::small_vec<int4, 4u> small;
   manual.push_back(arena.alloc, 1_i4).verify();
   manual.push_back(arena.alloc, 2_i4).verify();
   small.push_back(arena.alloc, 1_i4).verify();
   small.push_back(arena.alloc, 2_i4).verify();

   cat::raii::basic_vec<int4, cat::linear_allocator> linear(arena.alloc);
   cat::raii::vec<int4> dynamic{cat::dyn_allocator(pager)};
   linear.push_back(1_i4).verify();
   linear.push_back(2_i4).verify();
   dynamic.push_back(1_i4).verify();
   dynamic.push_back(2_i4).verify();

   cat::verify(manual == small);
   cat::verify(linear == dynamic);
   cat::verify(manual == linear);
   cat::verify((manual <=> dynamic) == 0);

   dynamic.push_back(3_i4).verify();
   cat::verify(linear != dynamic);
   cat::verify((manual <=> dynamic) < 0);
   small.free(arena.alloc);
   manual.free(arena.alloc);
}

$test(vec_make_factories) {
   linear_arena arena;

   cat::vec<int4> empty = cat::make_vec<int4>();
   cat::verify(empty.is_empty());
   cat::verify(empty.data() == nullptr);

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

$test(vec_noncontiguous_replace_failure_preserves_values) {
   linear_arena arena;
   cat::null_allocator null_alloc;
   cat::vec<int4> values =
      cat::make_vec<int4>(arena.alloc, {1_i4, 2_i4, 3_i4, 4_i4}).verify();
   cat::array<int4, 2u> replacements{8_i4, 9_i4};
   auto noncontiguous = cat::ref(replacements).filter([](int4) -> bool {
      return true;
   });

   auto const result =
      values.replace_with_range(null_alloc, 1u, 2u, noncontiguous);
   cat::verify(!result.has_value());
   cat::verify(values.size() == 4u);
   cat::verify(values[0u] == 1_i4);
   cat::verify(values[1u] == 2_i4);
   cat::verify(values[2u] == 3_i4);
   cat::verify(values[3u] == 4_i4);
   values.free(arena.alloc);
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

   cat::null_allocator null_alloc;
   int4* const p_before_failed_shrink = v.data();
   idx const capacity_before_failed_shrink = v.capacity();
   cat::verify(!v.shrink_to_fit(null_alloc).has_value());
   cat::verify(v.data() == p_before_failed_shrink);
   cat::verify(v.capacity() == capacity_before_failed_shrink);
   cat::verify(v.size() == 2);

   v.shrink_to_fit(arena.alloc).verify();
   cat::verify(v.size() == 2);
   cat::verify(v.capacity() == 2);
   cat::verify(v[0] == 21);
   cat::verify(v[1] == 22);

   // Idempotent when capacity already matches size.
   v.shrink_to_fit(arena.alloc).verify();
   cat::verify(v.capacity() == 2);

   // Shrinking an emptied vec releases its storage.
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

$test(vec_shrink_to_fit_minimum_allocation_static) {
   cat::vec<int4> v;
   $defer {
      v.free(pager);
   };
   v.reserve<cat::page_allocator>(pager, 2u).verify();
   v.push_back(pager, 1_i4).verify();

   int4* const p_before = v.data();
   idx const capacity_before = v.capacity();
   v.shrink_to_fit<cat::page_allocator>(pager).verify();

   cat::verify(v.data() == p_before);
   cat::verify(v.capacity() == capacity_before);
}

$test(vec_shrink_to_fit_minimum_allocation_dynamic) {
   cat::vec<int4> v;
   $defer {
      v.free(pager);
   };
   v.reserve(pager, 2u).verify();
   v.push_back(pager, 1_i4).verify();

   int4* const p_before = v.data();
   idx const capacity_before = v.capacity();
   v.shrink_to_fit(pager).verify();

   cat::verify(v.data() == p_before);
   cat::verify(v.capacity() == capacity_before);
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

$test(fixed_inline_vec_free_destroys_all_elements) {
   linear_arena arena;
   lifetime_destructor_count = 0u;
   cat::small_vec_fixed<lifetime_probe> values;
   values.initialize_fixed<cat::linear_allocator>(arena.alloc, 3u).verify();
   lifetime_destructor_count = 0u;

   values.free(arena.alloc);
   cat::verify(lifetime_destructor_count == 3u);
}

$test(fixed_inline_vec_cfree_destroys_all_elements) {
   linear_arena arena;
   lifetime_destructor_count = 0u;
   cat::small_vec_fixed<lifetime_probe> values;
   values.initialize_fixed<cat::linear_allocator>(arena.alloc, 3u).verify();
   lifetime_destructor_count = 0u;

   values.cfree(arena.alloc);
   cat::verify(lifetime_destructor_count == 3u);
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
