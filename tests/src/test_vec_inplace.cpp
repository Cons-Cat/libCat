#include <cat/array>
#include <cat/iterable>
#include <cat/meta>
#include <cat/utility>
#include <cat/vec_inplace>

#include "../unit_tests.hpp"

namespace {

struct lifetime_value {
   static inline idx live_count = 0u;

   int4 value = 0_i4;

   lifetime_value() {
      ++live_count;
   }

   lifetime_value(int4 input) : value(input) {
      ++live_count;
   }

   lifetime_value(lifetime_value const& other) : value(other.value) {
      ++live_count;
   }

   lifetime_value(lifetime_value&& other) : value(other.value) {
      other.value = 0_i4;
      ++live_count;
   }

   auto
   operator=(lifetime_value const&) -> lifetime_value& = default;

   auto
   operator=(lifetime_value&& other) -> lifetime_value& {
      value = other.value;
      other.value = 0_i4;
      return *this;
   }

   ~lifetime_value() {
      live_count = idx(live_count - 1u);
   }

   auto
   operator<=>(lifetime_value const&) const = default;
};

}  // namespace

static_assert([] {
   cat::vec_inplace<int4, 2u> values;
   if (values.push_back(1_i4).is_empty()) {
      return false;
   }
   if (values.push_back(2_i4).is_empty()) {
      return false;
   }
   return values.size() == 2u && values[1u] == 2_i4;
}());

$test(vec_inplace_default_and_access) {
   cat::vec_inplace<int4, 4u> values;

   cat::verify(values.size() == 0u);
   cat::verify(values.capacity() == 4u);
   cat::verify(values.max_size() == 4u);
   cat::verify(values.is_empty());
   cat::verify(values.at(0u).is_empty());

   values.push_back(10_i4).verify();
   values.emplace_back(20_i4).verify();

   cat::verify(values.front() == 10_i4);
   cat::verify(values.back() == 20_i4);
   cat::verify(values[1u] == 20_i4);
   cat::verify(values.at(1u).verify() == 20_i4);
}

$test(vec_inplace_push_and_unchecked_push) {
   cat::vec_inplace<int4, 2u> values;

   int4& first = values.push_back(1_i4).verify();
   int4& second = values.unchecked_emplace_back(2_i4);
   first = 3_i4;
   second = 4_i4;

   cat::verify(values.size() == 2u);
   cat::verify(values[0u] == 3_i4);
   cat::verify(values[1u] == 4_i4);
   cat::verify(values.push_back(5_i4).is_empty());
   cat::verify(values.emplace_back(6_i4).is_empty());
   cat::verify(values.size() == 2u);
}

$test(vec_inplace_resize_and_reserve) {
   cat::vec_inplace<int4, 4u> values;

   values.reserve(4u).verify();
   cat::verify(values.reserve(5u).is_empty());

   values.resize(3u, 7_i4).verify();
   cat::verify(values.size() == 3u);
   cat::verify(values[2u] == 7_i4);
   cat::verify(values.resize(5u, 9_i4).is_empty());
   cat::verify(values.size() == 3u);

   values.resize(1u).verify();
   cat::verify(values.size() == 1u);
   values.resize(4u, 8_i4).verify();
   cat::verify(values.size() == 4u);
   cat::verify(values[3u] == 8_i4);

   values.fill(6_i4);
   for (int4 value : values) {
      cat::verify(value == 6_i4);
   }

   auto filled = cat::make_vec_inplace_filled<int4, 4u>(3u, 5_i4).verify();
   cat::verify(filled.size() == 3u);
   cat::verify(filled[2u] == 5_i4);
}

$test(vec_inplace_append_and_iterate) {
   cat::array source{1_i4, 2_i4, 3_i4};
   cat::array extra{4_i4, 5_i4};
   cat::vec_inplace<int4, 4u> values;

   values.append_range(source).verify();
   cat::verify(values.size() == 3u);
   cat::verify(values.append_range(extra).is_empty());
   cat::verify(values.size() == 3u);
   values.unchecked_push_back(4_i4);

   cat::verify((values | cat::sum()) == 10_i4);
   cat::verify(cat::read_at(values, 2u) == 3_i4);
   idx count = 0u;
   for (int4 value : values) {
      cat::verify(value == int4(count + 1u));
      ++count;
   }
   cat::verify(count == values.size());

   auto even_squares = cat::ref(values)
                          .filter([](int4 value) -> bool {
                             return value % 2_i4 == 0_i4;
                          })
                          .transform([](int4 value) -> int4 {
                             return value * value;
                          });
   cat::verify(even_squares.sum() == 20_i4);
}

$test(vec_inplace_erase_pop_and_clear) {
   cat::vec_inplace<int4, 6u> values;
   cat::array source{1_i4, 2_i4, 3_i4, 4_i4, 5_i4};
   values.append_range(source).verify();

   values.erase(1u);
   cat::verify(values.size() == 4u);
   cat::verify(values[1u] == 3_i4);

   values.erase(1u, 3u);
   cat::verify(values.size() == 2u);
   cat::verify(values[0u] == 1_i4);
   cat::verify(values[1u] == 5_i4);

   cat::verify(values.pop_back().verify() == 5_i4);
   cat::verify(values.pop_back().verify() == 1_i4);
   cat::verify(values.pop_back().is_empty());

   values.resize(3u, 9_i4).verify();
   values.clear();
   cat::verify(values.is_empty());
   cat::verify(values.capacity() == 6u);
}

$test(vec_inplace_nontrivial_lifetime) {
   lifetime_value::live_count = 0u;
   {
      cat::vec_inplace<lifetime_value, 4u> values;
      values.emplace_back(1_i4).verify();
      values.emplace_back(2_i4).verify();
      values.emplace_back(3_i4).verify();
      cat::verify(lifetime_value::live_count == 3u);

      values.erase(1u);
      cat::verify(lifetime_value::live_count == 2u);

      auto popped = values.pop_back();
      cat::verify(popped.has_value());
      cat::verify(popped.value().value == 3_i4);
      cat::verify(lifetime_value::live_count == 2u);

      popped.reset();
      cat::verify(lifetime_value::live_count == 1u);
      values.clear();
      cat::verify(lifetime_value::live_count == 0u);
   }
   cat::verify(lifetime_value::live_count == 0u);
}

$test(vec_inplace_copy_move_and_swap) {
   cat::vec_inplace<int4, 4u> left;
   left.push_back(1_i4).verify();
   left.push_back(2_i4).verify();

   cat::vec_inplace<int4, 4u> copied(left);
   cat::verify(copied == left);

   cat::vec_inplace<int4, 4u> moved(cat::move(copied));
   cat::verify(moved == left);
   cat::verify(copied.is_empty());

   cat::vec_inplace<int4, 4u> right;
   right.push_back(8_i4).verify();
   cat::swap(left, right);
   cat::verify(left.size() == 1u);
   cat::verify(left[0u] == 8_i4);
   cat::verify(right.size() == 2u);
   cat::verify(right[1u] == 2_i4);
}

$test(vec_inplace_cross_capacity_compare) {
   cat::vec_inplace<int4, 2u> small;
   cat::vec_inplace<int4, 5u> large;
   small.push_back(1_i4).verify();
   small.push_back(2_i4).verify();
   large.push_back(1_i4).verify();
   large.push_back(2_i4).verify();

   cat::verify(small == large);
   cat::verify((small <=> large) == 0);

   large.push_back(3_i4).verify();
   cat::verify(small != large);
   cat::verify((small <=> large) < 0);
   cat::verify((large <=> small) > 0);
}

$test(vec_inplace_zero_capacity) {
   cat::vec_inplace<int4, 0u> values;
   cat::verify(values.is_empty());
   cat::verify(values.is_full());
   cat::verify(values.capacity() == 0u);
   cat::verify(values.push_back(1_i4).is_empty());
   cat::verify(values.resize(1u).is_empty());
}

$test(vec_inplace_subscript_compile_time_bounds) {
   cat::vec_inplace<int4, 4u> values;
   cat::vec_inplace<int4, 4u> const constant;
   cat::vec_inplace<int4, 0u> empty;
   idx runtime = 4u;

   static_assert(cat::is_same<decltype(values[3u]), int4&>);
   static_assert(cat::is_same<decltype(values[4u]), void>);
   static_assert(cat::is_same<decltype(constant[0u]), int4 const&>);
   static_assert(cat::is_same<decltype(constant[4u]), void>);
   static_assert(cat::is_same<decltype(cat::move(values)[0u]), int4>);
   static_assert(cat::is_same<decltype(cat::move(values)[4u]), void>);
   static_assert(cat::is_same<decltype(empty[0u]), void>);
   static_assert(cat::is_same<decltype(values[runtime]), int4&>);
}

namespace {

struct move_only {
   int4 value;

   move_only(int4 input) : value(input) {
   }

   move_only(move_only const&) = delete;
   move_only(move_only&&) = default;
   auto
   operator=(move_only const&) -> move_only& = delete;
   auto
   operator=(move_only&&) -> move_only& = default;
};

struct no_default {
   int4 value;

   no_default(int4 input) : value(input) {
   }
};

}  // namespace

static_assert(cat::vec_inplace<int4, 3u>::capacity() == 3u);
static_assert(cat::vec_inplace<int4, 3u>::max_size() == 3u);
static_assert(!cat::is_copy_constructible<cat::vec_inplace<move_only, 2u>>);
static_assert(cat::is_move_constructible<cat::vec_inplace<move_only, 2u>>);

template <typename Values>
concept can_default_resize = requires(Values values) { values.resize(1u); };

static_assert(!can_default_resize<cat::vec_inplace<no_default, 2u>>);
static_assert(can_default_resize<cat::vec_inplace<int4, 2u>>);

static_assert([] {
   cat::vec_inplace<int4, 6u> values;
   if (values.resize(5u, 3_i4).is_empty()) {
      return false;
   }
   values[0u] = 1_i4;
   values.erase(1u);
   values.erase(0u, 2u);
   if (values.pop_back().value() != 3_i4) {
      return false;
   }

   cat::vec_inplace<int4, 6u> copy = values;
   cat::vec_inplace<int4, 4u> other;
   other.unchecked_push_back(3_i4);
   return copy == values && values == other && values.size() == 1u;
}());

$test(vec_inplace_iterators) {
   using values_type = cat::vec_inplace<int4, 4u>;
   using iterator = values_type::iterator;
   using const_iterator = values_type::const_iterator;
   using reverse_iterator = values_type::reverse_iterator;
   using const_reverse_iterator = values_type::const_reverse_iterator;

   values_type values;
   values_type const& constant = values;
   static_assert(cat::is_same<iterator, decltype(values.begin())>);
   static_assert(cat::is_same<iterator, decltype(values.end())>);
   static_assert(cat::is_same<const_iterator, decltype(constant.begin())>);
   static_assert(cat::is_same<const_iterator, decltype(values.cbegin())>);
   static_assert(cat::is_same<const_iterator, decltype(values.cend())>);
   static_assert(cat::is_same<reverse_iterator, decltype(values.rbegin())>);
   static_assert(cat::is_same<reverse_iterator, decltype(values.rend())>);
   static_assert(
      cat::is_same<const_reverse_iterator, decltype(values.crbegin())>
   );
   static_assert(
      cat::is_same<const_reverse_iterator, decltype(values.crend())>
   );
   static_assert(cat::is_same<iterator::reference, int4&>);
   static_assert(cat::is_same<const_iterator::reference, int4 const&>);

   cat::verify(values.begin() == values.end());
   cat::verify(values.cbegin() == values.cend());

   values.append_range(cat::array{1_i4, 2_i4, 3_i4, 4_i4}).verify();
   cat::verify(values.end() - values.begin() == 4);
   cat::verify(values.cend() - values.cbegin() == 4);
   cat::verify(&*values.begin() == values.data());
   cat::verify(values.begin() + 4 == values.end());
   cat::verify(values.end() - 1 == values.begin() + 3);
   cat::verify(values.begin() < values.end());
   cat::verify(values.begin()[2] == 3_i4);
   cat::verify(*(values.end() - 1) == 4_i4);

   // Writes through a mutable iterator are visible through a const one.
   *values.begin() = 10_i4;
   values.begin()[1] = 20_i4;
   cat::verify(*constant.begin() == 10_i4);
   cat::verify(*(values.cbegin() + 1) == 20_i4);

   // `rbegin()` to `rend()` visits every element.
   cat::verify(values.rend() - values.rbegin() == 4);
   cat::verify(*(values.rend() - 1) == 10_i4);
   auto reverse = values.rbegin();
   cat::verify(*reverse == 4_i4);
   cat::verify(*++reverse == 3_i4);
   cat::verify(*(values.rbegin() + 3) == 10_i4);
   cat::verify(*values.crbegin() == 4_i4);

   idx count = 0u;
   for (auto it = values.cbegin(); it != values.cend(); ++it) {
      cat::verify(&*it == values.data() + count);
      ++count;
   }
   cat::verify(count == values.size());
}

// Operations that would exceed the capacity have no effects.
$test(vec_inplace_full_has_no_effects) {
   cat::array source{1_i4, 2_i4, 3_i4};
   cat::vec_inplace<int4, 3u> values;
   values.append_range(source).verify();
   cat::vec_inplace<int4, 3u> const before = values;

   cat::verify(values.push_back(9_i4).is_empty());
   cat::verify(values.emplace_back(9_i4).is_empty());
   cat::verify(values.resize(4u).is_empty());
   cat::verify(values.resize(4u, 9_i4).is_empty());
   cat::verify(values.append_range(cat::array{9_i4}).is_empty());
   cat::verify(values == before);

   cat::vec_inplace<int4, 3u> empty;
   cat::verify(
      empty.append_range(cat::array{1_i4, 2_i4, 3_i4, 4_i4}).is_empty()
   );
   cat::verify(empty.is_empty());
}

$test(vec_inplace_data) {
   cat::vec_inplace<int4, 3u> values;
   values.push_back(1_i4).verify();
   values.push_back(2_i4).verify();
   cat::vec_inplace<int4, 3u> const& constant = values;

   cat::verify(values.data() == __builtin_addressof(values.front()));
   cat::verify(constant.data() == __builtin_addressof(constant.front()));
   cat::verify(values.data()[1u] == 2_i4);
}

// Regrowing over a popped element value-initializes it.
$test(vec_inplace_resize_value_initializes) {
   cat::vec_inplace<int4, 2u> values;
   values.push_back(255_i4).verify();
   auto _ = values.pop_back();
   values.resize(2u).verify();
   cat::verify(values[0u] == 0_i4);
   cat::verify(values[1u] == 0_i4);

   lifetime_value::live_count = 0u;
   {
      cat::vec_inplace<lifetime_value, 4u> lifetimes;
      lifetimes.resize(3u).verify();
      cat::verify(lifetime_value::live_count == 3u);
      cat::verify(lifetimes[2u].value == 0_i4);
      lifetimes.resize(1u).verify();
      cat::verify(lifetime_value::live_count == 1u);
   }
   cat::verify(lifetime_value::live_count == 0u);
}

$test(vec_inplace_erase_edges) {
   cat::vec_inplace<int4, 4u> values;
   values.append_range(cat::array{1_i4, 2_i4, 3_i4, 4_i4}).verify();

   values.erase(2u, 2u);
   cat::verify(values.size() == 4u);
   values.erase(3u);
   cat::verify(values.size() == 3u);
   cat::verify(values.back() == 3_i4);
   values.erase(0u, values.size());
   cat::verify(values.is_empty());
}

$test(vec_inplace_move_only) {
   cat::vec_inplace<move_only, 3u> values;
   values.emplace_back(1_i4).verify();
   values.push_back(move_only(2_i4)).verify();
   values.unchecked_emplace_back(3_i4);
   values.erase(0u);
   cat::verify(values[0u].value == 2_i4);

   cat::vec_inplace<move_only, 3u> moved(cat::move(values));
   cat::verify(values.is_empty());
   cat::verify(moved.size() == 2u);
   cat::swap(values, moved);
   cat::verify(values.size() == 2u);
   cat::verify(moved.is_empty());
   cat::verify(values.pop_back().verify().value == 3_i4);
}

$test(vec_inplace_no_default) {
   cat::vec_inplace<no_default, 3u> values;
   values.emplace_back(1_i4).verify();
   values.resize(3u, no_default(5_i4)).verify();
   cat::verify(values[2u].value == 5_i4);
   values.resize(1u, no_default(0_i4)).verify();
   cat::verify(values.size() == 1u);
   cat::verify(values[0u].value == 1_i4);
}

$test(vec_inplace_assignment_lifetimes) {
   lifetime_value::live_count = 0u;
   {
      cat::vec_inplace<lifetime_value, 4u> large;
      cat::vec_inplace<lifetime_value, 4u> small;
      large.resize(4u, lifetime_value(1_i4)).verify();
      small.resize(1u, lifetime_value(2_i4)).verify();
      cat::verify(lifetime_value::live_count == 5u);

      large = small;
      cat::verify(lifetime_value::live_count == 2u);
      cat::verify(large == small);

      small.resize(3u, lifetime_value(3_i4)).verify();
      large = cat::move(small);
      cat::verify(small.is_empty());
      cat::verify(large.size() == 3u);
      cat::verify(lifetime_value::live_count == 3u);

      // Self-assignment and self-swap have no effects.
      auto& alias = large;
      large = alias;
      large = cat::move(alias);
      large.swap(alias);
      cat::verify(large.size() == 3u);
      cat::verify(large[0u].value == 2_i4);
      cat::verify(lifetime_value::live_count == 3u);
   }
   cat::verify(lifetime_value::live_count == 0u);
}

namespace {
template <idx capacity, idx extent>
void
verify_ints(
   cat::vec_inplace<int4, capacity> const& values,
   cat::array<int4, extent> const& expected
) {
   cat::verify(values.size() == extent);
   for (idx i; i < extent; ++i) {
      cat::verify(values[i] == expected[i]);
   }
}
}  // namespace

$test(vec_inplace_emplace_and_insert) {
   cat::vec_inplace<int4, 6u> values;
   values.append_range(cat::array{1_i4, 2_i4, 3_i4}).verify();

   cat::verify(values.emplace(0u, 0_i4).verify() == 0_i4);
   values.insert(4u, 4_i4).verify();
   int4 const five = 5_i4;
   values.insert(2u, five).verify();
   verify_ints(values, cat::array{0_i4, 1_i4, 5_i4, 2_i4, 3_i4, 4_i4});

   // `values` is full, so nothing changes.
   cat::verify(values.emplace(1u, 9_i4).is_empty());
   cat::verify(values.insert(1u, five).is_empty());
   cat::verify(values.insert_n(0u, 1u, five).is_empty());
   verify_ints(values, cat::array{0_i4, 1_i4, 5_i4, 2_i4, 3_i4, 4_i4});

   // Inserting an element of `values` copies it before anything moves.
   values.truncate(3u);
   values.insert(0u, values[2u]).verify();
   values.insert_n(1u, 2u, values[0u]).verify();
   verify_ints(values, cat::array{5_i4, 5_i4, 5_i4, 0_i4, 1_i4, 5_i4});

   values.truncate(2u);
   values.insert_n(1u, 3u, 7_i4).verify();
   verify_ints(values, cat::array{5_i4, 7_i4, 7_i4, 7_i4, 5_i4});
   values.insert_n(5u, 0u, 7_i4).verify();
   cat::verify(values.size() == 5u);
}

$test(vec_inplace_insert_range) {
   cat::vec_inplace<int4, 8u> values;
   values.append_range(cat::array{1_i4, 2_i4}).verify();

   values.insert_range(1u, cat::array{8_i4, 9_i4}).verify();
   verify_ints(values, cat::array{1_i4, 8_i4, 9_i4, 2_i4});
   values.insert_range(4u, cat::array<int4, 0u>{}).verify();
   cat::verify(values.size() == 4u);

   // A range over `values` itself is read before anything moves.
   values.insert_range(0u, values).verify();
   verify_ints(
      values, cat::array{1_i4, 8_i4, 9_i4, 2_i4, 1_i4, 8_i4, 9_i4, 2_i4}
   );
   cat::verify(values.insert_range(0u, cat::array{0_i4}).is_empty());
   cat::verify(values.size() == 8u);

   // A single-pass range which does not fit has no effects.
   cat::array source{10_i4, 11_i4, 12_i4};
   auto single_pass = cat::ref(source).filter([](int4) -> bool {
      return true;
   });
   values.truncate(6u);
   cat::verify(values.append_range(single_pass).is_empty());
   cat::verify(values.insert_range(0u, single_pass).is_empty());
   verify_ints(values, cat::array{1_i4, 8_i4, 9_i4, 2_i4, 1_i4, 8_i4});
   values.truncate(5u);
   values.insert_range(1u, single_pass).verify();
   verify_ints(
      values, cat::array{1_i4, 10_i4, 11_i4, 12_i4, 8_i4, 9_i4, 2_i4, 1_i4}
   );
}

$test(vec_inplace_replace_and_assign) {
   cat::vec_inplace<int4, 6u> values;
   values.append_range(cat::array{1_i4, 2_i4, 3_i4, 4_i4}).verify();

   // Grow, shrink, and keep the size of the replaced range.
   values.replace_with_range(1u, 2u, cat::array{7_i4, 8_i4, 9_i4}).verify();
   verify_ints(values, cat::array{1_i4, 7_i4, 8_i4, 9_i4, 3_i4, 4_i4});
   values.replace_with_range(1u, 4u, cat::array{5_i4}).verify();
   verify_ints(values, cat::array{1_i4, 5_i4, 3_i4, 4_i4});
   values.replace_with_range(0u, 2u, cat::array{6_i4, 6_i4}).verify();
   verify_ints(values, cat::array{6_i4, 6_i4, 3_i4, 4_i4});
   values.replace_with_range(1u, 3u, cat::array<int4, 0u>{}).verify();
   verify_ints(values, cat::array{6_i4, 4_i4});

   // The result must fit, even if the replaced range was large.
   cat::verify(values
                  .replace_with_range(
                     0u, 1u, cat::array{1_i4, 1_i4, 1_i4, 1_i4, 1_i4, 1_i4}
                  )
                  .is_empty());
   verify_ints(values, cat::array{6_i4, 4_i4});
   values.replace_with_range(0u, 1u, cat::array{1_i4, 2_i4, 3_i4, 4_i4, 5_i4})
      .verify();
   verify_ints(values, cat::array{1_i4, 2_i4, 3_i4, 4_i4, 5_i4, 4_i4});

   // A range over `values` itself is copied before anything is overwritten.
   values.replace_with_range(0u, 3u, cat::span(values).subspan(2u, 3u))
      .verify();
   verify_ints(values, cat::array{3_i4, 4_i4, 5_i4, 4_i4, 5_i4, 4_i4});

   values.assign_range(cat::array{9_i4, 8_i4}).verify();
   verify_ints(values, cat::array{9_i4, 8_i4});
   cat::verify(
      values.assign_range(cat::array{0_i4, 0_i4, 0_i4, 0_i4, 0_i4, 0_i4, 0_i4})
         .is_empty()
   );
   verify_ints(values, cat::array{9_i4, 8_i4});

   values.assign(4u, values[1u]).verify();
   verify_ints(values, cat::array{8_i4, 8_i4, 8_i4, 8_i4});
   values.assign(1u, 2_i4).verify();
   verify_ints(values, cat::array{2_i4});
   cat::verify(values.assign(7u, 0_i4).is_empty());
   values.append(2u, 3_i4).verify();
   verify_ints(values, cat::array{2_i4, 3_i4, 3_i4});
   cat::verify(values.append(4u, 3_i4).is_empty());
   cat::verify(values.size() == 3u);
}

$test(vec_inplace_truncate) {
   lifetime_value::live_count = 0u;
   {
      cat::vec_inplace<lifetime_value, 6u> values;
      values.resize(6u, lifetime_value(1_i4)).verify();
      values.truncate(4u);
      cat::verify(lifetime_value::live_count == 4u);
      values.pop_back_n(3u);
      cat::verify(lifetime_value::live_count == 1u);
      values.resize(2u).verify();
      cat::verify(lifetime_value::live_count == 2u);
      cat::verify(values[0u].value == 1_i4);

      values.insert_range(1u, cat::array{lifetime_value(2_i4)}).verify();
      values.replace_with_range(0u, 2u, cat::array{lifetime_value(3_i4)})
         .verify();
      cat::verify(values.size() == 2u);
      cat::verify(lifetime_value::live_count == 2u);
   }
   cat::verify(lifetime_value::live_count == 0u);
}

static_assert([] {
   cat::vec_inplace<int4, 6u> values;
   values.unchecked_push_back(1_i4);
   values.unchecked_push_back(4_i4);
   if (values.insert_range(1u, cat::array{2_i4, 3_i4}).is_empty()) {
      return false;
   }
   if (values.emplace(0u, 0_i4).is_empty()) {
      return false;
   }
   if (values.replace_with_range(4u, 5u, cat::array{4_i4, 5_i4}).is_empty()) {
      return false;
   }
   for (idx i; i < values.size(); ++i) {
      if (values[i] != int4(i)) {
         return false;
      }
   }
   return values.size() == 6u;
}());

#if __has_feature(cxx_exceptions)
#include "../injected_failure.hpp"

namespace {
template <idx capacity>
using fragile_vec = cat::vec_inplace<fragile, capacity>;

template <idx capacity>
void
fill_fragile(fragile_vec<capacity>& values, int4 count) {
   for (int4 i = 0; i < count; ++i) {
      values.emplace_back(i).verify();
   }
}

template <idx capacity>
void
verify_fragile_sequence(fragile_vec<capacity> const& values, int4 count) {
   cat::verify(values.size() == idx(count));
   for (int4 i = 0; i < count; ++i) {
      cat::verify(values[idx(i)].value == i);
   }
}
}  // namespace

// Appending one element has no effects if its constructor throws.
$test(vec_inplace_push_back_strong_guarantee) {
   fragile::live = 0;
   {
      fragile_vec<4u> values;
      fill_fragile(values, 2);
      fragile const source(7);

      fragile::arm(1);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.push_back(source);
      }));
      fragile::arm(1);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.push_back(fragile(source));
      }));
      fragile::arm(1);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.emplace_back(8);
      }));
      fragile::arm(1);
      cat::verify(throws_injected_failure([&] {
         values.unchecked_emplace_back(8);
      }));
      fragile::disarm();
      verify_fragile_sequence(values, 2);
      cat::verify(fragile::live == 3);

      values.push_back(source).verify();
      cat::verify(values.back().value == 7);
   }
   cat::verify(fragile::live == 0);
}

$test(vec_inplace_resize_strong_guarantee) {
   fragile::live = 0;
   for (iword failure = 1; failure <= 3; ++failure) {
      fragile_vec<6u> values;
      fill_fragile(values, 2);

      fragile::arm(failure);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.resize(5u);
      }));
      fragile::disarm();
      verify_fragile_sequence(values, 2);
      cat::verify(fragile::live == 2);

      fragile const source(9);
      fragile::arm(failure);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.resize(5u, source);
      }));
      fragile::disarm();
      verify_fragile_sequence(values, 2);
      cat::verify(fragile::live == 3);
   }
   cat::verify(fragile::live == 0);

   cat::array<fragile, 1u> const range = {fragile(4)};
   fragile::live = 1;
   for (iword failure = 1; failure <= 4; ++failure) {
      fragile::arm(failure);
      cat::verify(throws_injected_failure([&] {
         auto _ = cat::make_vec_inplace_filled<fragile, 4u>(4u, range[0u]);
      }));
      fragile::disarm();
      cat::verify(fragile::live == 1);
   }
}

// `append_range()` has the basic guarantee: the old elements are unmodified,
// and every new element that is not destroyed is still owned by the
// `vec_inplace`.
$test(vec_inplace_append_range_basic_guarantee) {
   fragile::live = 0;
   cat::array<fragile, 3u> const range = {fragile(2), fragile(3), fragile(4)};

   for (iword failure = 1; failure <= 4; ++failure) {
      {
         fragile_vec<6u> values;
         fill_fragile(values, 2);

         fragile::arm(failure);
         bool const threw = throws_injected_failure([&] {
            auto _ = values.append_range(range);
         });
         fragile::disarm();
         cat::verify(threw == (failure <= 3));
         cat::verify(fragile::live == iword(values.size()) + 3);
         cat::verify(values.size() >= 2u);
         verify_fragile_sequence(values, int4(values.size().raw));
      }
      cat::verify(fragile::live == 3);
   }
}

// A throwing copy or move constructor leaks nothing and leaves the source
// intact.
$test(vec_inplace_construct_throwing) {
   fragile::live = 0;
   {
      fragile_vec<4u> source;
      fill_fragile(source, 4);

      for (iword failure = 1; failure <= 4; ++failure) {
         fragile::arm(failure);
         cat::verify(throws_injected_failure([&] {
            fragile_vec<4u> copy(source);
         }));
         fragile::arm(failure);
         cat::verify(throws_injected_failure([&] {
            fragile_vec<4u> moved(cat::move(source));
         }));
         fragile::disarm();
         verify_fragile_sequence(source, 4);
         cat::verify(fragile::live == 4);
      }
   }
   cat::verify(fragile::live == 0);
}

// Assignment has the basic guarantee.
$test(vec_inplace_assign_basic_guarantee) {
   fragile::live = 0;
   for (iword failure = 1; failure <= 4; ++failure) {
      {
         fragile_vec<4u> target;
         fragile_vec<4u> source;
         fill_fragile(target, 2);
         fill_fragile(source, 4);

         fragile::arm(failure);
         cat::verify(throws_injected_failure([&] {
            target = source;
         }));
         fragile::disarm();
         cat::verify(fragile::live == iword(target.size()) + 4);
         verify_fragile_sequence(target, int4(target.size().raw));

         fragile::arm(failure);
         cat::verify(throws_injected_failure([&] {
            target = cat::move(source);
         }));
         fragile::disarm();
         cat::verify(fragile::live == iword(target.size()) + 4);
         verify_fragile_sequence(source, 4);
      }
      cat::verify(fragile::live == 0);
   }
}

// A throwing `swap()` leaks nothing.
$test(vec_inplace_swap_throwing) {
   fragile::live = 0;
   for (iword failure = 1; failure <= 8; ++failure) {
      {
         fragile_vec<4u> left;
         fragile_vec<4u> right;
         fill_fragile(left, 3);
         fill_fragile(right, 2);

         fragile::arm(failure);
         bool const threw = throws_injected_failure([&] {
            left.swap(right);
         });
         fragile::disarm();
         cat::verify(fragile::live == iword(left.size() + right.size()));
         if (!threw) {
            verify_fragile_sequence(left, 2);
            verify_fragile_sequence(right, 3);
         }
      }
      cat::verify(fragile::live == 0);
   }
}

// Inserting a single element or copies of one has no effects if a constructor
// throws.
$test(vec_inplace_insert_strong_guarantee) {
   fragile::live = 0;
   {
      fragile_vec<6u> values;
      fill_fragile(values, 3);
      fragile const source(9);

      for (iword failure = 1; failure <= 2; ++failure) {
         fragile::arm(failure);
         cat::verify(throws_injected_failure([&] {
            auto _ = values.insert_n(1u, 2u, source);
         }));
         fragile::arm(failure);
         cat::verify(throws_injected_failure([&] {
            auto _ = values.append(2u, source);
         }));
         fragile::disarm();
         verify_fragile_sequence(values, 3);
         cat::verify(fragile::live == 4);
      }

      fragile::arm(1);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.emplace(1u, 9);
      }));
      fragile::arm(1);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.insert(0u, source);
      }));
      fragile::disarm();
      verify_fragile_sequence(values, 3);
      cat::verify(fragile::live == 4);
   }
   cat::verify(fragile::live == 0);
}

// Inserting and replacing ranges have the basic guarantee.
$test(vec_inplace_range_modifiers_basic_guarantee) {
   fragile::live = 0;
   cat::array<fragile, 3u> const range = {fragile(7), fragile(8), fragile(9)};

   for (iword failure = 1; failure <= 12; ++failure) {
      for (idx position = 0u; position <= 3u; ++position) {
         {
            fragile_vec<8u> values;
            fill_fragile(values, 3);
            fragile::arm(failure);
            throws_injected_failure([&] {
               auto _ = values.insert_range(position, range);
            });
            fragile::arm(failure);
            throws_injected_failure([&] {
               auto _ = values.replace_with_range(
                  0u, position, cat::span(values).subspan(0u, 2u)
               );
            });
            fragile::arm(failure);
            throws_injected_failure([&] {
               auto _ = values.replace_with_range(position, 3u, range);
            });
            fragile::disarm();
            cat::verify(fragile::live == iword(values.size()) + 3);
         }
         cat::verify(fragile::live == 3);
      }
   }
}

$test(vec_inplace_pop_back_strong_guarantee) {
   fragile::live = 0;
   {
      fragile_vec<4u> values;
      fill_fragile(values, 3);

      fragile::arm(1);
      cat::verify(throws_injected_failure([&] {
         auto _ = values.pop_back();
      }));
      fragile::disarm();
      verify_fragile_sequence(values, 3);
      cat::verify(fragile::live == 3);
   }
   cat::verify(fragile::live == 0);
}
#endif
