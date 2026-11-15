#include <cat/allocator_linear>
#include <cat/allocator_null>
#include <cat/devec>
#include <cat/iterable>
#include <cat/random>
#include <cat/vec>

#include "../allocator_failing.hpp"
#include "../injected_failure.hpp"
#include "../unit_tests.hpp"

namespace {

inline constinit idx devec_live_count = 0u;

// An element that is not trivially relocatable, but moves without throwing,
// so that it slides element-wise. It checks that it is never used after being
// destroyed.
struct devec_probe {
   int4 value = 0;
   bool live = true;

   explicit devec_probe(int4 new_value) : value(new_value) {
      ++devec_live_count;
   }

   devec_probe(devec_probe const& other) : value(other.value) {
      cat::verify(other.live);
      ++devec_live_count;
   }

   devec_probe(devec_probe&& other) noexcept : value(other.value) {
      cat::verify(other.live);
      ++devec_live_count;
   }

   auto
   operator=(devec_probe const& other) -> devec_probe& {
      cat::verify(live && other.live);
      value = other.value;
      return *this;
   }

   auto
   operator=(devec_probe&& other) noexcept -> devec_probe& {
      cat::verify(live && other.live);
      value = other.value;
      return *this;
   }

   ~devec_probe() {
      cat::verify(live);
      live = false;
      devec_live_count = idx(devec_live_count - 1u);
   }
};

static_assert(!cat::is_trivially_relocatable<devec_probe>);

// A trivially copyable element that is too large to pass by value.
struct devec_wide {
   int4 value;
   cat::array<int4, 7u> padding;

   explicit constexpr devec_wide(int4 new_value) : value(new_value) {
   }
};

static_assert(cat::is_trivially_copyable<devec_wide>);
static_assert(!cat::devec<devec_wide>::takes_value_parameter);
static_assert(cat::devec<int4>::takes_value_parameter);

// An element which may only be relocated, never assigned or copied.
struct devec_relocatable {
   int4 value;

   explicit devec_relocatable(int4 new_value) : value(new_value) {
   }

   devec_relocatable(devec_relocatable&& other) : value(other.value) {
   }

   devec_relocatable(devec_relocatable const&) = delete;
   auto
   operator=(devec_relocatable&&) -> devec_relocatable& = delete;
};

// An over-aligned element.
struct alignas(64) devec_aligned {
   int4 value;

   explicit devec_aligned(int4 new_value) : value(new_value) {
   }
};

}  // namespace

template <>
inline constexpr bool cat::is_trivially_relocatable<devec_relocatable> = true;

namespace {

template <typename Element>
auto
make_element(int4 value) -> Element {
   return Element(value);
}

template <typename Element>
auto
element_value(Element const& element) -> int4 {
   if constexpr (cat::is_same<Element, int4>) {
      return element;
   } else if constexpr (cat::is_same<Element, int8>) {
      return cat::narrow_cast<int4>(element).value();
   } else {
      return element.value;
   }
}

// A small deterministic generator for randomized operations.
struct devec_lcg {
   cat::wrap_uint8 state = 0xc0decafeu;

   auto
   next(uint8 bound) -> uint8 {
      state = state * 6'364'136'223'846'793'005u + 1'442'695'040'888'963'407u;
      return uint8(state >> 33u) % bound;
   }

   auto
   index(idx bound) -> idx {
      return idx(next(uint8(bound)));
   }
};

// Check a `devec`'s layout, and that ASan only allows accessing its elements.
template <typename Element>
void
verify_devec_layout(cat::devec<Element> const& values) {
   cat::verify(
      values.capacity()
      == values.front_capacity() + values.size() + values.back_capacity()
   );
   if (values.data() == nullptr) {
      cat::verify(values.capacity() == 0u);
      return;
   }

#if __has_feature(address_sanitizer)
   // ASan can only poison whole granules of 8 bytes.
   if constexpr (sizeof(Element) % 8u == 0u) {
      Element const* p_storage = values.data() - values.front_capacity();
      for (idx i; i < values.capacity(); ++i) {
         bool const is_element = i >= values.front_capacity()
                                 && i < values.front_capacity() + values.size();
         bool const is_poisoned =
            cat::__asan_address_is_poisoned(p_storage + i) != 0;
         cat::verify(is_poisoned != is_element);
      }
   }
#endif
}

template <typename Element>
void
verify_devec_equals(
   cat::devec<Element> const& actual, cat::vec<int4> const& expected
) {
   cat::verify(actual.size() == expected.size());
   for (idx i; i < expected.size(); ++i) {
      cat::verify(element_value(actual[i]) == expected[i]);
   }

   verify_devec_layout(actual);
}

// Apply the same random operations to a `devec` and to a `vec` model. When
// `inject_failures`, allocations randomly fail, and a failed operation must
// leave the `devec` unchanged.
template <typename Element>
void
run_devec_model(idx operation_count, uint8 seed, bool inject_failures) {
   allocator_failing allocator;
   cat::devec<Element> actual;
   cat::vec<int4> expected;
   devec_lcg random{seed};

   for (idx i; i < operation_count; ++i) {
      int4 const value = cat::narrow_cast<int4>(i).value();
      if (inject_failures && random.next(4u) == 0u) {
         allocator.fail_after(idx(random.next(3u)));
      } else {
         allocator.never_fail();
      }

      allocator.can_grow = random.next(2u) == 0u;
      idx const size = expected.size();
      switch (random.next(17u).raw) {
         case 0:
            if (
               actual.push_back(allocator, make_element<Element>(value))
                  .has_value()
            ) {
               expected.push_back(g_tests_alloc, value).verify();
            }

            break;
         case 1:
            {
               Element const element = make_element<Element>(value);
               if (actual.push_back(allocator, element).has_value()) {
                  expected.push_back(g_tests_alloc, value).verify();
               }

               break;
            }
         case 2:
            if (
               actual.push_front(allocator, make_element<Element>(value))
                  .has_value()
            ) {
               expected.insert(g_tests_alloc, 0u, value).verify();
            }

            break;
         case 3:
            {
               Element const element = make_element<Element>(value);
               if (actual.push_front(allocator, element).has_value()) {
                  expected.insert(g_tests_alloc, 0u, value).verify();
               }

               break;
            }
         case 4:
            if (size > 0u) {
               cat::verify(
                  element_value(actual.pop_back().value()) == expected.back()
               );
               auto _ = expected.pop_back();
            } else {
               cat::verify(!actual.pop_back().has_value());
            }

            break;
         case 5:
            if (size > 0u) {
               cat::verify(
                  element_value(actual.pop_front().value()) == expected.front()
               );
               expected.erase(0u);
            } else {
               cat::verify(!actual.pop_front().has_value());
            }

            break;
         case 6:
            {
               idx const position = random.index(size + 1u);
               cat::maybe<Element&> const result =
                  actual.emplace(allocator, position, value);
               if (result.has_value()) {
                  cat::verify(&result.value() == &actual[position]);
                  expected.insert(g_tests_alloc, position, value).verify();
               }

               break;
            }
         case 7:
            if (size > 0u) {
               idx const start = random.index(size);
               idx const end = start + random.index(idx(size - start) + 1u);
               actual.erase(start, end);
               expected.erase(start, end);
            }

            break;
         case 8:
            if (size > 0u) {
               // Duplicate an element, which aliases the `devec`.
               idx const source = random.index(size);
               int4 const copied = expected[source];
               if (random.next(2u) == 0u) {
                  if (actual.push_back(allocator, actual[source]).has_value()) {
                     expected.push_back(g_tests_alloc, copied).verify();
                  }
               } else {
                  if (
                     actual.push_front(allocator, actual[source]).has_value()
                  ) {
                     expected.insert(g_tests_alloc, 0u, copied).verify();
                  }
               }
            }

            break;
         case 9:
            {
               cat::array<Element, 3u> const range = {
                  make_element<Element>(value),
                  make_element<Element>(value + 1),
                  make_element<Element>(value + 2),
               };

               cat::array<int4, 3u> const values = {
                  value, value + 1, value + 2
               };

               if (random.next(2u) == 0u) {
                  if (actual.append_range(allocator, range).has_value()) {
                     expected.append_range(g_tests_alloc, values).verify();
                  }
               } else {
                  if (actual.prepend_range(allocator, range).has_value()) {
                     expected.insert_range(g_tests_alloc, 0u, values).verify();
                  }
               }

               break;
            }
         case 10:
            if (size > 0u) {
               // Append or prepend a slice of this `devec`.
               idx const start = random.index(size);
               idx const end = start + random.index(idx(size - start) + 1u);
               cat::vec<int4> copied = expected.clone(g_tests_alloc).verify();
               cat::span<int4 const> const slice =
                  copied.subspan(start, end).verify();
               if (random.next(2u) == 0u) {
                  if (
                     actual
                        .append_range(
                           allocator, actual.subspan(start, end).verify()
                        )
                        .has_value()
                  ) {
                     expected.append_range(g_tests_alloc, slice).verify();
                  }
               } else {
                  if (
                     actual
                        .prepend_range(
                           allocator, actual.subspan(start, end).verify()
                        )
                        .has_value()
                  ) {
                     expected.insert_range(g_tests_alloc, 0u, slice).verify();
                  }
               }

               copied.free(g_tests_alloc);
            }

            break;
         case 11:
            {
               idx const new_size = random.index(idx(size * 2u) + 2u);
               if (size > 0u && random.next(2u) == 0u) {
                  // Fill with an element of this `devec`.
                  idx const source = random.index(size);
                  int4 const filled = expected[source];
                  if (
                     actual.resize(allocator, new_size, actual[source])
                        .has_value()
                  ) {
                     expected.resize(g_tests_alloc, new_size, filled).verify();
                  }
               } else {
                  if (
                     actual
                        .resize(
                           allocator, new_size, make_element<Element>(value)
                        )
                        .has_value()
                  ) {
                     expected.resize(g_tests_alloc, new_size, value).verify();
                  }
               }

               break;
            }
         case 12:
            {
               idx const front_count = random.index(size + 8u);
               idx const back_count = random.index(size + 8u);
               if (
                  actual.reserve(allocator, front_count, back_count).has_value()
               ) {
                  cat::verify(actual.front_capacity() >= front_count);
                  cat::verify(actual.back_capacity() >= back_count);
               }

               break;
            }
         case 13:
            if (random.next(4u) == 0u) {
               if (actual.shrink_to_fit(allocator).has_value()) {
                  cat::verify(actual.capacity() == actual.size());
               }
            }

            break;
         case 14:
            if (random.next(16u) == 0u) {
               actual.clear();
               expected.clear();
            }

            break;
         case 15:
            {
               idx const position = random.index(size + 1u);
               Element const element = make_element<Element>(value);
               if (actual.insert(allocator, position, element).has_value()) {
                  expected.insert(g_tests_alloc, position, value).verify();
               }

               break;
            }
         default:
            {
               cat::maybe<cat::devec<Element>> copy = actual.clone(allocator);
               if (copy.has_value()) {
                  verify_devec_equals(copy.value(), expected);
                  cat::verify(
                     copy.value().front_capacity() == actual.front_capacity()
                  );
                  copy.value().free(allocator);
               }

               break;
            }
      }

      verify_devec_equals(actual, expected);
   }

   allocator.never_fail();
   actual.free(allocator);
   expected.free(g_tests_alloc);
   cat::verify(allocator.live_allocations == 0u);
}

consteval auto
devec_consteval_both_ends() -> bool {
   cat::devec<int4> values;
   for (idx i; i < 9u; ++i) {
      auto _ = values.push_back(pager, int4(i));
      auto _ = values.push_front(pager, -int4(i));
   }

   bool result =
      values.size() == 18u && values.front() == -8_i4 && values.back() == 8_i4;
   auto _ = values.emplace(pager, 9u, 100_i4);
   values.erase(0u, 4u);
   result = result && values.size() == 15u && values[5u] == 100_i4;
   values.free(pager);
   return result;
}

// Sliding during constant evaluation moves element-wise in both directions.
consteval auto
devec_consteval_slides() -> bool {
   cat::devec<int4> values;
   for (idx i; i < 8u; ++i) {
      auto _ = values.push_back(pager, int4(i));
   }

   bool result = true;
   for (idx i = 8u; i < 200u; ++i) {
      auto _ = values.push_back(pager, int4(i));
      result = result && values.pop_front().value() == int4(i - 8u);
   }

   for (idx i; i < 200u; ++i) {
      auto _ = values.push_front(pager, int4(i));
      auto _ = values.pop_back();
   }

   result = result && values.size() == 8u && values.front() == 199_i4;
   auto copy = values.clone(pager).value();
   result = result && copy == values;
   auto _ = values.shrink_to_fit(pager);
   result = result && values.capacity() == 8u;
   copy.free(pager);
   values.free(pager);
   return result;
}

static_assert(devec_consteval_both_ends());
static_assert(devec_consteval_slides());
static_assert(cat::is_nothrow_default_constructible<cat::devec<int4>>);
static_assert(cat::is_nothrow_move_constructible<cat::devec<devec_probe>>);
static_assert(cat::is_nothrow_destructible<cat::devec<devec_probe>>);

// Operations which may only fail through the assert handler are `noexcept`
// without exceptions. The rest never throw.
constexpr bool is_without_exceptions = !__has_feature(cxx_exceptions);
static_assert(
   noexcept(
      cat::declval<cat::devec<int4>&>() = cat::declval<cat::devec<int4>>()
   )
   == is_without_exceptions
);
static_assert(noexcept(cat::declval<cat::devec<int4>&>()
                          .swap(cat::declval<cat::devec<int4>&>())));
static_assert(noexcept(cat::declval<cat::devec<devec_probe>&>().clear()));
static_assert(noexcept(cat::declval<cat::devec<int4>&>().data()));
static_assert(noexcept(cat::declval<cat::devec<int4> const&>().size()));
static_assert(noexcept(cat::declval<cat::devec<int4> const&>().capacity()));
static_assert(
   noexcept(cat::declval<cat::devec<int4> const&>().front_capacity())
);
static_assert(
   noexcept(cat::declval<cat::devec<int4>&>()
               .free(cat::declval<cat::allocator_dyn>()))
   == is_without_exceptions
);
static_assert(!noexcept(cat::declval<cat::devec<int4>&>().erase(0u)));
static_assert(
   !noexcept(cat::declval<cat::devec<int4>&>().push_back(g_tests_alloc, 1))
);
static_assert(!noexcept(cat::declval<cat::devec<devec_probe>&>().pop_back()));
static_assert(cat::is_nothrow_move_constructible<cat::raii::devec<int4>>);
static_assert(
   noexcept(cat::declval<cat::raii::devec<int4>&>().free())
   == is_without_exceptions
);
static_assert(
   noexcept(cat::declval<cat::raii::devec<int4>&>()
               .swap(cat::declval<cat::raii::devec<int4>&>()))
   == is_without_exceptions
);
static_assert(noexcept(cat::declval<cat::raii::devec<int4>&>().clear()));
static_assert(!cat::is_copy_constructible<cat::devec<int4>>);
static_assert(cat::is_random_access_collection<cat::devec<int4>>);
static_assert(cat::is_contiguous_iterable<cat::devec<int4>>);
static_assert(cat::devec<char>::max_size() == idx::max());
static_assert(cat::devec<int4>::max_size() == idx::max() / 4u);

}  // namespace

$test(devec_empty) {
   allocator_failing allocator;
   cat::devec<int4> values;
   cat::devec<int4> const& constant = values;
   cat::verify(values.data() == nullptr && constant.data() == nullptr);
   cat::verify(values.is_empty() && values.size() == 0u);
   cat::verify(values.capacity() == 0u);
   cat::verify(values.front_capacity() == 0u && values.back_capacity() == 0u);
   cat::verify(values.begin() == values.end());
   cat::verify(!values.pop_back().has_value());
   cat::verify(!values.pop_front().has_value());
   cat::verify(!values.at(0u).has_value());
   cat::verify((cat::ref(values) | cat::sum()) == 0_i4);
   cat::verify(values == cat::devec<int4>());

   values.erase(0u, 0u);
   values.clear();
   values.reserve(allocator, 0u, 0u).verify();
   values.shrink_to_fit(allocator).verify();
   cat::devec<int4> copy = values.clone(allocator).verify();
   cat::verify(copy.data() == nullptr);
   values.append_range(allocator, cat::span<int4 const>()).verify();
   values.prepend_range(allocator, cat::span<int4 const>()).verify();
   values.resize(allocator, 0u).verify();
   values.resize(allocator, 0u, 1_i4).verify();
   cat::verify(allocator.allocation_count == 0u);
   values.free(allocator);
   verify_devec_layout(values);
}

$test(devec_move_and_swap) {
   cat::devec<int4> values =
      cat::make_devec<int4>(g_tests_alloc, {1_i4, 2_i4}).verify();
   values.push_front(g_tests_alloc, 0_i4).verify();
   int4 const* p_data = values.data();
   idx const front_capacity = values.front_capacity();

   cat::devec<int4> moved = move(values);
   cat::verify(values.data() == nullptr && values.capacity() == 0u);
   cat::verify(moved.data() == p_data && moved.size() == 3u);
   cat::verify(moved.front_capacity() == front_capacity);

   values = move(moved);
   cat::verify(moved.data() == nullptr && values.data() == p_data);

   // Self-move-assignment is a no-op.
   cat::devec<int4>& alias = values;
   values = move(alias);
   cat::verify(values.data() == p_data && values.size() == 3u);

   cat::devec<int4> other =
      cat::make_devec<int4>(g_tests_alloc, {9_i4}).verify();
   values.swap(other);
   cat::verify(values.size() == 1u && values[0u] == 9_i4);
   cat::verify(other.size() == 3u && other.data() == p_data);
   cat::verify(other.front_capacity() == front_capacity);

   values.free(g_tests_alloc);
   other.free(g_tests_alloc);
}

$test(devec_comparisons) {
   cat::devec<int4> low =
      cat::make_devec<int4>(g_tests_alloc, {1_i4, 2_i4}).verify();
   cat::devec<int4> high =
      cat::make_devec<int4>(g_tests_alloc, {1_i4, 3_i4}).verify();
   cat::devec<int4> same;
   same.push_front(g_tests_alloc, 2_i4).verify();
   same.push_front(g_tests_alloc, 1_i4).verify();

   // Equality ignores where the elements sit in their storage.
   cat::verify(low == same && !(low != same));
   cat::verify(low != high);
   cat::verify(low < high && high > low && low <= same && high >= low);

   // A `raii::devec` compares with a manual one in either order.
   cat::raii::devec<int4> managed =
      cat::raii::make_devec<int4>(g_tests_alloc, {1_i4, 2_i4}).verify();
   cat::verify(managed == low && low == managed);
   cat::verify(managed < high && high > managed && low <= managed);

   low.free(g_tests_alloc);
   high.free(g_tests_alloc);
   same.free(g_tests_alloc);
}

$test(devec_accessors) {
   cat::devec<int4> values =
      cat::make_devec<int4>(g_tests_alloc, {1_i4, 2_i4, 3_i4, 4_i4}).verify();
   cat::devec<int4> const& constant = values;
   cat::verify(constant.front() == 1_i4 && constant.back() == 4_i4);
   cat::verify(constant[2u] == 3_i4 && constant.at(3u).value() == 4_i4);
   cat::verify(!constant.at(4u).has_value());
   cat::verify(values.first(2u).verify().size() == 2u);
   cat::verify(values.last(1u).verify()[0u] == 4_i4);
   cat::verify(values.subspan(1u, 3u).verify()[0u] == 2_i4);
   values.at(0u).value() = 10_i4;
   values.back() = 40_i4;
   cat::verify(values.front() == 10_i4 && values[3u] == 40_i4);

   // Stepanov iterators.
   int4 total = 0;
   for (int4 value : constant) {
      total += value;
   }

   cat::verify(total == 55_i4);
   cat::verify(*values.rbegin() == 40_i4);
   cat::verify(values.end() - values.begin() == 4);
   cat::verify(*(values.cbegin() + 1) == 2_i4);

   // Iterables.
   cat::verify(
      (cat::ref(values) | cat::reverse() | cat::take(2u) | cat::sum()) == 43_i4
   );
   cat::verify((values.slice(1u, 3u) | cat::sum()) == 5_i4);
   cat::verify(values.as_span().size() == 4u);
   values.free(g_tests_alloc);
}

$test(devec_push_back_grows_like_vec) {
   cat::devec<int4> values;
   cat::vec_big<int4> reference;
   for (idx i; i < 1'000u; ++i) {
      values.push_back(g_tests_alloc, int4(i)).verify();
      reference.push_back(g_tests_alloc, int4(i)).verify();
      // Growing at the back never gives slots to the front.
      cat::verify(values.front_capacity() == 0u);
      cat::verify(values.capacity() <= max(idx(2u) * values.size(), 16u));
   }

   cat::verify(
      cat::span<int4 const>(values) == cat::span<int4 const>(reference)
   );
   values.free(g_tests_alloc);
   reference.free(g_tests_alloc);
}

$test(devec_push_front_grows_at_front) {
   cat::devec<int4> values;
   for (idx i; i < 1'000u; ++i) {
      values.push_front(g_tests_alloc, int4(i)).verify();
      cat::verify(values.back_capacity() == 0u);
      cat::verify(values.capacity() <= max(idx(2u) * values.size(), 16u));
   }

   for (idx i; i < 1'000u; ++i) {
      cat::verify(values[i] == int4(999u - i));
   }

   values.free(g_tests_alloc);
}

// A bump allocator which can extend its last allocation in place.
class devec_bump_allocator
    : public cat::allocator_interface<devec_bump_allocator> {
 public:
   static constexpr cat::ualign min_alignment = 16u;

   cat::span<cat::byte> page =
      g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   idx used;
   cat::byte* _Nullable p_last = nullptr;

   devec_bump_allocator() = default;
   devec_bump_allocator(devec_bump_allocator const&) = delete;

   ~devec_bump_allocator() {
      g_tests_alloc.free(page);
   }

   auto
   allocate(idx bytes) -> void* _Nullable {
      idx const start = (used + 15u) / 16u * 16u;
      if (start + bytes > page.size()) {
         return nullptr;
      }

      p_last = page.data() + start;
      used = start + bytes;
      return p_last;
   }

   void
   deallocate(void const* _Nullable, idx) {
   }

   auto
   grow(cat::span<cat::byte> allocation, idx new_bytes) -> cat::maybe<void> {
      if (
         allocation.data() != p_last
         || p_last + new_bytes > page.data() + page.size()
      ) {
         return cat::nullopt;
      }

      used = idx(p_last - page.data()) + new_bytes;
      return cat::monostate;
   }
};

$test(devec_grows_back_in_place) {
   devec_bump_allocator bump;
   cat::devec<int4> values;
   values.push_back<devec_bump_allocator>(bump, 0_i4).verify();
   int4 const* p_data = values.data();
   for (idx i = 1u; i < 200u; ++i) {
      values.push_back<devec_bump_allocator>(bump, int4(i)).verify();
   }

   // Growing at the back extended the storage instead of moving it.
   cat::verify(values.data() == p_data);
   cat::verify(values.capacity() >= 200u);
   for (idx i; i < 200u; ++i) {
      cat::verify(values[i] == int4(i));
   }

   verify_devec_layout(values);

   // Growing at the front cannot extend in place.
   values.push_front<devec_bump_allocator>(bump, -1_i4).verify();
   cat::verify(values.data() != p_data);
   values.free<devec_bump_allocator>(bump);

   // Without in-place growth, the storage moves.
   allocator_failing allocator;
   allocator.can_grow = false;
   cat::devec<int4> moved;
   moved.push_back(allocator, 0_i4).verify();
   p_data = moved.data();
   for (idx i = 1u; i < 200u; ++i) {
      moved.push_back(allocator, int4(i)).verify();
   }

   cat::verify(moved.data() != p_data);
   cat::verify(allocator.growth_count == 0u);
   moved.free(allocator);
   cat::verify(allocator.live_allocations == 0u);
}

template <typename Element>
void
verify_queue_slides() {
   allocator_failing allocator;
   cat::devec<Element> queue;
   for (idx i; i < 100u; ++i) {
      queue.push_back(allocator, make_element<Element>(int4(i))).verify();
   }

   // Warm up until the storage stops growing.
   for (idx i = 100u; i < 1'000u; ++i) {
      queue.push_back(allocator, make_element<Element>(int4(i))).verify();
      auto _ = queue.pop_front();
   }

   idx const capacity = queue.capacity();
   idx const allocation_count = allocator.allocation_count;
   for (idx i = 1'000u; i < 20'000u; ++i) {
      queue.push_back(allocator, make_element<Element>(int4(i))).verify();
      cat::verify(element_value(queue.pop_front().value()) == int4(i - 100u));
      cat::verify(queue.capacity() == capacity);
   }

   verify_devec_layout(queue);

   // The same holds at the other end.
   for (idx i; i < 20'000u; ++i) {
      queue.push_front(allocator, make_element<Element>(int4(i))).verify();
      auto _ = queue.pop_back();
      cat::verify(queue.capacity() == capacity);
   }

   cat::verify(allocator.allocation_count == allocation_count);
   cat::verify(capacity <= 256u);
   queue.free(allocator);
   cat::verify(allocator.live_allocations == 0u);
}

$test(devec_queue_slides_without_allocating) {
   verify_queue_slides<int4>();
   verify_queue_slides<int8>();
   verify_queue_slides<devec_probe>();
   verify_queue_slides<devec_wide>();
   cat::verify(devec_live_count == 0u);
}

$test(devec_slides_overlapping_and_disjoint) {
   // A large front capacity lets the elements slide past themselves.
   cat::devec<devec_probe> values;
   values.reserve(g_tests_alloc, 64u, 0u).verify();
   for (idx i; i < 4u; ++i) {
      values.emplace_back(g_tests_alloc, int4(i)).verify();
   }

   idx const capacity = values.capacity();
   values.emplace_back(g_tests_alloc, 4_i4).verify();
   cat::verify(values.capacity() == capacity);
   for (idx i; i < 5u; ++i) {
      cat::verify(values[i].value == int4(i));
   }

   values.free(g_tests_alloc);

   cat::devec<devec_probe> reversed;
   reversed.reserve(g_tests_alloc, 0u, 64u).verify();
   for (idx i; i < 4u; ++i) {
      reversed.emplace_front(g_tests_alloc, int4(i)).verify();
   }

   reversed.emplace_front(g_tests_alloc, 4_i4).verify();
   for (idx i; i < 5u; ++i) {
      cat::verify(reversed[i].value == int4(4u - i));
   }

   reversed.free(g_tests_alloc);
   cat::verify(devec_live_count == 0u);
}

$test(devec_alternating_ends_stays_bounded) {
   cat::devec<int4> values;
   for (idx i; i < 10'000u; ++i) {
      if (i % 2u == 0u) {
         values.push_back(g_tests_alloc, int4(i)).verify();
      } else {
         values.push_front(g_tests_alloc, int4(i)).verify();
      }

      cat::verify(values.capacity() <= max(idx(2u) * values.size(), 16u));
   }

   cat::verify(values.front() == 9'999_i4);
   cat::verify(values.back() == 9'998_i4);
   values.free(g_tests_alloc);
}

$test(devec_randomized) {
   run_devec_model<int4>(20'000u, 1u, false);
   run_devec_model<int8>(20'000u, 2u, false);
   run_devec_model<devec_probe>(20'000u, 3u, false);
   run_devec_model<devec_wide>(20'000u, 4u, false);
   cat::verify(devec_live_count == 0u);
}

$test(devec_randomized_allocation_failure) {
   run_devec_model<int4>(20'000u, 5u, true);
   run_devec_model<int8>(20'000u, 6u, true);
   run_devec_model<devec_probe>(20'000u, 7u, true);
   run_devec_model<devec_wide>(20'000u, 8u, true);
   cat::verify(devec_live_count == 0u);
}

$test(devec_aliasing_insertions) {
   cat::devec<devec_probe> values;
   for (idx i; i < 8u; ++i) {
      values.emplace_back(g_tests_alloc, int4(i)).verify();
   }

   // These fill the storage, which forces reallocations and slides while the
   // argument refers to an element.
   for (idx i; i < 64u; ++i) {
      values.push_back(g_tests_alloc, values.front()).verify();
      values.push_front(g_tests_alloc, values.back()).verify();
      cat::verify(values.back().value == values[1u].value);
   }

   for (idx i; i < 64u; ++i) {
      values.insert(g_tests_alloc, i, values[idx(values.size() - 1u)]).verify();
      values.emplace(g_tests_alloc, idx(values.size() - i), values[0u])
         .verify();
   }

   cat::verify(values.size() == 264u);
   values.free(g_tests_alloc);
   cat::verify(devec_live_count == 0u);
}

$test(devec_emplace_returns_element) {
   cat::devec<int4> values =
      cat::make_devec<int4>(g_tests_alloc, {0_i4, 1_i4, 2_i4, 3_i4}).verify();
   int4& front = values.emplace_front(g_tests_alloc, -1).verify();
   cat::verify(&front == &values[0u]);
   int4& back = values.emplace_back(g_tests_alloc, 4).verify();
   cat::verify(&back == &values[5u]);

   // The shorter side shifts.
   values.reserve(g_tests_alloc, 4u, 4u).verify();
   int4 const* p_data = values.data();
   values.emplace(g_tests_alloc, 1u, 50).verify();
   cat::verify(values.data() == p_data - 1u);
   values.emplace(g_tests_alloc, idx(values.size() - 1u), 60).verify();
   cat::verify(values.data() == p_data - 1u);

   cat::array<int4, 8u> const expected = {-1_i4, 50_i4, 0_i4,  1_i4,
                                          2_i4,  3_i4,  60_i4, 4_i4};
   cat::verify(
      cat::span<int4 const>(values) == cat::span<int4 const>(expected)
   );

   // Positions at the ends delegate to the end insertions.
   values.emplace(g_tests_alloc, 0u, 100).verify();
   values.emplace(g_tests_alloc, values.size(), 200).verify();
   cat::verify(values.front() == 100_i4 && values.back() == 200_i4);
   values.free(g_tests_alloc);
}

template <typename Element>
void
verify_devec_erase() {
   for (idx start; start <= 8u; ++start) {
      for (idx end = start; end <= 8u; ++end) {
         cat::devec<Element> values;
         for (idx i; i < 8u; ++i) {
            values.push_back(g_tests_alloc, make_element<Element>(int4(i)))
               .verify();
         }

         Element const* p_data = values.data();
         idx const capacity = values.capacity();

         values.erase(start, end);
         cat::verify(values.size() == 8u - (end - start));
         cat::verify(values.capacity() == capacity);
         idx index;
         for (idx i; i < 8u; ++i) {
            if (i < start || i >= end) {
               cat::verify(element_value(values[index]) == int4(i));
               ++index;
            }
         }

         // The shorter side shifted.
         if (start < 8u - end) {
            cat::verify(values.data() == p_data + (end - start));
         } else {
            cat::verify(values.data() == p_data);
         }

         verify_devec_layout(values);
         values.free(g_tests_alloc);
      }
   }

   cat::devec<Element> values;
   for (idx i; i < 3u; ++i) {
      values.push_back(g_tests_alloc, make_element<Element>(int4(i))).verify();
   }

   values.erase(1u);
   cat::verify(values.size() == 2u && element_value(values[1u]) == 2_i4);
   values.free(g_tests_alloc);
}

$test(devec_erase) {
   verify_devec_erase<int4>();
   verify_devec_erase<int8>();
   verify_devec_erase<devec_probe>();
   cat::verify(devec_live_count == 0u);
}

$test(devec_ranges) {
   cat::devec<int4> values =
      cat::make_devec<int4>(g_tests_alloc, {3_i4, 4_i4, 5_i4}).verify();
   cat::array<int4, 2u> const front_values = {1_i4, 2_i4};
   values.prepend_range(g_tests_alloc, front_values).verify();
   values.append_range(g_tests_alloc, front_values).verify();

   cat::array<int4, 7u> const expected = {1_i4, 2_i4, 3_i4, 4_i4,
                                          5_i4, 1_i4, 2_i4};
   cat::verify(
      cat::span<int4 const>(values) == cat::span<int4 const>(expected)
   );

   // A range that aliases this `devec`.
   values.append_range(g_tests_alloc, cat::span<int4 const>(values)).verify();
   values.prepend_range(g_tests_alloc, values.first(2u).verify()).verify();
   cat::verify(values.size() == 16u);
   cat::verify(values[0u] == 1_i4 && values[1u] == 2_i4);
   cat::verify(values[2u] == 1_i4 && values[15u] == 2_i4);
   cat::verify((cat::ref(values) | cat::sum()) == 39_i4);

   // Non-contiguous ranges, with and without a size, keep their order.
   cat::devec<int4> taken;
   taken.prepend_range(g_tests_alloc, cat::ref(values) | cat::take(3u))
      .verify();
   taken.append_range(g_tests_alloc, cat::ref(values) | cat::take(2u)).verify();
   cat::array<int4, 5u> const expected_taken = {1_i4, 2_i4, 1_i4, 1_i4, 2_i4};
   cat::verify(
      cat::span<int4 const>(taken) == cat::span<int4 const>(expected_taken)
   );

   cat::devec<int4> filtered;
   filtered
      .append_range(g_tests_alloc, cat::ref(values) | cat::filter(cat::is_odd))
      .verify();
   filtered
      .prepend_range(
         g_tests_alloc, cat::ref(values) | cat::filter(cat::is_even)
      )
      .verify();
   cat::verify(filtered.size() == 16u);
   cat::verify(filtered.front() == 2_i4 && filtered.back() == 1_i4);
   cat::verify(filtered[5u] == 4_i4 && filtered[6u] == 2_i4);

   // Repeatedly appending small ranges grows geometrically.
   cat::devec<int4> grown;
   for (idx i; i < 1'000u; ++i) {
      grown.append_range(g_tests_alloc, cat::ref(values) | cat::take(1u))
         .verify();
      grown.prepend_range(g_tests_alloc, cat::ref(values) | cat::take(1u))
         .verify();
   }

   cat::verify(grown.size() == 2'000u);
   cat::verify(grown.capacity() <= 4'000u);

   taken.free(g_tests_alloc);
   filtered.free(g_tests_alloc);
   grown.free(g_tests_alloc);
   values.free(g_tests_alloc);
}

$test(devec_range_failure) {
   cat::allocator_null null_alloc;
   cat::devec<int4> values;
   cat::array<int4, 3u> const range = {1_i4, 2_i4, 3_i4};
   cat::verify(!values.append_range(null_alloc, range).has_value());
   cat::verify(!values.prepend_range(null_alloc, range).has_value());
   cat::verify(!values.append_range(null_alloc, cat::ref(range) | cat::take(2u))
                   .has_value());
   cat::verify(!values
                   .prepend_range(null_alloc, cat::ref(range) | cat::take(2u))
                   .has_value());
   cat::verify(
      !values
          .append_range(null_alloc, cat::ref(range) | cat::filter(cat::is_odd))
          .has_value()
   );
   cat::verify(values.is_empty());

   // An unsized range fails partway, keeping what it appended.
   allocator_failing allocator;
   allocator.can_grow = false;
   cat::devec<int4> partial;
   for (idx i; i < 16u; ++i) {
      partial.push_back(allocator, int4(i)).verify();
   }

   partial.shrink_to_fit(allocator).verify();
   cat::devec<int4> source;
   for (idx i; i < 100u; ++i) {
      source.push_back(g_tests_alloc, int4(i)).verify();
   }

   allocator.fail_after(0u);
   cat::verify(
      !partial
          .append_range(allocator, cat::ref(source) | cat::filter(cat::is_even))
          .has_value()
   );
   cat::verify(partial.size() == 16u);
   cat::verify(!partial
                   .prepend_range(
                      allocator, cat::ref(source) | cat::filter(cat::is_even)
                   )
                   .has_value());
   cat::verify(partial.size() == 16u);
   for (idx i; i < 16u; ++i) {
      cat::verify(partial[i] == int4(i));
   }

   allocator.never_fail();
   partial.free(allocator);
   source.free(g_tests_alloc);
   cat::verify(allocator.live_allocations == 0u);
}

$test(devec_reserve_resize_shrink_clone) {
   cat::devec<int4> values;
   values.reserve(g_tests_alloc, 10u, 20u).verify();
   cat::verify(values.front_capacity() == 10u);
   cat::verify(values.back_capacity() == 20u);
   int4 const* p_storage = values.data();

   values.resize(g_tests_alloc, 5u, 7_i4).verify();
   for (idx i; i < 10u; ++i) {
      values.push_front(g_tests_alloc, int4(i)).verify();
   }

   // Reserved slots are used before anything moves.
   cat::verify(values.data() == p_storage - 10u);

   // Reserving keeps the larger of each end.
   values.reserve(g_tests_alloc, 2u, 30u).verify();
   cat::verify(values.front_capacity() == 2u);
   cat::verify(values.back_capacity() == 30u);
   int4 const* p_reserved = values.data();
   values.reserve(g_tests_alloc, 1u, 1u).verify();
   cat::verify(values.data() == p_reserved);

   cat::devec<int4> copy = values.clone(g_tests_alloc).verify();
   cat::verify(copy == values);
   cat::verify(copy.front_capacity() == values.front_capacity());
   cat::verify(copy.back_capacity() == values.back_capacity());

   values.resize(g_tests_alloc, 3u).verify();
   cat::verify(values.size() == 3u && values[2u] == 7_i4);
   values.resize(g_tests_alloc, 6u).verify();
   cat::verify(values[5u] == 0_i4);
   values.shrink_to_fit(g_tests_alloc).verify();
   cat::verify(values.capacity() == 6u && values.front_capacity() == 0u);
   int4 const* p_tight = values.data();
   values.shrink_to_fit(g_tests_alloc).verify();
   cat::verify(values.data() == p_tight);

   values.clear();
   cat::verify(values.is_empty());
   cat::verify(values.front_capacity() == 3u && values.back_capacity() == 3u);
   values.shrink_to_fit(g_tests_alloc).verify();
   cat::verify(values.data() == nullptr);

   copy.free(g_tests_alloc);
   values.free(g_tests_alloc);
}

$test(devec_allocation_failure) {
   cat::allocator_null null_alloc;
   cat::devec<int4> values;
   cat::verify(!values.push_back(null_alloc, 1_i4).has_value());
   cat::verify(!values.push_front(null_alloc, 1_i4).has_value());
   cat::verify(!values.emplace(null_alloc, 0u, 1_i4).has_value());
   cat::verify(!values.reserve(null_alloc, 1u, 0u).has_value());
   cat::verify(!values.resize(null_alloc, 1u).has_value());
   cat::verify(!values.resize(null_alloc, 1u, 1_i4).has_value());
   cat::verify(values.data() == nullptr);

   // Requests beyond `max_size()` fail without allocating.
   allocator_failing allocator;
   cat::verify(!values.reserve(allocator, idx::max(), 0u).has_value());
   cat::verify(!values.reserve(allocator, 0u, idx::max()).has_value());
   cat::verify(
      !values.reserve(allocator, cat::devec<int4>::max_size(), 1u).has_value()
   );
   cat::verify(!values.resize(allocator, idx::max()).has_value());
   cat::verify(allocator.allocation_count == 0u);

   // Failing to grow leaves the elements intact.
   for (idx i; i < 16u; ++i) {
      values.push_back(allocator, int4(i)).verify();
   }

   values.shrink_to_fit(allocator).verify();
   allocator.fail_after(0u);
   cat::verify(!values.push_back(allocator, 99_i4).has_value());
   cat::verify(!values.push_front(allocator, 99_i4).has_value());
   cat::verify(!values.emplace(allocator, 3u, 99_i4).has_value());
   cat::verify(!values.emplace(allocator, 13u, 99_i4).has_value());
   cat::verify(!values.insert(allocator, 3u, 99_i4).has_value());
   cat::verify(!values.reserve(allocator, 1u, 1u).has_value());
   cat::verify(!values.resize(allocator, 17u).has_value());
   cat::verify(!values.clone(allocator).has_value());
   cat::verify(values.size() == 16u);
   for (idx i; i < 16u; ++i) {
      cat::verify(values[i] == int4(i));
   }

   values.pop_back().verify();
   cat::verify(!values.shrink_to_fit(allocator).has_value());
   cat::verify(values.size() == 15u);

   allocator.never_fail();
   values.free(allocator);
   cat::verify(allocator.live_allocations == 0u);
}

$test(devec_static_and_dynamic_dispatch) {
   allocator_failing allocator;
   cat::devec<int4> values;
   values.push_back<allocator_failing>(allocator, 1_i4).verify();
   values.push_front<allocator_failing>(allocator, 0_i4).verify();
   values.emplace_back<allocator_failing>(allocator, 3).verify();
   values.emplace_front<allocator_failing>(allocator, -1).verify();
   values.emplace<allocator_failing>(allocator, 3u, 2).verify();
   values.insert<allocator_failing>(allocator, 0u, -2_i4).verify();
   values.reserve<allocator_failing>(allocator, 4u, 4u).verify();
   values.resize<allocator_failing>(allocator, 7u).verify();
   values.resize<allocator_failing>(allocator, 8u, 5_i4).verify();
   cat::array<int4, 1u> const range = {6_i4};
   values.append_range<allocator_failing>(allocator, range).verify();
   values.prepend_range<allocator_failing>(allocator, range).verify();
   values.shrink_to_fit<allocator_failing>(allocator).verify();
   cat::devec<int4> copy = values.clone<allocator_failing>(allocator).verify();
   cat::devec<int4> made =
      cat::make_devec<int4, allocator_failing>(allocator, {1_i4}).verify();
   cat::verify(values.size() == 10u && copy == values && made.size() == 1u);

   // Large trivially copyable elements are passed by reference.
   cat::devec<devec_wide> wide;
   devec_wide const element(4);
   wide.push_back(allocator, element).verify();
   wide.push_front(allocator, element).verify();
   wide.push_back<allocator_failing>(allocator, element).verify();
   wide.push_front<allocator_failing>(allocator, element).verify();
   wide.push_back<allocator_failing>(allocator, devec_wide(5)).verify();
   wide.push_front<allocator_failing>(allocator, devec_wide(5)).verify();
   wide.insert(allocator, 1u, element).verify();
   wide.insert(allocator, 1u, devec_wide(6)).verify();
   wide.insert<allocator_failing>(allocator, 1u, devec_wide(6)).verify();
   wide.insert<allocator_failing>(allocator, 1u, element).verify();
   cat::verify(wide.size() == 10u && wide[1u].value == 4_i4);
   cat::verify(wide[2u].value == 6_i4);

   wide.free<allocator_failing>(allocator);
   made.free<allocator_failing>(allocator);
   copy.free(allocator);
   values.free(allocator);
   cat::verify(allocator.live_allocations == 0u);
}

$test(devec_relocation_only_elements) {
   // Elements which cannot be assigned still slide and grow by relocation.
   cat::devec<devec_relocatable> values;
   for (idx i; i < 100u; ++i) {
      values.emplace_back(g_tests_alloc, int4(i)).verify();
      values.emplace_front(g_tests_alloc, -int4(i)).verify();
   }

   for (idx i; i < 1'000u; ++i) {
      values.emplace_back(g_tests_alloc, int4(i)).verify();
      values.pop_front().verify();
   }

   cat::verify(values.size() == 200u);
   cat::verify(values.back().value == 999_i4);
   values.free(g_tests_alloc);
}

$test(devec_aligned_elements) {
   cat::devec<devec_aligned> values;
   for (idx i; i < 50u; ++i) {
      values.emplace_back(g_tests_alloc, int4(i)).verify();
      values.emplace_front(g_tests_alloc, int4(i)).verify();
      cat::verify(cat::is_aligned(values.data(), 64u));
   }

   cat::verify(values.capacity() >= 100u);
   values.free(g_tests_alloc);
}

$test(devec_nested) {
   cat::devec<cat::devec<int4>> outer;
   for (idx i; i < 20u; ++i) {
      cat::devec<int4> inner;
      inner.push_back(g_tests_alloc, int4(i)).verify();
      outer.push_back(g_tests_alloc, move(inner)).verify();
      outer.emplace_front(g_tests_alloc).verify();
   }

   cat::verify(outer.size() == 40u && outer.back()[0u] == 19_i4);
   for (cat::devec<int4>& inner : outer) {
      inner.free(g_tests_alloc);
   }

   outer.free(g_tests_alloc);
}

#if __has_feature(cxx_exceptions)
// Throw from each constructor that `operation` invokes in turn. A throwing
// operation must leave the `devec` unchanged, and nothing may leak.
template <typename Operation>
void
verify_devec_strong_guarantee(bool is_full, Operation&& operation) {
   bool completed = false;
   for (iword failure = 1; !completed; ++failure) {
      cat::verify(failure < 200);
      fragile::live = 0;
      allocator_failing allocator;
      cat::devec<fragile> values;
      for (idx i; i < 6u; ++i) {
         values.emplace_back(allocator, int4(i)).verify();
      }

      for (idx i; i < 3u; ++i) {
         values.emplace_front(allocator, -int4(i)).verify();
      }

      if (is_full) {
         values.shrink_to_fit(allocator).verify();
      }

      cat::vec<int4> before;
      for (fragile const& element : values) {
         before.push_back(g_tests_alloc, element.value).verify();
      }

      fragile::arm(failure);
      bool const threw = throws_injected_failure([&] {
         operation(values, allocator);
      });

      fragile::disarm();
      completed = !threw;
      if (threw) {
         cat::verify(values.size() == before.size());
         for (idx i; i < before.size(); ++i) {
            cat::verify(values[i].value == before[i]);
         }

         verify_devec_layout(values);
      }

      values.free(allocator);
      before.free(g_tests_alloc);
      cat::verify(allocator.live_allocations == 0u);
      cat::verify(fragile::live == 0);
   }
}

$test(devec_exception_safety) {
   for (bool is_full : {false, true}) {
      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.emplace_back(allocator, 10_i4).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.emplace_front(allocator, 10_i4).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.push_back(allocator, values[2u]).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.push_front(allocator, fragile(3)).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.emplace(allocator, 2u, 10_i4).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.emplace(allocator, 7u, 10_i4).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.insert(allocator, 4u, values[0u]).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.resize(allocator, 20u, values[1u]).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.resize(allocator, 20u).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.append_range(allocator, values.first(4u).verify()).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.prepend_range(allocator, values.last(4u).verify()).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         values.reserve(allocator, 30u, 30u).verify();
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         auto _ = values.shrink_to_fit(allocator);
      });

      verify_devec_strong_guarantee(is_full, [](auto& values, auto& allocator) {
         cat::devec<fragile> copy = values.clone(allocator).verify();
         copy.free(allocator);
      });
   }
}

$test(devec_exception_basic_guarantee) {
   // A single-pass range keeps what it appended before throwing.
   for (iword failure = 1; failure < 12; ++failure) {
      fragile::live = 0;
      allocator_failing allocator;
      cat::devec<fragile> values;
      cat::array<int4, 4u> const source = {1_i4, 2_i4, 3_i4, 4_i4};
      fragile::arm(failure);
      auto _ = throws_injected_failure([&] {
         values
            .append_range(
               allocator, cat::ref(source) | cat::transform([](int4 value) {
                             return fragile(value);
                          })
            )
            .verify();
      });

      fragile::disarm();
      cat::verify(fragile::live == iword(values.size()));
      values.free(allocator);
      cat::verify(allocator.live_allocations == 0u);
      cat::verify(fragile::live == 0);
   }
}
#endif

$test(raii_devec) {
   allocator_failing allocator;
   {
      cat::raii::devec<int4, allocator_failing> values(allocator);
      cat::verify(values.is_empty() && values.data() == nullptr);
      values.push_back(1_i4).verify();
      int4 const two = 2_i4;
      values.push_back(two).verify();
      values.push_front(0_i4).verify();
      values.push_front(two).verify();
      values.emplace_back(3).verify();
      values.emplace_front(-1).verify();
      values.emplace(2u, 7).verify();
      values.insert(1u, two).verify();
      values.insert(1u, 8_i4).verify();
      cat::array<int4, 7u> const expected_middle = {-1_i4, 8_i4, 2_i4, 2_i4,
                                                    7_i4,  0_i4, 1_i4};
      cat::verify(
         values.first(7u).verify() == cat::span<int4 const>(expected_middle)
      );
      cat::verify(values.pop_front().value() == -1_i4);
      cat::verify(values.pop_back().value() == 3_i4);
      values.erase(0u);
      values.erase(0u, 2u);
      cat::verify(values.size() == 4u && values.front() == 7_i4);

      cat::array<int4, 2u> const range = {10_i4, 11_i4};
      values.append_range(range).verify();
      values.prepend_range(range).verify();
      values.resize(10u).verify();
      values.resize(12u, 5_i4).verify();
      values.reserve(4u, 4u).verify();
      cat::verify(
         values.front_capacity() >= 4u && values.back_capacity() >= 4u
      );
      cat::verify(
         values.capacity()
         == values.front_capacity() + values.size() + values.back_capacity()
      );
      values.shrink_to_fit().verify();
      cat::verify(values.capacity() == 12u);
      cat::verify(
         cat::raii::devec<int4, allocator_failing>::max_size()
         == cat::devec<int4>::max_size()
      );

      // Copies compare equal, regardless of their allocator.
      cat::raii::devec<int4, allocator_failing> copy = values.clone().verify();
      cat::raii::devec<int4> dynamic_copy =
         values.clone(g_tests_alloc).verify();
      cat::verify(copy == values && dynamic_copy == values);
      cat::verify(!(copy < values));
      copy.push_back(1_i4).verify();
      cat::verify(copy != values && values < copy);

      cat::raii::devec<int4, allocator_failing> moved = move(copy);
      cat::verify(copy.data() == nullptr && moved.size() == 13u);
      moved.swap(values);
      cat::verify(values.size() == 13u && moved.size() == 12u);
      values = move(moved);
      cat::verify(values.size() == 12u && moved.data() == nullptr);
      cat::raii::devec<int4, allocator_failing>& alias = values;
      values = move(alias);
      cat::verify(values.size() == 12u);

      // The bound allocator is the one given at construction.
      idx const allocation_count = allocator.allocation_count;
      cat::span<int4> const borrowed =
         values.get_allocator().alloc_multi_uninit<int4>(1u).verify();
      values.get_allocator().free_multi_uninit(borrowed);
      cat::verify(allocator.allocation_count == allocation_count + 1u);

      cat::devec<int4> released = values.release();
      cat::verify(values.data() == nullptr && released.size() == 12u);
      cat::verify(dynamic_copy == released);
      released.free(allocator);

      values.push_back(1_i4).verify();
      values.clear();
      cat::verify(values.is_empty() && values.capacity() > 0u);
      values.free();
      cat::verify(values.capacity() == 0u);
      values.push_back(1_i4).verify();
   }

   // The destructors freed everything.
   cat::verify(allocator.live_allocations == 0u);

   cat::raii::devec<int4> made =
      cat::raii::make_devec<int4>(g_tests_alloc, {1_i4, 2_i4}).verify();
   cat::raii::devec<int4, allocator_failing> made_static =
      cat::raii::make_devec<int4, allocator_failing>(allocator, {3_i4})
         .verify();
   cat::verify(made.size() == 2u && made_static[0u] == 3_i4);
   cat::verify((cat::ref(made) | cat::sum()) == 3_i4);
   for (int4& value : made) {
      value += 1;
   }
   cat::verify(made[1u] == 3_i4);

   made_static.shrink_to_fit().verify();
   allocator.fail_after(0u);
   cat::verify(!made_static.push_back(1_i4).has_value());
   cat::verify(!made_static.clone().has_value());
   cat::verify(
      !cat::raii::make_devec<int4, allocator_failing>(allocator, {3_i4})
          .has_value()
   );
   allocator.never_fail();
}

$test(devec_insert_range) {
   cat::array<int4, 20u> range;
   for (idx i; i < range.size(); ++i) {
      range[i] = int4(100 + i.raw);
   }

   cat::array<idx, 7u> const positions = {0u, 1u, 5u, 20u, 35u, 39u, 40u};
   for (idx position : positions) {
      cat::devec<int4> values;
      cat::vec<int4> expected;
      for (idx i; i < 40u; ++i) {
         values.push_back(g_tests_alloc, int4(i.raw)).verify();
         expected.push_back(g_tests_alloc, int4(i.raw)).verify();
      }

      values.insert_range(g_tests_alloc, position, range).verify();
      expected.insert_range(g_tests_alloc, position, range).verify();

      // A non-contiguous range.
      values
         .insert_range(
            g_tests_alloc, position, cat::ref(range).filter([](int4) {
               return true;
            })
         )
         .verify();
      expected.insert_range(g_tests_alloc, position, range).verify();

      // A contiguous range of this `devec`'s own elements.
      values.insert_range(g_tests_alloc, position, values.first(3u).verify())
         .verify();
      cat::array<int4, 3u> const first = {
         expected[0u], expected[1u], expected[2u]
      };
      expected.insert_range(g_tests_alloc, position, first).verify();

      cat::verify(
         cat::span<int4 const>(values) == cat::span<int4 const>(expected)
      );
      values.free(g_tests_alloc);
      expected.free(g_tests_alloc);
   }
}

$test(devec_factories) {
   cat::devec<int4> empty = cat::make_devec<int4>();
   cat::verify(empty.size() == 0u && empty.data() == nullptr);

   cat::devec<int4> reserved =
      cat::make_devec_reserved<int4>(g_tests_alloc, 3u, 5u).verify();
   cat::verify(reserved.size() == 0u);
   cat::verify(reserved.front_capacity() >= 3u);
   cat::verify(reserved.back_capacity() >= 5u);

   cat::devec<int4> filled =
      cat::make_devec_filled(g_tests_alloc, 4u, 7_i4).verify();
   cat::verify(filled.size() == 4u);
   for (int4 value : filled) {
      cat::verify(value == 7_i4);
   }

   cat::xoshiro_engine<cat::uint8> engine(102u);
   cat::devec<cat::uint8> random =
      cat::make_devec_filled_random<cat::uint8>(g_tests_alloc, 9u, engine)
         .verify();
   cat::verify(random.size() == 9u);

   cat::uniform_int_distribution<int4> distribution(-4, 7);
   cat::devec<int4> distributed = cat::make_devec_filled_random<int4>(
                                     g_tests_alloc, 11u, engine, distribution
   )
                                     .verify();
   cat::verify(distributed.size() == 11u);
   for (int4 value : distributed) {
      cat::verify(value >= -4 && value <= 7);
   }

   cat::raii::devec<int4> managed_empty =
      cat::raii::make_devec<int4>(g_tests_alloc);
   cat::verify(managed_empty.size() == 0u);

   cat::raii::devec<int4> managed_reserved =
      cat::raii::make_devec_reserved<int4>(g_tests_alloc, 2u, 2u).verify();
   cat::verify(managed_reserved.capacity() >= 4u);

   cat::raii::devec<int4> managed_filled =
      cat::raii::make_devec_filled(g_tests_alloc, 3u, 2_i4).verify();
   cat::verify(managed_filled.size() == 3u && managed_filled[2u] == 2_i4);

   cat::raii::devec<int4> managed_distributed =
      cat::raii::make_devec_filled_random<int4>(
         g_tests_alloc, 5u, engine, distribution
      )
         .verify();
   cat::verify(managed_distributed.size() == 5u);

   reserved.free(g_tests_alloc);
   filled.free(g_tests_alloc);
   random.free(g_tests_alloc);
   distributed.free(g_tests_alloc);
}
