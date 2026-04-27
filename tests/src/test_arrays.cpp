#include <cat/array>
#include <cat/math>
#include <cat/meta>
#include <cat/runtime>
#include <cat/string>
#include <cat/utility>

#include "../unit_tests.hpp"

struct move_only {
   int i = 0;
   move_only() = default;
   move_only(move_only const&) = delete;

   move_only(move_only&& other) : i(other.i) {
   }
};

test(array_structured_bindings) {
   [] consteval {
      constexpr cat::array<int, 3> constexpr_array{1, 2, 3};
      static_assert(std::tuple_size_v<cat::array<int, 3>> == 3);
      static_assert(cat::is_same<
                    std::tuple_element_t<1, cat::array<int, 3>>, int>);
      static_assert(cat::get<0>(constexpr_array) == 1);
      static_assert(cat::get<2>(constexpr_array) == 3);
   }();

   cat::array<int4, 3u> bound_array = {1_i4, 2_i4, 3_i4};
   auto& [by_a, by_b, by_c] = bound_array;
   cat::verify(by_a == 1_i4 && by_b == 2_i4 && by_c == 3_i4);

   auto const& [constref_a, constref_b, constref_c] = bound_array;
   cat::verify(constref_a == 1_i4 && constref_b == 2_i4 && constref_c == 3_i4);

   cat::array<int4, 3u> const bound_const = {1_i4, 2_i4, 3_i4};
   auto const& [c_a, c_b, c_c] = bound_const;
   cat::verify(c_a == 1_i4 && c_b == 2_i4 && c_c == 3_i4);

   auto [copy_a, copy_b, copy_c] = bound_array;
   cat::verify(copy_a == 1_i4 && copy_b == 2_i4 && copy_c == 3_i4);

   cat::array<int4, 3u> mutable_array = {4_i4, 5_i4, 6_i4};
   auto& [ref_a, ref_b, ref_c] = mutable_array;
   ref_a = 7_i4;
   cat::verify(mutable_array[0] == 7_i4 && ref_b == 5_i4 && ref_c == 6_i4);

   // Two-element `cat::get` + structured binding (shorter `cat::array` than
   // above)
   {
      cat::array<int, 2u> pair_array{1, 2};
      cat::verify(cat::get<0>(pair_array) == 1);
      auto& [e0, e1] = pair_array;
      cat::verify(e0 == 1 && e1 == 2);
      e0 = 0;
      cat::verify(pair_array[0u] == 0);
   }
}

test(array_traits) {
   static_assert(cat::is_trivially_default_constructible<cat::array<int, 1u>>);
   static_assert(cat::is_trivially_copyable<cat::array<int, 1u>>);
   static_assert(cat::is_trivially_copy_constructible<cat::array<int, 1u>>);
   static_assert(cat::is_trivially_move_constructible<cat::array<int, 1u>>);
   static_assert(cat::is_trivially_copy_assignable<cat::array<int, 1u>>);
   static_assert(cat::is_trivially_move_assignable<cat::array<int, 1u>>);
   static_assert(cat::is_trivially_destructible<cat::array<int, 1u>>);
   static_assert(cat::is_implicit_lifetime<cat::array<int, 1u>>);
}

test(array_brace_initialization_and_list_assign) {
   cat::array array_1{0, 1, 2, 3, 4};
   array_1 = {5, 6, 7, 8, 9};
   cat::verify(array_1[0] == 5);
   cat::verify(array_1[4] == 9);
   [[maybe_unused]]
   cat::array<int4, 1u> array_2;
}

test(array_move_only_element) {
   [[maybe_unused]]
   cat::array array_move_only = {move_only()};
   [[maybe_unused]]
   cat::array array_3 = mov array_move_only;
}

test(array_const_element_access) {
   cat::array<int4, 3u> const array_const = {0, 1, 2};
   [[maybe_unused]]
   int4 const_val = array_const.at(1).or_exit();
   cat::verify(const_val == 1);
}

test(array_consteval_copy_assign) {
   [] consteval {
      cat::array<int4, 1u> const_array_1{};
      cat::array<int4, 1u> const_array_2 = {1};
      const_array_2 = const_array_1;
   }();
}

test(array_range_based_for_each_direction) {
   cat::array array_1{5, 6, 7, 8, 9};

   idx count = 0u;
   for (int& a : array_1) {
      cat::verify(a == array_1[count]);
      ++count;
   }

   for (int& a : cat::as_reverse_stepanov(array_1)) {
      count.raw -= 1u;
      cat::verify(a == array_1[count]);
   }

   count = 0u;
   for (int const& a : cat::as_const(array_1)) {
      cat::verify(a == array_1[count]);
      ++count;
   }
   auto _ = array_1.cbegin();

   for (int const& a : cat::as_const_reverse_stepanov(array_1)) {
      count.raw -= 1u;
      cat::verify(a == array_1[count]);
   }
}

test(array_front_back_advance_to) {
   cat::array array_1{5, 6, 7, 8, 9};
   cat::verify(array_1.front() == 5);
   cat::verify(array_1.back() == 9);

   int4 array_to = *(array_1.begin().advance_to(--array_1.end()));
   cat::verify(array_to == array_1.back());
}

test(array_at_in_and_out_of_bounds) {
   cat::array array_1{5, 6, 7, 8, 9};
   cat::verify(array_1.at(0).value() == 5);
   cat::verify(!array_1.at(6).has_value());
}

test(array_class_template_argument_deduction) {
   cat::array implicit_array_1 = {0, 1, 2, 3, 4};
   cat::array implicit_array_2{0, 1, 2, 3, 4};
   cat::array implicit_array_3(0, 1, 2, 3, 4);
   static_assert(implicit_array_1.size() == 5u);
   static_assert(implicit_array_1.capacity() == 5);
   auto _ = implicit_array_1.capacity();
   static_assert(implicit_array_2.size() == 5);
   static_assert(implicit_array_3.size() == 5);

   // TODO: string deduction?
   //    cat::array implicit_string = "Hi, Conscat!";
   //    static_assert(implicit_string.size() ==
   //                  cat::string_length("Hi, Conscat!"));
}

test(array_subspan_first_last) {
   cat::array array_1{5, 6, 7, 8, 9};
   cat::span span = array_1;
   span = array_1.first(1u);
   auto _ = array_1.subspan(0u, 2u);
   auto _ = array_1.last(2u);

   cat::array<int4, 3u> const array_const = {0, 1, 2};
   cat::span const span_const = array_1.first(1u);
   auto _ = array_const.subspan(0u, 2u);
   auto _ = array_const.last(2u);
}

test(array_copy_assignment) {
   cat::array<int4, 4u> base_array = {0_i4, 0_i4, 0_i4, 0_i4};
   cat::array<int4, 4u> copy_array = {1_i4, 2_i4, 3_i4, 4_i4};
   base_array = copy_array;
   cat::verify(base_array[0] == 1_i4 && base_array[3] == 4_i4);
}

test(array_make_array_filled_and_fill) {
   cat::array filled_array = cat::make_array_filled<8>(6_i4);
   for (idx i; i < 8; ++i) {
      cat::verify(filled_array[i] == 6);
   }

   filled_array.fill(9);
   for (idx i; i < 8; ++i) {
      cat::verify(filled_array[i] == 9);
   }
}

test(array_brace_initializer_lists) {
   cat::array from_array{1, 2, 3};
   cat::verify(from_array[0] == 1);
   cat::verify(from_array[1] == 2);
   cat::verify(from_array[2] == 3);

   cat::array huge_array{1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3};
   cat::verify(huge_array.size() == 12u);
}

// Mirrors `std::array` lexicographic compare and equality for the same payload.
test(array_three_way_and_equality_consteval) {
   [] consteval {
      constexpr cat::array<int, 3> a{1, 2, 3};
      constexpr cat::array<int, 3> b{1, 2, 3};
      constexpr cat::array<int, 3> c{1, 2, 4};
      constexpr cat::array<int, 3> d{2, 0, 0};
      constexpr cat::array<int, 3> e{1, 3, 0};
      constexpr cat::array<int, 3> f{1, 2, 9};
      constexpr cat::array<int, 0> z{};
      constexpr cat::array<int, 0> w{};

      static_assert((a <=> b) == 0);
      static_assert((a <=> c) < 0);
      static_assert((c <=> a) > 0);
      static_assert((a <=> d) < 0);
      static_assert((e <=> f) > 0);
      static_assert((z <=> w) == 0);

      static_assert(a == b);
      static_assert(!(a == c));
      static_assert(!(a != b));
      static_assert(a != c);
   }();
}

test(array_three_way_and_equality_runtime) {
   cat::array<int, 3> const a{1, 2, 3};
   cat::array<int, 3> const b{1, 2, 3};
   cat::array<int, 3> const c{1, 2, 4};
   cat::verify((a <=> b) == 0);
   cat::verify((a <=> c) < 0);
   cat::verify(a == b);
   cat::verify(a != c);
}
