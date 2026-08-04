#include <cat/array>
#include <cat/iterable>
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
   if (values.try_push_back(1_i4).is_empty()) {
      return false;
   }
   if (values.try_push_back(2_i4).is_empty()) {
      return false;
   }
   return values.size() == 2u && values[1u] == 2_i4;
}());

$test(vec_inplace_fixed_iteration) {
   cat::vec_inplace_fixed<int4, 4u> values{
      1_i4,
      2_i4,
      3_i4,
      4_i4,
   };

   cat::verify(values.size() == 4u);
   cat::verify(values.capacity() == 4u);
   cat::verify(values.is_full());

   idx count;
   for (int4 value : values) {
      cat::verify(value == int4(count + 1u));
      ++count;
   }
   cat::verify(count == values.size());

   auto even_squares = values
                          .filter([](int4 value) -> bool {
                             return value % 2_i4 == 0_i4;
                          })
                          .transform([](int4 value) -> int4 {
                             return value * value;
                          });
   cat::verify(even_squares.sum() == 20_i4);

   values.fill(7_i4);
   for (int4 value : values) {
      cat::verify(value == 7_i4);
   }

   auto fixed_filled = cat::make_vec_inplace_fixed_filled<int4, 3u>(8_i4);
   cat::verify(fixed_filled.size() == 3u);
   cat::verify(fixed_filled[2u] == 8_i4);
}

$test(vec_inplace_default_and_access) {
   cat::vec_inplace<int4, 4u> values;

   cat::verify(values.size() == 0u);
   cat::verify(values.capacity() == 4u);
   cat::verify(values.max_size() == 4u);
   cat::verify(values.is_empty());
   cat::verify(values.at(0u).is_empty());

   values.try_push_back(10_i4).verify();
   values.try_emplace_back(20_i4).verify();

   cat::verify(values.front() == 10_i4);
   cat::verify(values.back() == 20_i4);
   cat::verify(values[1u] == 20_i4);
   cat::verify(values.at(1u).verify() == 20_i4);
}

$test(vec_inplace_try_and_unchecked_push) {
   cat::vec_inplace<int4, 2u> values;

   int4& first = values.try_push_back(1_i4).verify();
   int4& second = values.unchecked_emplace_back(2_i4);
   first = 3_i4;
   second = 4_i4;

   cat::verify(values.size() == 2u);
   cat::verify(values[0u] == 3_i4);
   cat::verify(values[1u] == 4_i4);
   cat::verify(values.try_push_back(5_i4).is_empty());
   cat::verify(values.try_emplace_back(6_i4).is_empty());
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

   values.try_append_range(source).verify();
   cat::verify(values.size() == 3u);
   cat::verify(values.try_append_range(extra).is_empty());
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
   values.try_append_range(source).verify();

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
      values.try_emplace_back(1_i4).verify();
      values.try_emplace_back(2_i4).verify();
      values.try_emplace_back(3_i4).verify();
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
   left.try_push_back(1_i4).verify();
   left.try_push_back(2_i4).verify();

   cat::vec_inplace<int4, 4u> copied(left);
   cat::verify(copied == left);

   cat::vec_inplace<int4, 4u> moved(cat::move(copied));
   cat::verify(moved == left);
   cat::verify(copied.is_empty());

   cat::vec_inplace<int4, 4u> right;
   right.try_push_back(8_i4).verify();
   cat::swap(left, right);
   cat::verify(left.size() == 1u);
   cat::verify(left[0u] == 8_i4);
   cat::verify(right.size() == 2u);
   cat::verify(right[1u] == 2_i4);
}

$test(vec_inplace_cross_capacity_compare) {
   cat::vec_inplace<int4, 2u> small;
   cat::vec_inplace<int4, 5u> large;
   small.try_push_back(1_i4).verify();
   small.try_push_back(2_i4).verify();
   large.try_push_back(1_i4).verify();
   large.try_push_back(2_i4).verify();

   cat::verify(small == large);
   cat::verify((small <=> large) == 0);

   large.try_push_back(3_i4).verify();
   cat::verify(small != large);
   cat::verify((small <=> large) < 0);
   cat::verify((large <=> small) > 0);
}

$test(vec_inplace_zero_capacity) {
   cat::vec_inplace<int4, 0u> values;
   cat::verify(values.is_empty());
   cat::verify(values.is_full());
   cat::verify(values.capacity() == 0u);
   cat::verify(values.try_push_back(1_i4).is_empty());
   cat::verify(values.resize(1u).is_empty());
}
