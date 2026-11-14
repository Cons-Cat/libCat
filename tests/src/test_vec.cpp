#include <cat/allocator_linear>
#include <cat/allocator_null>
#include <cat/allocator_page>
#include <cat/array>
#include <cat/iterable>
#include <cat/meta>
#include <cat/utility>
#include <cat/vec>

#include "../unit_tests.hpp"

namespace {
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

inline constinit idx destructor_count = 0u;

inline constinit idx lifetime_live_count = 0u;
inline constinit idx lifetime_destructor_count = 0u;
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

   constexpr constexpr_lifetime_probe(
      idx& live [[clang::lifetimebound]], int4 new_value
   )
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
   cat::vec_big<int> vector;
   auto _ = vector.resize<cat::allocator_page>(pager, 8);

   vector[0] = 1;
   vector[1] = 2;
   vector[7] = 10;
   auto _ = vector.push_back<cat::allocator_page>(pager, 10);
   int4 result = vector[8];
   vector.free<cat::allocator_page>(pager);
   return result;
}

consteval auto
vec_consteval_reserve_preserves_values() -> bool {
   cat::vec_big<int4> values;
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
   cat::vec_big<int4> values;
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
   cat::vec_big<constexpr_lifetime_probe> values;
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

// Boilerplate for tests that need a fast scratch arena.
struct linear_arena {
   cat::span<cat::byte> page =
      g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   cat::allocator_linear alloc = cat::make_allocator_linear(page);

   ~linear_arena() {
      g_tests_alloc.free(page);
   }
};

template <typename Vector>
void
verify_vec_niche() {
   static_assert(sizeof(cat::maybe<Vector>) == sizeof(Vector));
   cat::maybe<Vector> empty;
   cat::verify(empty.is_empty());
}

template <typename Vector>
concept can_change_vec_size =
   requires(Vector& vector, cat::allocator_linear& allocator) {
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
   verify_vec_niche<cat::vec_big<int4>>();
   verify_vec_niche<cat::vec<int4, cat::vec_flags::pointer_size_layout>>();
}

$test(raii_vec_maybe_niche) {
   verify_vec_niche<cat::raii::vec<int4>>();
   verify_vec_niche<cat::raii::vec<
      int4, cat::allocator_dyn, cat::vec_flags::pointer_size_layout>>();
   verify_vec_niche<cat::raii::vec_big<int4>>();
   verify_vec_niche<cat::raii::vec_big<
      int4, cat::allocator_dyn, cat::vec_flags::pointer_size_layout>>();

   linear_arena arena;
   auto engaged = cat::raii::make_vec<int4>(arena.alloc);
   cat::verify(engaged.has_value());
}

$test(vec_flags) {
   constexpr auto flags =
      cat::vec_flags::pointer_size_layout | cat::vec_flags::initial_growth(8u);
   static_assert(flags.uses_pointer_size_layout);
   static_assert(!flags.is_fixed_size);
   static_assert(flags.inline_storage_count == 0u);
   static_assert(flags.initial_growth_count == 8u);
   static_assert(cat::vec_flags{}.initial_growth_count == 0u);
   constexpr auto inline_flags =
      cat::vec_flags::pointer_size_layout | cat::vec_flags::inline_storage(4u);
   static_assert(inline_flags.inline_storage_count == 4u);
   static_assert(inline_flags.initial_growth_count == 0u);
   // The default inline capacity matches LLVM's `SmallVector<T>`.
   static_assert(cat::vec<char>::flags.inline_storage_count == 40u);
   static_assert(cat::vec<int2>::flags.inline_storage_count == 20u);
   static_assert(cat::vec<int4>::flags.inline_storage_count == 12u);
   static_assert(cat::vec<int8>::flags.inline_storage_count == 6u);
   static_assert(cat::vec<void*>::flags.inline_storage_count == 6u);
   static_assert(cat::vec<int4x4>::flags.inline_storage_count == 3u);
   static_assert(
      cat::vec<cat::array<int8, 6u>>::flags.inline_storage_count == 1u
   );
   static_assert(cat::vec_fixed<int4>::flags.inline_storage_count == 12u);
   // A `T` larger than 256 bytes gets no inline storage, like LLVM.
   static_assert(
      cat::is_same<
         cat::vec<cat::array<char, 257u>>, cat::vec_big<cat::array<char, 257u>>>
   );
   static_assert(
      cat::vec<cat::array<char, 256u>>::flags.inline_storage_count == 1u
   );
   static_assert(cat::is_same<
                 cat::raii::vec<int4>,
                 cat::raii::vec_small<int4, cat::allocator_dyn, 12u>>);
   static_assert(cat::vec<int4>::flags.initial_growth_count == 0u);
   static_assert(cat::vec_big<int4>::flags.inline_storage_count == 0u);
   static_assert(cat::vec_big<int4>::flags.initial_growth_count == 4u);
   static_assert(cat::vec_small<int4, 8u>::flags.inline_storage_count == 8u);
   static_assert(
      cat::is_same<
         cat::raii::vec_small<int4, cat::allocator_dyn, 8u>,
         cat::raii::basic_vec<
            int4, cat::allocator_dyn, cat::vec_flags::inline_storage(8u)>>
   );
   static_assert(
      cat::vec<int4, cat::vec_flags::inline_storage(8u)>::flags
         .inline_storage_count
      == 8u
   );

   linear_arena arena;
   cat::vec<int4, cat::vec_flags::initial_growth(8u)> values;
   values.push_back(arena.alloc, 1_i4).verify();
   cat::verify(values.capacity() == 8u);

   static_assert(sizeof(cat::vec_big<int4>) == sizeof(void*) * 3u);
   // `pointer_size_layout` stores `uint4` counts, like LLVM's `SmallVector`,
   // and holds at most that many elements.
   static_assert(
      sizeof(cat::vec_big<int4, cat::vec_flags::pointer_size_layout>)
      == sizeof(void*) * 2u
   );
   static_assert(
      cat::vec_big<int4, cat::vec_flags::pointer_size_layout>::max_size()
      == cat::uint4::max()
   );
   static_assert(
      sizeof(cat::vec_big<int4, cat::vec_flags::fixed_size>)
      == sizeof(void*) * 2u
   );
   static_assert(
      sizeof(cat::vec_big<
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
                 cat::vec_big_fixed<int4>,
                 cat::vec_big<int4, cat::vec_flags::fixed_size>>);
   static_assert(
      cat::is_same<
         cat::raii::vec_fixed<int4>,
         cat::raii::vec<int4, cat::allocator_dyn, cat::vec_flags::fixed_size>>
   );
   static_assert(cat::is_same<
                 cat::raii::vec_big_fixed<int4>,
                 cat::raii::vec_big<
                    int4, cat::allocator_dyn, cat::vec_flags::fixed_size>>);
   static_assert(!can_change_vec_size<cat::vec_fixed<int4>>);
   static_assert(!can_change_vec_size<cat::vec_big_fixed<int4>>);
   static_assert(!can_reserve_raii<cat::raii::vec_fixed<int4>>);
   static_assert(!can_reserve_raii<cat::raii::vec_big_fixed<int4>>);
}

$test(fixed_size_vec) {
   linear_arena arena;
   using fixed_vec = cat::vec_fixed<int4>;
   static_assert(!can_change_vec_size<fixed_vec>);

   fixed_vec values =
      cat::make_vec<int4, cat::allocator_linear, cat::vec_flags::fixed_size>(
         arena.alloc, {1_i4, 2_i4, 3_i4}
      )
         .verify();
   cat::verify(values.size() == 3u);
   cat::verify(values.capacity() == 3u);
   cat::verify(values[1u] == 2_i4);
   values.free(arena.alloc);

   cat::vec_small_fixed<int4, 4u> inline_values =
      cat::make_vec<
         int4, cat::allocator_linear,
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

   auto small = cat::make_vec_filled<int4>(arena.alloc, 3u, 1_i4).verify();
   auto fixed =
      cat::make_vec_fixed_filled<int4>(arena.alloc, 3u, 2_i4).verify();
   auto small_fixed =
      cat::make_vec_fixed_filled<int4>(arena.alloc, 3u, 3_i4).verify();

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
      cat::raii::make_vec_filled<int4>(arena.alloc, 3u, 1_i4).verify();
   auto raii_fixed =
      cat::raii::make_vec_fixed_filled<int4>(arena.alloc, 3u, 2_i4).verify();
   auto raii_small_fixed =
      cat::raii::make_vec_fixed_filled<int4>(arena.alloc, 3u, 3_i4).verify();
   auto ordered =
      cat::raii::make_vec_small_filled<int4, 8u, cat::allocator_linear>(
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

$test(vec_big_inline_storage) {
   linear_arena arena;
   idx const bytes_before = arena.alloc.bytes_used();

   cat::vec_small<int4, 8u> values;
   for (idx i = 0u; i < 8u; ++i) {
      values.push_back(arena.alloc, int4(i)).verify();
   }
   cat::verify(arena.alloc.bytes_used() == bytes_before);

   values.push_back(arena.alloc, 8_i4).verify();
   cat::verify(arena.alloc.bytes_used() > bytes_before);
   cat::verify(values.size() == 9u);
   values.free(arena.alloc);

   cat::raii::vec_small<int4, cat::allocator_linear, 8u> managed(arena.alloc);
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
   verify_manual.template operator()<cat::vec_big<int4>>();
   verify_manual.template operator()<cat::vec<
      int4, cat::vec_flags::inline_storage(4u)
               | cat::vec_flags::pointer_size_layout>>();

   auto verify_raii = [&]<typename Vector> {
      Vector values(cat::allocator_dyn(arena.alloc));
      values.append_range(source).verify();
      values.append_range(source).verify();
      cat::verify(values.size() == 6u);
      cat::verify(values[4u] == 2_i4);
   };
   verify_raii.template operator()<cat::raii::vec<int4>>();
   verify_raii.template operator()<cat::raii::vec<
      int4, cat::allocator_dyn, cat::vec_flags::pointer_size_layout>>();
   verify_raii.template operator()<cat::raii::vec_big<int4>>();
   verify_raii.template operator()<cat::raii::vec_big<
      int4, cat::allocator_dyn, cat::vec_flags::pointer_size_layout>>();
}

$test(vec_iterable_range_modifiers) {
   linear_arena arena;
   cat::array<int4, 4u> source{1_i4, 2_i4, 3_i4, 4_i4};
   cat::raii::vec<int4, cat::allocator_linear> values(arena.alloc);

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
   cat::raii::vec<int4, cat::allocator_linear> values(arena.alloc);

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

$test(vec_subscript_runtime_extent) {
   cat::vec<int4> heap;
   cat::vec<int4> const constant;
   idx runtime = 100u;

   static_assert(cat::is_same<decltype(heap[100u]), int4&>);
   static_assert(cat::is_same<decltype(constant[100u]), int4 const&>);
   static_assert(cat::is_same<decltype(cat::move(heap)[0u]), int4>);
   static_assert(cat::is_same<decltype(heap[runtime]), int4&>);
}

$test(vec_push_back) {
   linear_arena arena;
   cat::allocator_ref allocator_reference = arena.alloc;

   cat::vec_big<int4> v;
   $defer {
      v.free(arena.alloc);
   };

   // Each `push_back` flavor: a bare allocator, an `allocator_ref`,
   // an explicit-template-allocator dispatch.
   v.push_back(arena.alloc, 1_i4).verify();
   v.push_back(allocator_reference, 2_i4).verify();
   v.push_back<cat::allocator_linear>(arena.alloc, 3_i4).verify();
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

$test(vec_push_back_after_clear_restarts_lifetime) {
   linear_arena arena;
   lifetime_dead_assignment_count = 0u;
   cat::vec<lifetime_probe> values;
   values.push_back(arena.alloc, lifetime_probe(1_i4)).verify();
   values.clear();

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

$test(vec_clear_preserves_capacity) {
   linear_arena arena;
   cat::vec_big<int4> v;
   $defer {
      v.free(arena.alloc);
   };
   v.push_back(arena.alloc, 1_i4).verify();
   v.push_back(arena.alloc, 2_i4).verify();
   idx const cap_before = v.capacity();

   v.clear();
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() == cap_before);
}

$test(vec_resize) {
   linear_arena arena;
   cat::vec<int4> v;
   $defer {
      v.free(arena.alloc);
   };

   v.resize<cat::allocator_linear>(arena.alloc, 4).verify();
   cat::verify(v.size() == 4);
   cat::verify(v.capacity() >= 4);

   // Shrinking via `resize` keeps capacity.
   idx const cap_after_grow = v.capacity();
   v.resize(arena.alloc, 2).verify();
   cat::verify(v.size() == 2);
   cat::verify(v.capacity() == cap_after_grow);

   // Growing past capacity reallocates geometrically.
   v.resize(arena.alloc, cap_after_grow + 1u).verify();
   cat::verify(v.size() == cap_after_grow + 1u);
   cat::verify(v.capacity() >= cap_after_grow * 2u);
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
   cat::vec_small<int4, 4u> small;
   manual.push_back(arena.alloc, 1_i4).verify();
   manual.push_back(arena.alloc, 2_i4).verify();
   small.push_back(arena.alloc, 1_i4).verify();
   small.push_back(arena.alloc, 2_i4).verify();

   cat::raii::basic_vec<int4, cat::allocator_linear> linear(arena.alloc);
   cat::raii::vec<int4> dynamic{cat::allocator_dyn(g_tests_alloc)};
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
      cat::make_vec_reserved<int4, cat::allocator_linear>(arena.alloc, 7)
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
      cat::make_vec_filled<int4, cat::allocator_linear>(arena.alloc, 3, 9_i4)
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
      source.clone<cat::allocator_linear>(arena.alloc).verify();
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

$test(vec_allocator_null_failure) {
   cat::allocator_null null_alloc;
   cat::vec_big<int4> v;
   $defer {
      v.free(null_alloc);
   };
   cat::verify(v.reserve(null_alloc, 1).is_empty());
}

$test(vec_noncontiguous_replace_failure_preserves_values) {
   linear_arena arena;
   cat::allocator_null null_alloc;
   // The replacement must not fit inline, so that it has to allocate.
   cat::vec_small<int4, 4u> values =
      cat::make_vec<
         int4, cat::allocator_linear, cat::vec_flags::inline_storage(4u)>(
         arena.alloc, {1_i4, 2_i4, 3_i4, 4_i4}
      )
         .verify();
   cat::array<int4, 2u> replacements{8_i4, 9_i4};
   auto noncontiguous = cat::ref(replacements).filter([](int4) -> bool {
      return true;
   });

   auto const result =
      values.replace_with_range(null_alloc, 1u, 2u, noncontiguous);
   cat::verify(result.is_empty());
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
   cat::vec_big<int4> v;
   $defer {
      v.free(arena.alloc);
   };
   v.reserve(arena.alloc, 64).verify();
   v.push_back(arena.alloc, 21_i4).verify();
   v.push_back(arena.alloc, 22_i4).verify();
   cat::verify(v.capacity() >= 64);

   cat::allocator_null null_alloc;
   int4* const p_before_failed_shrink = v.data();
   idx const capacity_before_failed_shrink = v.capacity();
   cat::verify(v.shrink_to_fit(null_alloc).is_empty());
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
   v.clear();
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
   v.reserve<cat::allocator_page>(pager, 2u).verify();
   v.push_back(pager, 1_i4).verify();

   int4* const p_before = v.data();
   idx const capacity_before = v.capacity();
   v.shrink_to_fit<cat::allocator_page>(pager).verify();

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
   cat::vec_fixed<lifetime_probe> values;
   values.initialize_fixed<cat::allocator_linear>(arena.alloc, 3u).verify();
   lifetime_destructor_count = 0u;

   values.free(arena.alloc);
   cat::verify(lifetime_destructor_count == 3u);
}

$test(fixed_inline_vec_cfree_destroys_all_elements) {
   linear_arena arena;
   lifetime_destructor_count = 0u;
   cat::vec_fixed<lifetime_probe> values;
   values.initialize_fixed<cat::allocator_linear>(arena.alloc, 3u).verify();
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

$test(raii_vec_free_releases_storage) {
   linear_arena arena;
   cat::raii::vec v = cat::raii::make_vec<int4>(arena.alloc).verify();
   v.push_back(1_i4).verify();
   v.push_back(2_i4).verify();
   // `.free()` deallocates through the owned allocator.
   v.free();
   cat::verify(v.size() == 0);
   cat::verify(v.capacity() == 0);
}

$test(raii_vec_factories) {
   linear_arena arena;
   cat::raii::vec reserved =
      cat::raii::make_vec_reserved<int4>(arena.alloc, 6).verify();
   cat::verify(reserved.capacity() >= 6);

   cat::raii::vec static_reserved =
      cat::raii::make_vec_reserved<int4, cat::allocator_linear>(arena.alloc, 7)
         .verify();
   cat::verify(static_reserved.capacity() >= 7);

   cat::raii::vec filled =
      cat::raii::make_vec_filled(arena.alloc, 8, 1_i4).verify();
   cat::verify(filled.size() == 8);
   verify_all_ones(filled);

   cat::raii::vec static_filled =
      cat::raii::make_vec_filled<int4, cat::allocator_linear>(
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
      source.clone<cat::allocator_linear>(arena.alloc).verify();
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
   using raii_vec_t = cat::raii::vec<int4, cat::allocator_page>;
   static_assert(cat::is_random_access_collection<raii_vec_t>);
}

$test(raii_vec_shrink_to_fit) {
   linear_arena arena;
   cat::raii::vec<int4, cat::allocator_linear> v(arena.alloc);
   v.reserve(64).verify();
   v.push_back(31_i4).verify();
   v.push_back(32_i4).verify();
   cat::verify(v.capacity() >= 64);

   v.shrink_to_fit().verify();
   cat::verify(v.size() == 2);
   cat::verify(v.capacity() == cat::detail::default_inline_count<int4>());
   cat::verify(v[0] == 31);

   v.clear();
   v.shrink_to_fit().verify();
   cat::verify(v.capacity() == 0);
   cat::verify(v.data() == nullptr);
}

$test(raii_vec_element_destructors) {
   linear_arena arena;
   destructor_count = 0u;
   {
      cat::raii::vec<foo, cat::allocator_linear> raii_foos(arena.alloc);
      raii_foos.push_back(foo{}).verify();
      raii_foos.push_back(foo{}).verify();
      raii_foos.push_back(foo{}).verify();
   }
   cat::assert(destructor_count == 3);
}

$test(vec_collection) {
   using flux_test_vec = cat::vec<int>;
   static_assert(cat::is_random_access_collection<flux_test_vec>);

   auto vec_values = cat::make_vec_filled(g_tests_alloc, 3, 0).verify();
   $defer {
      vec_values.free(g_tests_alloc);
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
      cat::raii::make_vec_filled(g_tests_alloc, 3u, 0).verify();
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
   values.resize(g_tests_alloc, 65u, 0).verify();
   $defer {
      values.free(g_tests_alloc);
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
   wrapped_values.resize(g_tests_alloc, 65u, 0_i4).verify();
   $defer {
      wrapped_values.free(g_tests_alloc);
   };
   for (cat::idx i = 0u; i < wrapped_values.size(); ++i) {
      wrapped_values[i] = cat::int4(i);
   }

   wrapped_values | cat::reverse_inplace();
   for (cat::idx i = 0u; i < wrapped_values.size(); ++i) {
      cat::verify(wrapped_values[i] == cat::int4(64u - i));
   }
}

struct throwing_move_element {
   throwing_move_element() = default;
   throwing_move_element(throwing_move_element const&) = default;

   throwing_move_element(throwing_move_element&&) noexcept(false) {
   }

   auto
   operator=(throwing_move_element const&) -> throwing_move_element& = default;

   auto
   operator=(throwing_move_element&&) noexcept(false)
      -> throwing_move_element& {
      return *this;
   }
};

static_assert(cat::is_nothrow_default_constructible<cat::vec<int4>>);
// Moving a `vec` relocates through functions which assert, and an assert
// handler may throw, so it is only `noexcept` without exceptions.
static_assert(
   cat::is_nothrow_move_constructible<cat::vec_big<int4>>
   == !__has_feature(cxx_exceptions)
);
static_assert(
   cat::is_nothrow_move_constructible<cat::vec<int4>>
   == !__has_feature(cxx_exceptions)
);
static_assert(
   cat::is_nothrow_move_constructible<cat::vec_big<throwing_move_element>>
   == !__has_feature(cxx_exceptions)
);
static_assert(
   !cat::is_nothrow_move_constructible<cat::vec<throwing_move_element>>
);

#if __has_feature(cxx_exceptions)
// `push_back()` may be passed one of the `vec`'s own elements, which must
// survive reallocation.
$test(vec_push_back_self_reference) {
   cat::vec_big<int4> vector;
   for (int4 i = 0; i < 4; ++i) {
      cat::verify(vector.push_back(g_tests_alloc, i + 10).has_value());
   }
   cat::verify(vector.push_back(g_tests_alloc, vector[0]).has_value());
   cat::verify(vector[4] == 10);

   cat::verify(vector.resize(g_tests_alloc, 64u, vector[1]).has_value());
   cat::verify(vector[63] == 11);
   vector.free(g_tests_alloc);
}

#endif

namespace {
// A non-trivially-copyable element which records its own address, so that a
// bitwise copy or relocation of it is detectable.
struct self_pointer {
   int4 value;
   self_pointer const* p_self = this;

   self_pointer() : value(0) {
   }

   self_pointer(int4 input) : value(input) {
   }

   self_pointer(self_pointer const& other) : value(other.value) {
   }

   self_pointer(self_pointer&& other) : value(other.value) {
   }

   auto
   operator=(self_pointer const& other) -> self_pointer& {
      value = other.value;
      return *this;
   }

   auto
   operator=(self_pointer&& other) -> self_pointer& {
      value = other.value;
      return *this;
   }
};

// An aggregate is implicit-lifetime even when its members are not trivially
// copyable.
struct self_pointer_aggregate {
   self_pointer member;
};

static_assert(cat::is_implicit_lifetime<self_pointer_aggregate>);
static_assert(!cat::is_trivially_copyable<self_pointer_aggregate>);

// Like `std::vector`, a `vec` is `noexcept` to destroy regardless of `T`, so
// `T` may contain a `vec<T>`. Inline storage needs a complete `T`, so only
// heap-only `vec`s can be recursive.
struct recursive_vec_node {
   cat::vec_big<recursive_vec_node> children;
};

static_assert(cat::is_nothrow_destructible<recursive_vec_node>);

struct throwing_destructor_element {
   ~throwing_destructor_element() noexcept(false) {
   }
};

// Otherwise, the assertions below would be vacuous.
static_assert(!cat::is_nothrow_destructible<throwing_destructor_element>);

template <typename Vector>
constexpr bool is_nothrow_destroying =
   cat::is_nothrow_destructible<Vector>
   && noexcept(cat::declval<Vector&>().~Vector());

// Fixed-size `vec`s have no `.clear()`.
template <typename Vector>
constexpr bool is_nothrow_clearing =
   is_nothrow_destroying<Vector> && noexcept(cat::declval<Vector&>().clear());

using throwing = throwing_destructor_element;
static_assert(is_nothrow_clearing<cat::vec<throwing>>);
static_assert(is_nothrow_clearing<cat::vec_big<throwing>>);
static_assert(is_nothrow_clearing<cat::vec_small<throwing, 1u>>);
static_assert(is_nothrow_clearing<
              cat::vec_big<throwing, cat::vec_flags::pointer_size_layout>>);
static_assert(is_nothrow_destroying<cat::vec_fixed<throwing>>);
static_assert(is_nothrow_destroying<cat::vec_big_fixed<throwing>>);
static_assert(is_nothrow_destroying<cat::vec_small_fixed<throwing, 1u>>);
static_assert(is_nothrow_destroying<cat::raii::vec<throwing>>);
static_assert(is_nothrow_destroying<cat::raii::vec_big<throwing>>);
// `raii::vec::free()` frees through an allocator, which may assert, and an
// assert handler may throw.
static_assert(
   noexcept(cat::declval<cat::raii::vec<throwing>&>().free())
   == !__has_feature(cxx_exceptions)
);

template <typename Vector, cat::idx count>
void
verify_self_pointers(
   Vector const& vector, cat::array<int4, count> const& expected
) {
   cat::verify(vector.size() == count);
   for (idx i; i < count; ++i) {
      cat::verify(vector[i].value == expected[i]);
      cat::verify(vector[i].p_self == &vector[i]);
   }
}
}  // namespace

// Inserting more elements than follow the insertion point into spare capacity
// constructs both the new elements and the shifted tail past the end.
$test(vec_insert_range_past_end_with_spare_capacity) {
   cat::array<self_pointer, 3u> const range = {
      self_pointer(10), self_pointer(11), self_pointer(12)
   };
   cat::vec_big<self_pointer> vector;
   cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());
   cat::verify(vector.push_back(g_tests_alloc, self_pointer(0)).has_value());
   cat::verify(vector.push_back(g_tests_alloc, self_pointer(1)).has_value());

   cat::verify(vector.insert_range(g_tests_alloc, 1u, range).has_value());
   verify_self_pointers(vector, cat::array<int4, 5u>{0, 10, 11, 12, 1});

   // This assigns over one element, then inserts two before one element.
   cat::verify(
      vector.replace_with_range(g_tests_alloc, 3u, 4u, range).has_value()
   );
   verify_self_pointers(vector, cat::array<int4, 7u>{0, 10, 11, 10, 11, 12, 1});
   vector.free(g_tests_alloc);
}

// Aggregates which are not trivially copyable are copy-constructed.
$test(vec_append_range_copies_aggregates) {
   cat::array<self_pointer_aggregate, 2u> const range = {
      self_pointer_aggregate{self_pointer(1)},
      self_pointer_aggregate{self_pointer(2)}
   };
   cat::vec_big<self_pointer_aggregate> vector;
   cat::verify(vector.reserve(g_tests_alloc, 8u).has_value());
   cat::verify(vector.append_range(g_tests_alloc, range).has_value());
   cat::verify(vector.insert_range(g_tests_alloc, 0u, range).has_value());
   cat::verify(vector.size() == 4u);
   for (idx i; i < 4u; ++i) {
      cat::verify(vector[i].member.value == int4((i % 2u).raw + 1));
      cat::verify(vector[i].member.p_self == &vector[i].member);
   }
   vector.free(g_tests_alloc);
}

// Elements which fit in inline storage are constructed at their final
// address.
$test(vec_inline_storage_constructs_in_place) {
   cat::vec<self_pointer> vector;
   cat::verify(vector.emplace_back(g_tests_alloc, 1).has_value());
   cat::verify(vector.push_back(g_tests_alloc, self_pointer(2)).has_value());
   verify_self_pointers(vector, cat::array<int4, 2u>{1, 2});
   vector.free(g_tests_alloc);

   cat::verify(vector.resize(g_tests_alloc, 3u, self_pointer(3)).has_value());
   verify_self_pointers(vector, cat::array<int4, 3u>{3, 3, 3});
   vector.free(g_tests_alloc);
}

// Range modifiers may be passed part of the `vec` itself.
$test(vec_range_modifiers_alias_self) {
   cat::vec_big<int4> vector;
   do {
      cat::verify(
         vector.push_back(g_tests_alloc, int4(vector.size().raw)).has_value()
      );
   } while (vector.size() < vector.capacity());
   idx const old_size = vector.size();

   // This reallocates while reading the old storage.
   cat::verify(vector
                  .append_range(
                     g_tests_alloc,
                     cat::span<int4>(vector.data(), vector.size())
                  )
                  .has_value());
   cat::verify(vector.size() == old_size * 2u);
   for (idx i; i < vector.size(); ++i) {
      cat::verify(vector[i] == int4((i % old_size).raw));
   }

   vector.erase(4u, vector.size());
   cat::verify(vector.reserve(g_tests_alloc, 32u).has_value());
   cat::verify(vector
                  .insert_range(
                     g_tests_alloc, 0u, cat::span<int4>(vector.data() + 1, 2u)
                  )
                  .has_value());
   cat::array<int4, 6u> const inserted = {1, 2, 0, 1, 2, 3};
   cat::verify(vector.size() == 6u);
   for (idx i; i < 6u; ++i) {
      cat::verify(vector[i] == inserted[i]);
   }

   cat::verify(vector
                  .replace_with_range(
                     g_tests_alloc, 0u, 1u,
                     cat::span<int4>(vector.data() + 4, 2u)
                  )
                  .has_value());
   cat::array<int4, 7u> const replaced = {2, 3, 2, 0, 1, 2, 3};
   cat::verify(vector.size() == 7u);
   for (idx i; i < 7u; ++i) {
      cat::verify(vector[i] == replaced[i]);
   }
   vector.free(g_tests_alloc);
}

namespace {
template <typename Vector, cat::idx count>
void
verify_ints(Vector const& vector, cat::array<int4, count> const& expected) {
   cat::verify(vector.size() == count);
   for (idx i; i < count; ++i) {
      cat::verify(vector[i] == expected[i]);
   }
}

template <typename Vector>
void
fill_ints(Vector& vector, int4 count) {
   for (int4 i = 0; i < count; ++i) {
      cat::verify(vector.push_back(g_tests_alloc, i).has_value());
   }
}

inline constinit idx counted_destructions = 0u;

struct counted_destructor {
   int4 value;

   ~counted_destructor() {
      ++counted_destructions;
   }
};
}  // namespace

static_assert(cat::vec<int4>::takes_value_parameter);
static_assert(!cat::vec<self_pointer>::takes_value_parameter);

// Growing preserves every element, including when the new element is one of
// them.
$test(vec_growth_preserves_elements) {
   cat::vec_big<int4> vector;
   for (int4 i = 0; i < 4'096; ++i) {
      cat::verify(vector.push_back(g_tests_alloc, i).has_value());
   }
   for (idx i; i < vector.size(); ++i) {
      cat::verify(vector[i] == int4(i.raw));
   }

   while (vector.size() < vector.capacity()) {
      cat::verify(vector.push_back(g_tests_alloc, 0).has_value());
   }
   cat::verify(vector.emplace_back(g_tests_alloc, vector[7]).has_value());
   cat::verify(vector[idx(vector.size() - 1u)] == 7);

   cat::verify(
      vector.reserve(g_tests_alloc, vector.capacity() * 4u).has_value()
   );
   cat::verify(vector[4'095] == 4'095);
   vector.free(g_tests_alloc);
}

$test(vec_emplace_and_insert_single) {
   cat::vec_big<int4> vector;
   fill_ints(vector, 3);
   cat::verify(vector.emplace(g_tests_alloc, 1u, 10).has_value());
   cat::verify(vector.insert(g_tests_alloc, 0u, 20).has_value());
   cat::verify(vector.insert(g_tests_alloc, vector.size(), 30).has_value());
   verify_ints(vector, cat::array<int4, 6u>{20, 0, 10, 1, 2, 30});

   // The inserted value may be an element which the insertion shifts.
   cat::verify(vector.insert(g_tests_alloc, 1u, vector[4]).has_value());
   verify_ints(vector, cat::array<int4, 7u>{20, 2, 0, 10, 1, 2, 30});
   vector.free(g_tests_alloc);

   cat::vec_big<self_pointer> objects;
   cat::verify(objects.push_back(g_tests_alloc, self_pointer(0)).has_value());
   cat::verify(objects.push_back(g_tests_alloc, self_pointer(1)).has_value());
   cat::verify(objects.emplace(g_tests_alloc, 0u, objects[1]).has_value());
   cat::verify(objects.emplace(g_tests_alloc, 2u, 5).has_value());
   verify_self_pointers(objects, cat::array<int4, 4u>{1, 0, 5, 1});
   objects.free(g_tests_alloc);
}

$test(vec_insert_repeated_value) {
   cat::vec_big<int4> vector;
   fill_ints(vector, 4);
   cat::verify(vector.insert_n(g_tests_alloc, 1u, 2u, 9).has_value());
   verify_ints(vector, cat::array<int4, 6u>{0, 9, 9, 1, 2, 3});

   // An element of this `vec` is copied before the tail shifts.
   cat::verify(vector.insert_n(g_tests_alloc, 0u, 3u, vector[5]).has_value());
   verify_ints(vector, cat::array<int4, 9u>{3, 3, 3, 0, 9, 9, 1, 2, 3});
   vector.free(g_tests_alloc);

   cat::vec_big<self_pointer> objects;
   cat::verify(objects.reserve(g_tests_alloc, 16u).has_value());
   cat::verify(objects.push_back(g_tests_alloc, self_pointer(0)).has_value());
   cat::verify(objects.push_back(g_tests_alloc, self_pointer(1)).has_value());
   cat::verify(objects.insert_n(g_tests_alloc, 1u, 3u, objects[0]).has_value());
   verify_self_pointers(objects, cat::array<int4, 5u>{0, 0, 0, 0, 1});
   objects.free(g_tests_alloc);
}

$test(vec_append_and_assign_values) {
   cat::vec_big<int4> vector;
   fill_ints(vector, 2);
   cat::verify(vector.append(g_tests_alloc, 2u, vector[1]).has_value());
   verify_ints(vector, cat::array<int4, 4u>{0, 1, 1, 1});

   idx const capacity = vector.capacity();
   cat::verify(vector.assign(g_tests_alloc, 2u, 7).has_value());
   verify_ints(vector, cat::array<int4, 2u>{7, 7});
   cat::verify(vector.capacity() == capacity);

   cat::verify(vector.assign(g_tests_alloc, 3u, vector[0]).has_value());
   verify_ints(vector, cat::array<int4, 3u>{7, 7, 7});

   cat::verify(vector.assign(g_tests_alloc, capacity * 2u, 5).has_value());
   cat::verify(vector.size() == capacity * 2u);
   cat::verify(vector[capacity] == 5);

   cat::array<int4, 3u> const source = {1, 2, 3};
   cat::verify(vector.assign_range(g_tests_alloc, source).has_value());
   verify_ints(vector, source);

   // A range within the `vec` itself may be assigned.
   cat::verify(
      vector.assign_range(g_tests_alloc, cat::span<int4>(vector.data() + 1, 2u))
         .has_value()
   );
   verify_ints(vector, cat::array<int4, 2u>{2, 3});
   vector.free(g_tests_alloc);
}

$test(vec_resize_and_truncate) {
   cat::vec_big<int4> vector;
   fill_ints(vector, 3);
   cat::verify(vector.resize(g_tests_alloc, 64u).has_value());
   cat::verify(vector.size() == 64u);
   verify_ints(
      cat::span<int4>(vector.data(), 3u), cat::array<int4, 3u>{0, 1, 2}
   );
   vector.truncate(2u);
   verify_ints(vector, cat::array<int4, 2u>{0, 1});
   vector.free(g_tests_alloc);

   counted_destructions = 0u;
   cat::vec_big<counted_destructor> objects;
   for (int4 i = 0; i < 5; ++i) {
      cat::verify(objects.emplace_back(g_tests_alloc, i).has_value());
   }
   objects.pop_back_n(2u);
   cat::verify(counted_destructions == 2u);
   cat::verify(objects.size() == 3u);
   objects.truncate(1u);
   cat::verify(counted_destructions == 4u);
   cat::verify(objects[0].value == 0);
   objects.free(g_tests_alloc);
}

// Inserting or replacing with part of the `vec` itself needs no temporary
// storage when there is spare capacity.
$test(vec_aliased_ranges_do_not_allocate) {
   cat::allocator_null null_alloc;
   cat::vec_big<int4> vector;
   cat::verify(vector.reserve(g_tests_alloc, 32u).has_value());
   fill_ints(vector, 4);

   cat::verify(
      vector
         .insert_range(null_alloc, 1u, cat::span<int4>(vector.data() + 2, 2u))
         .has_value()
   );
   verify_ints(vector, cat::array<int4, 6u>{0, 2, 3, 1, 2, 3});

   cat::verify(vector
                  .replace_with_range(
                     null_alloc, 0u, 2u, cat::span<int4>(vector.data() + 3, 3u)
                  )
                  .has_value());
   verify_ints(vector, cat::array<int4, 7u>{1, 2, 3, 3, 1, 2, 3});
   vector.free(g_tests_alloc);

   cat::vec_big<self_pointer> objects;
   cat::verify(objects.reserve(g_tests_alloc, 16u).has_value());
   for (int4 i = 0; i < 3; ++i) {
      cat::verify(objects.emplace_back(g_tests_alloc, i).has_value());
   }
   cat::verify(objects
                  .insert_range(
                     null_alloc, 0u,
                     cat::span<self_pointer const>(objects.data() + 1, 2u)
                  )
                  .has_value());
   verify_self_pointers(objects, cat::array<int4, 5u>{1, 2, 0, 1, 2});
   objects.free(g_tests_alloc);
}

// Two inline `vec`s swap element-wise.
$test(vec_swap_inline) {
   cat::vec<self_pointer> left;
   cat::vec<self_pointer> right;
   for (int4 i = 0; i < 3; ++i) {
      cat::verify(left.emplace_back(g_tests_alloc, i).has_value());
   }
   cat::verify(right.emplace_back(g_tests_alloc, 10).has_value());

   left.swap(right);
   verify_self_pointers(left, cat::array<int4, 1u>{10});
   verify_self_pointers(right, cat::array<int4, 3u>{0, 1, 2});

   left.swap(right);
   verify_self_pointers(left, cat::array<int4, 3u>{0, 1, 2});
   verify_self_pointers(right, cat::array<int4, 1u>{10});
   left.free(g_tests_alloc);
   right.free(g_tests_alloc);
}

namespace {
// Read the value of either kind of element that these tests use.
constexpr auto
element_value(int4 value) -> int4 {
   return value;
}

constexpr auto
element_value(self_pointer const& value) -> int4 {
   return value.value;
}

template <typename Vector, cat::idx count>
constexpr auto
has_values(Vector const& vector, cat::array<int4, count> const& expected)
   -> bool {
   if (vector.size() != count) {
      return false;
   }
   for (idx i; i < count; ++i) {
      if (element_value(vector[i]) != expected[i]) {
         return false;
      }
   }
   return true;
}

template <typename Vector>
constexpr auto
has_intact_self_pointers(Vector const& vector) -> bool {
   if constexpr (
      cat::is_same<cat::remove_cvref<decltype(vector[0u])>, self_pointer>
   ) {
      for (idx i; i < vector.size(); ++i) {
         if (vector[i].p_self != &vector[i]) {
            return false;
         }
      }
   }
   return true;
}

// Exercise every modifier on a trivial element type in constant evaluation,
// where none of the `memcpy()`-based fast paths run.
template <typename Vector>
consteval auto
consteval_modifiers() -> bool {
   Vector values;
   bool result = true;
   for (int4 i = 0; i < 4; ++i) {
      result = result && values.push_back(pager, i).has_value();
   }

   cat::array<int4, 3u> const range = {10, 11, 12};
   result = result && values.insert_range(pager, 1u, range).has_value();
   result = result
            && has_values(values, cat::array<int4, 7u>{0, 10, 11, 12, 1, 2, 3});
   result = result && values.insert(pager, 0u, 20).has_value();
   result = result && values.emplace(pager, 2u, 30).has_value();
   result = result && values.insert_n(pager, 1u, 2u, values[3]).has_value();
   result =
      result
      && has_values(
         values, cat::array<int4, 11u>{20, 10, 10, 0, 30, 10, 11, 12, 1, 2, 3}
      );

   result = result
            && values
                  .replace_with_range(
                     pager, 0u, 3u, cat::span<int4>(values.data() + 8, 3u)
                  )
                  .has_value();
   result =
      result
      && has_values(
         values, cat::array<int4, 11u>{1, 2, 3, 0, 30, 10, 11, 12, 1, 2, 3}
      );
   values.erase(4u);
   values.erase(0u, 3u);
   result = result
            && has_values(values, cat::array<int4, 7u>{0, 10, 11, 12, 1, 2, 3});
   result =
      result
      && values.insert_range(pager, 2u, cat::span<int4>(values.data() + 4, 3u))
            .has_value();
   result = result
            && has_values(
               values, cat::array<int4, 10u>{0, 10, 1, 2, 3, 11, 12, 1, 2, 3}
            );
   result =
      result && values.replace_with_range(pager, 1u, 9u, range).has_value();
   result =
      result && has_values(values, cat::array<int4, 5u>{0, 10, 11, 12, 3});

   result = result && values.append(pager, 2u, values[4]).has_value();
   values.truncate(5u);
   values.pop_back_n(1u);
   result = result && has_values(values, cat::array<int4, 4u>{0, 10, 11, 12});
   result = result && values.assign(pager, 3u, values[1]).has_value();
   result = result && has_values(values, cat::array<int4, 3u>{10, 10, 10});
   result = result && values.assign_range(pager, range).has_value();
   result =
      result
      && values.assign_range(pager, cat::span<int4>(values.data() + 1, 2u))
            .has_value();
   result = result && has_values(values, cat::array<int4, 2u>{11, 12});

   auto maybe_copy = values.clone(pager);
   result = result && maybe_copy.has_value();
   auto copy = cat::move(maybe_copy).value();
   result = result && values.push_back(pager, 13).has_value();
   values.swap(copy);
   result = result && has_values(values, cat::array<int4, 2u>{11, 12});
   result = result && has_values(copy, cat::array<int4, 3u>{11, 12, 13});
   values.free(pager);
   copy.free(pager);
   return result;
}

static_assert(consteval_modifiers<cat::vec_big<int4>>());

// Every element built during constant evaluation is destroyed exactly once.
consteval auto
consteval_modifier_lifetimes() -> bool {
   idx live = 0u;
   constexpr_lifetime_probe const value(live, 1);
   bool result = true;
   {
      cat::vec_big<constexpr_lifetime_probe> values;
      result = result && values.resize(pager, 3u, value).has_value();
      result = result && values.insert_n(pager, 1u, 2u, value).has_value();
      result = result && live == values.size() + 1u;
      values.erase(0u, 2u);
      result = result && live == values.size() + 1u;
      cat::array<constexpr_lifetime_probe, 2u> const range = {value, value};
      result =
         result && values.replace_with_range(pager, 0u, 1u, range).has_value();
      result =
         result
         && values
               .insert_range(
                  pager, 0u,
                  cat::span<constexpr_lifetime_probe>(values.data() + 1, 2u)
               )
               .has_value();
      result = result && live == values.size() + 3u;
      auto maybe_copy = values.clone(pager);
      result = result && maybe_copy.has_value();
      auto copy = cat::move(maybe_copy).value();
      result = result && live == values.size() * 2u + 3u;
      copy.free(pager);
      values.free(pager);
      result = result && live == 3u;
   }
   return result && live == 1u;
}

static_assert(consteval_modifier_lifetimes());

// Pass elements of the `vec` itself to every modifier, both while there is
// spare capacity and while growing, including from inline storage.
template <typename Element, typename Vector>
void
exercise_self_references() {
   Vector vector;
   cat::verify(vector.emplace_back(g_tests_alloc, 0).has_value());
   cat::verify(vector.emplace_back(g_tests_alloc, 1).has_value());

   // There is spare capacity here.
   cat::verify(vector.push_back(g_tests_alloc, vector[0]).has_value());
   cat::verify(vector.emplace_back(g_tests_alloc, vector[1]).has_value());
   cat::verify(has_values(vector, cat::array<int4, 4u>{0, 1, 0, 1}));
   while (vector.size() < vector.capacity()) {
      cat::verify(vector.emplace_back(g_tests_alloc, vector[1]).has_value());
   }

   // These grow.
   cat::verify(vector.push_back(g_tests_alloc, vector[1]).has_value());
   cat::verify(
      vector.insert(g_tests_alloc, 0u, cat::move(vector[4])).has_value()
   );
   cat::verify(element_value(vector[0]) == 1);
   cat::verify(has_values(
      cat::span<Element>(vector.data() + 1, 4u),
      cat::array<int4, 4u>{0, 1, 0, 1}
   ));
   while (vector.size() < vector.capacity()) {
      cat::verify(vector.emplace_back(g_tests_alloc, vector[1]).has_value());
   }
   idx const full = vector.size();
   cat::verify(vector.emplace_back(g_tests_alloc, vector[0]).has_value());
   cat::verify(element_value(vector[full]) == 1);
   vector.truncate(6u);

   cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());
   cat::verify(vector.insert_n(g_tests_alloc, 2u, 3u, vector[1]).has_value());
   cat::verify(
      has_values(vector, cat::array<int4, 9u>{1, 0, 0, 0, 0, 1, 0, 1, 1})
   );
   cat::verify(vector
                  .insert_range(
                     g_tests_alloc, 1u,
                     cat::span<Element>(vector.data() + 4, 3u)
                  )
                  .has_value());
   cat::verify(has_values(
      vector, cat::array<int4, 12u>{1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1}
   ));
   cat::verify(vector.insert(g_tests_alloc, 0u, vector[11]).has_value());
   cat::verify(element_value(vector[0]) == 1);
   cat::verify(has_intact_self_pointers(vector));

   // These grow with an element of the `vec` as the source.
   int4 const second = element_value(vector[1]);
   idx const spare = idx(vector.capacity() - vector.size());
   cat::verify(vector.append(g_tests_alloc, spare + 1u, vector[1]).has_value());
   cat::verify(element_value(vector[idx(vector.size() - 1u)]) == second);
   int4 const first = element_value(vector[0]);
   cat::verify(vector.resize(g_tests_alloc, vector.capacity() + 1u, vector[0])
                  .has_value());
   cat::verify(element_value(vector[idx(vector.size() - 1u)]) == first);
   int4 const last = element_value(vector[idx(vector.size() - 1u)]);
   cat::verify(vector
                  .assign(
                     g_tests_alloc, vector.capacity() + 1u,
                     vector[idx(vector.size() - 1u)]
                  )
                  .has_value());
   cat::verify(element_value(vector[0]) == last);
   cat::verify(element_value(vector[idx(vector.size() - 1u)]) == last);
   cat::verify(has_intact_self_pointers(vector));
   vector.free(g_tests_alloc);
}

struct move_only {
   int4 value;

   move_only(int4 input) : value(input) {
   }

   move_only(move_only&& other) : value(other.value) {
      other.value = -1;
   }

   auto
   operator=(move_only&& other) -> move_only& {
      value = other.value;
      other.value = -1;
      return *this;
   }

   move_only(move_only const&) = delete;
   auto
   operator=(move_only const&) -> move_only& = delete;
};

// Elements which can be moved but never assigned.
struct construct_only {
   int4 value;

   construct_only(int4 input) : value(input) {
   }

   construct_only(construct_only&& other) : value(other.value) {
   }

   auto
   operator=(construct_only&&) -> construct_only& = delete;
   auto
   operator=(construct_only const&) -> construct_only& = delete;
};

template <typename Vector>
void
exercise_move_only() {
   Vector vector;
   for (int4 i = 0; i < 10; ++i) {
      cat::verify(vector.emplace_back(g_tests_alloc, i).has_value());
   }
   cat::verify(vector.insert(g_tests_alloc, 0u, move_only(20)).has_value());
   cat::verify(vector.emplace(g_tests_alloc, 5u, 21).has_value());
   cat::verify(vector[0].value == 20);
   cat::verify(vector[5].value == 21);
   cat::verify(vector[11].value == 9);

   vector.erase(0u);
   vector.erase(3u, 5u);
   cat::verify(vector.size() == 9u);
   cat::verify(vector[2].value == 2);
   cat::verify(vector[3].value == 4);
   cat::verify(vector.pop_back().verify().value == 9);
   cat::verify(vector.size() == 8u);
   vector.free(g_tests_alloc);
}

template <typename Vector>
void
exercise_construct_only() {
   Vector vector;
   for (int4 i = 0; i < 10; ++i) {
      cat::verify(vector.emplace_back(g_tests_alloc, i).has_value());
   }
   for (int4 i = 0; i < 10; ++i) {
      cat::verify(vector[idx(i)].value == i);
   }
   cat::verify(vector.pop_back().verify().value == 9);
   vector.truncate(3u);

   Vector other;
   cat::verify(other.emplace_back(g_tests_alloc, 7).has_value());
   vector.swap(other);
   cat::verify(vector.size() == 1u && vector[0].value == 7);
   cat::verify(other.size() == 3u && other[2].value == 2);
   vector.free(g_tests_alloc);
   other.free(g_tests_alloc);
}

inline constinit cat::array<int4, 16u> destruction_order = {};
inline constinit idx destruction_count = 0u;

struct ordered_destructor {
   int4 value;

   ~ordered_destructor() {
      destruction_order[destruction_count] = value;
      ++destruction_count;
   }
};
}  // namespace

$test(vec_consteval_modifiers) {
   cat::verify(consteval_modifiers<cat::vec_big<int4>>());
}

$test(vec_self_references) {
   exercise_self_references<int4, cat::vec<int4>>();
   exercise_self_references<int4, cat::vec_big<int4>>();
   exercise_self_references<self_pointer, cat::vec<self_pointer>>();
   exercise_self_references<self_pointer, cat::vec_big<self_pointer>>();
}

$test(vec_move_only_elements) {
   exercise_move_only<cat::vec<move_only>>();
   exercise_move_only<cat::vec_big<move_only>>();
   exercise_construct_only<cat::vec<construct_only>>();
   exercise_construct_only<cat::vec_big<construct_only>>();
}

// Growing and inserting into a `vec` of inline `vec`s relocates each inner
// `vec`'s elements.
$test(vec_of_inline_vecs) {
   cat::vec_big<cat::vec<int4>> outer;
   for (int4 i = 0; i < 20; ++i) {
      cat::vec<int4> inner;
      cat::verify(inner.push_back(g_tests_alloc, i).has_value());
      cat::verify(inner.push_back(g_tests_alloc, i * 2).has_value());
      cat::verify(outer.push_back(g_tests_alloc, cat::move(inner)).has_value());
   }

   cat::vec<int4> front;
   cat::verify(front.push_back(g_tests_alloc, -1).has_value());
   cat::verify(outer.insert(g_tests_alloc, 0u, cat::move(front)).has_value());
   cat::verify(outer.size() == 21u);
   cat::verify(outer[0].size() == 1u && outer[0][0] == -1);
   for (idx i = 1u; i < outer.size(); ++i) {
      int4 const value = int4((i - 1u).raw);
      cat::verify(outer[i].size() == 2u);
      cat::verify(outer[i][0] == value && outer[i][1] == value * 2);
      cat::verify(outer[i].data() != nullptr);
   }

   for (auto& inner : outer) {
      inner.free(g_tests_alloc);
   }
   outer.free(g_tests_alloc);
}

// A moved-from `vec` with inline storage can store elements again without
// allocating.
$test(vec_moved_from_reuses_inline_storage) {
   cat::allocator_null null_alloc;
   cat::vec<int4> source;
   fill_ints(source, 3);
   cat::vec<int4> destination(cat::move(source));
   verify_ints(destination, cat::array<int4, 3u>{0, 1, 2});
   cat::verify(source.size() == 0u);
   cat::verify(source.push_back(null_alloc, 7).has_value());
   verify_ints(source, cat::array<int4, 1u>{7});

   // A `vec` which had moved to the heap returns to inline storage.
   cat::vec<int4> heap;
   fill_ints(heap, 16);
   cat::vec<int4> taken(cat::move(heap));
   cat::verify(heap.push_back(null_alloc, 8).has_value());
   verify_ints(heap, cat::array<int4, 1u>{8});

   source.free(g_tests_alloc);
   destination.free(g_tests_alloc);
   heap.free(g_tests_alloc);
   taken.free(g_tests_alloc);
}

$test(vec_empty_and_end_edge_cases) {
   cat::vec_big<int4> vector;
   cat::span<int4> const empty{};
   vector.erase(0u, 0u);
   vector.truncate(0u);
   vector.pop_back_n(0u);
   cat::verify(vector.pop_back().is_empty());
   cat::verify(vector.insert_range(g_tests_alloc, 0u, empty).has_value());
   cat::verify(vector.insert_n(g_tests_alloc, 0u, 0u, 5).has_value());
   cat::verify(
      vector.replace_with_range(g_tests_alloc, 0u, 0u, empty).has_value()
   );
   cat::verify(vector.append_range(g_tests_alloc, empty).has_value());
   cat::verify(vector.append(g_tests_alloc, 0u, 5).has_value());
   cat::verify(vector.assign(g_tests_alloc, 0u, 5).has_value());
   cat::verify(vector.resize(g_tests_alloc, 0u).has_value());
   cat::vec_big<int4> other;
   vector.swap(other);
   cat::verify(vector.size() == 0u && vector.capacity() == 0u);
   cat::verify(vector.data() == nullptr);
   cat::vec<int4> inline_left;
   cat::vec<int4> inline_right;
   inline_left.swap(inline_right);
   cat::verify(inline_left.size() == 0u && inline_right.size() == 0u);

   fill_ints(vector, 4);
   cat::array<int4, 2u> const tail = {7, 8};
   cat::verify(
      vector.insert_range(g_tests_alloc, vector.size(), tail).has_value()
   );
   cat::verify(
      vector.insert_n(g_tests_alloc, vector.size(), 2u, 9).has_value()
   );
   cat::verify(vector.insert_range(g_tests_alloc, 2u, empty).has_value());
   cat::verify(vector.insert_n(g_tests_alloc, 2u, 0u, 5).has_value());
   verify_ints(vector, cat::array<int4, 8u>{0, 1, 2, 3, 7, 8, 9, 9});
   vector.erase(idx(vector.size() - 1u));
   vector.erase(2u, 2u);
   vector.erase(vector.size(), vector.size());
   verify_ints(vector, cat::array<int4, 7u>{0, 1, 2, 3, 7, 8, 9});
   idx const capacity = vector.capacity();
   vector.erase(0u, vector.size());
   cat::verify(vector.size() == 0u && vector.capacity() == capacity);
   vector.free(g_tests_alloc);
}

// A single-pass range which fails partway leaves every element that fit.
$test(vec_single_pass_range_failure) {
   cat::allocator_null null_alloc;
   cat::array<int4, 4u> source = {10, 11, 12, 13};
   auto single_pass = cat::ref(source).filter([](int4) -> bool {
      return true;
   });

   cat::vec_big<int4> vector;
   cat::verify(vector.reserve(g_tests_alloc, 4u).has_value());
   idx const capacity = vector.capacity();
   fill_ints(vector, int4((capacity - 2u).raw));
   cat::verify(vector.append_range(null_alloc, single_pass).is_empty());
   cat::verify(vector.size() == capacity);
   cat::verify(vector[idx(capacity - 2u)] == 10);
   cat::verify(vector[idx(capacity - 1u)] == 11);

   // Nothing fits, so nothing is inserted.
   cat::verify(vector.insert_range(null_alloc, 0u, single_pass).is_empty());
   cat::verify(vector.size() == capacity && vector[0] == 0);

   // What fits is inserted at the position, in order.
   vector.truncate(idx(capacity - 2u));
   cat::verify(vector.insert_range(null_alloc, 1u, single_pass).is_empty());
   cat::verify(vector.size() == capacity);
   cat::verify(vector[0] == 0 && vector[1] == 10 && vector[2] == 11);
   cat::verify(vector[idx(capacity - 1u)] == int4((capacity - 3u).raw));
   vector.free(g_tests_alloc);
}

// Every modifier destroys each element exactly once.
$test(vec_destroys_each_element_once) {
   idx live = 0u;
   constexpr_lifetime_probe const value(live, 1);
   cat::array<constexpr_lifetime_probe, 3u> const range = {value, value, value};
   idx const outside = live;

   cat::vec_big<constexpr_lifetime_probe> vector;
   cat::verify(vector.resize(g_tests_alloc, 6u, value).has_value());
   vector.erase(1u);
   vector.erase(0u, 2u);
   cat::verify(live == vector.size() + outside);
   cat::verify(
      vector.replace_with_range(g_tests_alloc, 0u, 2u, range).has_value()
   );
   cat::verify(live == vector.size() + outside);
   cat::verify(vector
                  .replace_with_range(
                     g_tests_alloc, 0u, 4u, cat::span(range).subspan(0u, 1u)
                  )
                  .has_value());
   cat::verify(live == vector.size() + outside);
   cat::verify(vector
                  .insert_range(
                     g_tests_alloc, 1u,
                     cat::span<constexpr_lifetime_probe>(vector.data(), 2u)
                  )
                  .has_value());
   cat::verify(live == vector.size() + outside);
   cat::verify(vector.assign(g_tests_alloc, 12u, value).has_value());
   cat::verify(live == vector.size() + outside);
   vector.truncate(5u);
   cat::verify(live == vector.size() + outside);
   vector.clear();
   cat::verify(live == outside);
   cat::verify(vector.resize(g_tests_alloc, 3u, value).has_value());
   vector.free(g_tests_alloc);
   cat::verify(live == outside);
}

// Elements are destroyed in reverse order, like `std::vector`.
$test(vec_destroys_in_reverse_order) {
   cat::vec_big<ordered_destructor> vector;
   cat::verify(vector.reserve(g_tests_alloc, 8u).has_value());
   for (int4 i = 0; i < 6; ++i) {
      cat::verify(vector.emplace_back(g_tests_alloc, i).has_value());
   }
   destruction_count = 0u;
   vector.truncate(3u);
   cat::verify(destruction_count == 3u);
   cat::verify(destruction_order[0] == 5 && destruction_order[1] == 4);
   cat::verify(destruction_order[2] == 3);
   vector.free(g_tests_alloc);
   cat::verify(destruction_count == 6u);
   cat::verify(destruction_order[3] == 2 && destruction_order[4] == 1);
   cat::verify(destruction_order[5] == 0);
}

#if __has_feature(cxx_exceptions)
#include "../injected_failure.hpp"

namespace {
using allocator_type = __typeof_unqual(g_tests_alloc);

template <typename Vector>
void
fill_vec(Vector& vector, int4 count) {
   for (int4 i = 0; i < count; ++i) {
      cat::verify(vector.emplace_back(g_tests_alloc, i).has_value());
   }
}

// Fill a `vec` with `0, 1, 2...` until it has no spare capacity.
template <typename Vector>
auto
fill_vec_to_capacity(Vector& vector) -> int4 {
   fill_vec(vector, 1);
   while (vector.size() < vector.capacity()) {
      cat::verify(
         vector.emplace_back(g_tests_alloc, int4(vector.size().raw)).has_value()
      );
   }
   return int4(vector.size().raw);
}

template <typename Vector>
void
verify_vec_sequence(Vector const& vector, int4 count) {
   cat::verify(vector.size() == idx(count));
   for (int4 i = 0; i < count; ++i) {
      cat::verify(vector[idx(i)].value == i);
   }
}
}  // namespace

// Reallocation copies throwing-move elements, so a throwing copy leaves the
// original elements untouched.
$test(vec_reallocation_strong_guarantee) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   int4 const count = fill_vec_to_capacity(vector);
   idx const capacity = vector.capacity();
   cat::verify(vector.size() == capacity);

   fragile::arm(2);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.emplace_back(g_tests_alloc, count);
   }));
   verify_vec_sequence(vector, count);
   cat::verify(vector.capacity() == capacity);
   cat::verify(fragile::live == iword(count));

   vector.free(g_tests_alloc);
   cat::verify(fragile::live == 0);
}

$test(vec_inline_reallocation_strong_guarantee) {
   fragile::live = 0;
   cat::vec<fragile> vector;
   fill_vec(vector, 4);

   fragile::arm(3);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.reserve(g_tests_alloc, 16u);
   }));
   verify_vec_sequence(vector, 4);
   cat::verify(fragile::live == 4);

   vector.free(g_tests_alloc);
   cat::verify(fragile::live == 0);
}

$test(vec_emplace_back_throwing_constructor) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   fill_vec(vector, 2);
   fragile const source(7);

   fragile::arm(1);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.push_back(g_tests_alloc, source);
   }));
   verify_vec_sequence(vector, 2);
   cat::verify(fragile::live == 3);

   vector.free(g_tests_alloc);
}

$test(vec_resize_strong_guarantee) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   fill_vec(vector, 3);
   cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());
   fragile const source(9);

   fragile::arm(4);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.resize(g_tests_alloc, 10u, source);
   }));
   verify_vec_sequence(vector, 3);
   cat::verify(fragile::live == 4);

   vector.free(g_tests_alloc);
}

$test(vec_clone_throwing_copy) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   fill_vec(vector, 6);

   fragile::arm(4);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.clone(g_tests_alloc);
   }));
   cat::verify(fragile::live == 6);

   vector.free(g_tests_alloc);
}

// Insertion has the basic guarantee: every element that is not destroyed is
// still owned by the `vec`.
$test(vec_insert_range_basic_guarantee) {
   fragile::live = 0;
   cat::array<fragile, 3u> const range = {
      fragile(100), fragile(101), fragile(102)
   };

   // Positions past `2u` insert more elements than follow them.
   for (idx position = 0u; position <= 5u; ++position) {
      for (iword failure = 1; failure <= 8; ++failure) {
         cat::vec_big<fragile> vector;
         fill_vec(vector, 5);
         cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());

         fragile::arm(failure);
         bool const threw = throws_injected_failure([&] {
            auto _ = vector.insert_range(g_tests_alloc, position, range);
         });
         fragile::disarm();
         cat::verify(fragile::live == iword(vector.size()) + 3);
         if (!threw) {
            cat::verify(vector.size() == 8u);
            for (idx i; i < vector.size(); ++i) {
               int4 const expected = i < position ? int4(i.raw)
                                     : i < position + 3u
                                        ? int4(100 + (i - position).raw)
                                        : int4((i - 3u).raw);
               cat::verify(vector[i].value == expected);
            }
         }

         vector.free(g_tests_alloc);
         cat::verify(fragile::live == 3);
      }
   }
}

$test(vec_replace_with_range_basic_guarantee) {
   fragile::live = 0;
   cat::array<fragile, 4u> const range = {
      fragile(100), fragile(101), fragile(102), fragile(103)
   };

   for (iword failure = 1; failure <= 8; ++failure) {
      cat::vec_big<fragile> vector;
      fill_vec(vector, 5);
      cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());

      fragile::arm(failure);
      throws_injected_failure([&] {
         auto _ = vector.replace_with_range(g_tests_alloc, 1u, 2u, range);
      });
      fragile::disarm();
      cat::verify(fragile::live == iword(vector.size()) + 4);

      vector.free(g_tests_alloc);
      cat::verify(fragile::live == 4);
   }
}

// Inserting into a full `vec` reallocates around the new elements, so a throw
// anywhere leaks nothing, and succeeding preserves element order.
$test(vec_insert_range_reallocating) {
   fragile::live = 0;
   cat::array<fragile, 3u> const range = {
      fragile(100), fragile(101), fragile(102)
   };

   for (idx position = 0u; position <= 4u; ++position) {
      for (iword failure = 1; failure <= 10; ++failure) {
         cat::vec_big<fragile> vector;
         idx const old_size = idx(fill_vec_to_capacity(vector));

         fragile::arm(failure);
         bool const threw = throws_injected_failure([&] {
            auto _ = vector.insert_range(g_tests_alloc, position, range);
         });
         fragile::disarm();
         cat::verify(fragile::live == iword(vector.size()) + 3);
         if (!threw) {
            cat::verify(vector.size() == old_size + 3u);
            for (idx i; i < vector.size(); ++i) {
               int4 const expected = i < position ? int4(i.raw)
                                     : i < position + 3u
                                        ? int4(100 + (i - position).raw)
                                        : int4((i - 3u).raw);
               cat::verify(vector[i].value == expected);
            }
         }
         vector.free(g_tests_alloc);
         cat::verify(fragile::live == 3);
      }
   }
}

// If relocating inline elements throws partway through a swap, the heap
// storage on the other side is not lost.
$test(vec_swap_throwing_relocation) {
   fragile::live = 0;
   cat::vec<fragile> small;
   fill_vec(small, 2);
   cat::vec<fragile> big;
   fill_vec(big, 8);

   fragile::arm(1);
   cat::verify(throws_injected_failure([&] {
      big.swap(small);
   }));
   fragile::disarm();
   verify_vec_sequence(big, 8);
   cat::verify(fragile::live == 8 + iword(small.size()));

   big.free(g_tests_alloc);
   small.free(g_tests_alloc);
   cat::verify(fragile::live == 0);
}

$test(vec_make_filled_throwing_copy) {
   fragile::live = 0;
   fragile const value(5);

   fragile::arm(4);
   cat::verify(throws_injected_failure([&] {
      auto _ = cat::make_vec_big_filled<fragile, allocator_type>(
         g_tests_alloc, 10u, value
      );
   }));
   fragile::arm(4);
   cat::verify(throws_injected_failure([&] {
      auto _ = cat::make_vec_big_fixed_filled<fragile, allocator_type>(
         g_tests_alloc, 10u, value
      );
   }));
   fragile::disarm();
   cat::verify(fragile::live == 1);
}

// Inserting copies of a value has the basic guarantee at every position.
$test(vec_insert_repeated_basic_guarantee) {
   fragile::live = 0;
   fragile const value(100);
   for (idx position = 0u; position <= 5u; ++position) {
      for (iword failure = 1; failure <= 8; ++failure) {
         cat::vec_big<fragile> vector;
         fill_vec(vector, 5);
         cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());

         fragile::arm(failure);
         bool const threw = throws_injected_failure([&] {
            auto _ = vector.insert_n(g_tests_alloc, position, 3u, value);
         });
         fragile::disarm();
         cat::verify(fragile::live == iword(vector.size()) + 1);
         if (!threw) {
            cat::verify(vector.size() == 8u);
            for (idx i; i < vector.size(); ++i) {
               int4 const expected = i < position        ? int4(i.raw)
                                     : i < position + 3u ? int4(100)
                                                         : int4((i - 3u).raw);
               cat::verify(vector[i].value == expected);
            }
         }

         vector.free(g_tests_alloc);
         cat::verify(fragile::live == 1);
      }
   }
}

// Swapping inline `vec`s leaks nothing if relocating an element throws.
$test(vec_swap_inline_throwing_relocation) {
   fragile::live = 0;
   cat::vec<fragile> left;
   fill_vec(left, 3);
   cat::vec<fragile> right;
   fill_vec(right, 1);

   fragile::arm(2);
   throws_injected_failure([&] {
      left.swap(right);
   });
   fragile::disarm();
   cat::verify(fragile::live == iword(left.size() + right.size()));

   left.free(g_tests_alloc);
   right.free(g_tests_alloc);
   cat::verify(fragile::live == 0);
}
#endif
