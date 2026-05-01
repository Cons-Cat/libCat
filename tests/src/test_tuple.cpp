#include <cat/meta>
#include <cat/tuple>

#include "../unit_tests.hpp"

// Not a P2141 magic-aggregate: tuple-like `std` hooks + `cat::get` and member
// `get` for structured bindings.
$test(tuple_structured_bindings_with_get) {
   cat::tuple<int, bool> t{4, true};
   static_assert(!cat::has_aggregate_get<decltype(t)>);
   cat::verify(t.first() == 4);
   cat::verify(t.template get<0u>() == 4);
   cat::verify(cat::get<0u>(t) == 4);
   cat::verify(cat::get<1u>(t) == true);
   auto& [ti, tb] = t;
   cat::verify(ti == 4 && tb);
   ti = 9;
   cat::verify(t.first() == 9);
}

struct tup_non_trivial {
   tup_non_trivial(int) {
   }

   ~tup_non_trivial() {
   }
};

static_assert(!cat::is_implicit_lifetime<tup_non_trivial>);

$test(tuple_basics) {
   using intint = cat::tuple<int, int>;
   static_assert(cat::is_trivially_copyable<intint>
                 && cat::is_trivially_default_constructible<intint>);
   static_assert(cat::is_implicit_lifetime<intint>);
   static_assert(sizeof(intint) == 8);

   using floatfloat = cat::tuple<float, float>;
   static_assert(cat::is_trivially_copyable<floatfloat>
                 && cat::is_trivially_default_constructible<floatfloat>);
   static_assert(cat::is_implicit_lifetime<floatfloat>);
   static_assert(sizeof(floatfloat) == 8);

   using non_and_int4 = cat::tuple<tup_non_trivial, int4>;
   [[maybe_unused]]
   non_and_int4 test_intint4{1, int4{0}};
   static_assert(!cat::is_implicit_lifetime<cat::array<non_and_int4, 1>>);
   static_assert(sizeof(non_and_int4) == 4);

   // Test `tuple` storage.
   intint tuple;
   int& left_1 = tuple.get<0>();
   left_1 = 10;
   int left_2 = tuple.get<0>();
   cat::verify(left_2 == 10);
   cat::verify(left_2 == left_1);
   cat::verify(left_2 == tuple.first());
   tuple.second() = 20;
   cat::verify(tuple.size() == 2);

   // Test destructuring.
   auto& [int_1, int_2] = tuple;
   cat::verify(int_1 == 10);
   cat::verify(int_2 == 20);

   // Test aggregate construction.
   cat::tuple<int, char> intchar = {100, 'a'};
   cat::verify(intchar.first() == 100);
   cat::verify(intchar.second() == 'a');

   // Test aggregate assignment.
   intchar = {200, 'b'};
   cat::tuple<int, char> intchar2{200, 'b'};
   cat::verify(intchar.first() == 200);
   cat::verify(intchar.second() == 'b');

   // Test `const`.
   cat::tuple<int, char> const intchar_const = {100, 'a'};
   cat::verify(intchar_const.first() == 100);
   cat::verify(intchar_const.second() == 'a');

   // Test value categories for moved tuples.
   cat::tuple<int, char> intchar_move{100, 'a'};
   static_assert(
      cat::is_same<decltype(cat::move(intchar_move).first()), int&&>);
   static_assert(
      cat::is_same<decltype(cat::move(intchar_move).second()), char&&>);

   cat::tuple<int, char> const intchar_move_const{100, 'a'};
   static_assert(cat::is_same<decltype(cat::move(intchar_move_const).first()),
                              int const&&>);
   static_assert(cat::is_same<decltype(cat::move(intchar_move_const).second()),
                              char const&&>);

   // Test type deduction.
   cat::tuple deduced = {0, 'b', 10.f};
   static_assert(cat::is_same<decltype(deduced.get<0>()), int&>);
   static_assert(cat::is_same<decltype(deduced.get<1>()), char&>);
   static_assert(cat::is_same<decltype(deduced.get<2>()), float&>);

   // Test `tuple` auto-generated getters.
   static_assert(
      cat::is_implicit_lifetime<cat::tuple<char, int4, bool, void*, uint8>>);
   cat::tuple<char, int4, bool, void*, uint8> five_tuple;
   static_assert(cat::is_same<decltype(five_tuple.first()), char&>);
   static_assert(cat::is_same<decltype(five_tuple.second()), int4&>);
   static_assert(cat::is_same<decltype(five_tuple.third()), bool&>);
   static_assert(cat::is_same<decltype(five_tuple.fourth()), void*&>);
   static_assert(cat::is_same<decltype(five_tuple.fifth()), uint8&>);

   // Test structured bindings.
   auto& [one, two, three, four, five] = five_tuple;
   one = 'a';
   two = 2;
   three = true;
   four = nullptr;
   five = 1u;

   // Test that `tuple` size is zero-overhead.
   static_assert(sizeof(intint) == sizeof(int) * 2);

   // This type is 32 bytes due to padding for member alignment.
   struct five_layout {
      // Eight bytes:
      char c;
      int4 i;
      // Eight bytes:
      bool b;
      // Sixteen bytes:
      void* p;
      uint8 u;
   };

   static_assert(sizeof(five_tuple) == sizeof(five_layout));

   // Test empty tuple.
   static_assert(cat::is_implicit_lifetime<cat::tuple<>>);
   [[maybe_unused]]
   cat::tuple<> empty_tuple;
   [[maybe_unused]]
   cat::tuple<> empty_tuple_2{};
   static_assert(empty_tuple.is_empty());
   cat::verify(empty_tuple.is_empty());
   cat::verify(empty_tuple.size() == 0);
   cat::verify(!tuple.is_empty());

   // Tuple of tuples.
   static_assert(cat::is_implicit_lifetime<
                 cat::tuple<cat::tuple<int, int>, cat::tuple<int, char>>>);
   cat::tuple<cat::tuple<int, int>, cat::tuple<int, char>> tuple_of_tuple = {
      tuple, intchar};
   cat::tuple tuple_of_tuple_ctad = {tuple, intchar};
   static_assert(
      cat::is_same<decltype(tuple_of_tuple_ctad), decltype(tuple_of_tuple)>);
}

$test(tuple_implicit_lifetime_when_members_are) {
   static_assert(cat::is_implicit_lifetime<cat::tuple<>>);

   static_assert(cat::is_implicit_lifetime<cat::tuple<int4>>);
   static_assert(cat::is_implicit_lifetime<cat::tuple<uint8, bool>>);
   static_assert(cat::is_implicit_lifetime<cat::tuple<char, int4, float4>>);
   static_assert(cat::is_implicit_lifetime<cat::tuple<idx, iword, uword>>);

   using inner = cat::tuple<int4, int4>;
   static_assert(cat::is_implicit_lifetime<cat::tuple<inner, uint8>>);
   static_assert(cat::is_implicit_lifetime<
                 cat::tuple<cat::tuple<>, cat::tuple<int1, uint1>>>);

   static_assert(cat::is_implicit_lifetime<cat::tuple<cat::array<int4, 2u>>>);
}

$test(tuple_operator_index) {
   // Subscript is `t[index]` with `cat::idx` and Clang `enable_if` so only the
   // element whose slot matches `index` is a viable member `operator[]`.
   cat::tuple<int, int> t{1, 2};
   static_assert(cat::is_same<decltype(t[0_idx]), int&>);
   static_assert(cat::is_same<decltype(t[1_idx]), int&>);
   cat::verify(t[0_idx] == 1);
   cat::verify(t[1_idx] == 2);
   t[0_idx] = 100;
   cat::verify(t.first() == 100);
   cat::verify(t[0_idx] == 100);

   cat::tuple<int, int> const tc{3, 4};
   static_assert(cat::is_same<decltype(tc[0_idx]), int const&>);
   static_assert(cat::is_same<decltype(tc[1_idx]), int const&>);
   cat::verify(tc[0_idx] == 3);
   cat::verify(tc[1_idx] == 4);

   // r-value `tuple<...>` and value elements: `[]` yields x-value and can move
   // from move-only element storage.
   {
      struct move_only {
         int4 payload = 0;
         bool moved_from = false;

         constexpr move_only() = default;

         constexpr explicit move_only(int4 in_payload) : payload(in_payload) {
         }

         constexpr move_only(move_only const&) = delete;
         constexpr auto
         operator=(move_only const&) -> move_only& = delete;

         constexpr move_only(move_only&& other)
             : payload(other.payload), moved_from(false) {
            other.moved_from = true;
         }
      };

      static_assert(
         cat::is_same<decltype(cat::tuple<move_only>{}[0_idx]), move_only&&>);

      cat::tuple<move_only> holder{move_only{42}};
      move_only moved = cat::move(holder)[0_idx];
      cat::verify(moved.payload == 42);
      cat::verify(holder[0_idx].moved_from);
   }

   {
      // `const` r-value: `T const&&` for non-ref elements.
      int from_const_rvalue =
         static_cast<cat::tuple<int> const&&>(cat::tuple<int>{5})[0_idx];
      cat::verify(from_const_rvalue == 5);
   }

   int n0 = 11;
   int n1 = 22;
   cat::tuple<int&, int&> r{n0, n1};
   static_assert(cat::is_same<decltype(r[0_idx]), int&>);
   static_assert(cat::is_same<decltype(r[1_idx]), int&>);
   r[0_idx] = 33;
   r[1_idx] = 44;
   cat::verify(n0 == 33);
   cat::verify(n1 == 44);
}

$test(tuple_at) {
   cat::tuple<int, int> t{1, 2};
   cat::verify(t.at(0_idx).has_value() && t.at(0_idx).value() == 1);
   cat::verify(t.at(1_idx).has_value() && t.at(1_idx).value() == 2);
   cat::verify(!t.at(2_idx).has_value());
   t.at(0_idx).assert() = 9;
   cat::verify(t[0_idx] == 9);

   cat::tuple<int, int> const c{3, 4};
   static_assert(cat::is_same<decltype(c.at(0_idx).value()), int const&>);
   cat::verify(c.at(0_idx).assert() == 3);
   cat::verify(!c.at(2_idx).has_value());

   cat::tuple<int, char> mixed{7, 'x'};
   static_assert(cat::is_same<decltype(mixed.at(0_idx).value()), int&>);
   static_assert(cat::is_same<decltype(mixed.at(1_idx).value()), char&>);
   cat::verify(mixed.at(0_idx).assert() == 7);
   cat::verify(mixed.at(1_idx).assert() == 'x');
   cat::verify(!mixed.at(2_idx).has_value());
}

$test(tuple_cat) {
   using intint = cat::tuple<int, int>;
   using floatfloat = cat::tuple<float, float>;
   [[maybe_unused]]
   cat::tuple<> empty_tuple;

   cat::tuple_cat();
   auto cat_empty = cat::tuple_cat(empty_tuple);
   static_assert(cat::is_same<decltype(cat_empty), cat::tuple<>>);

   auto cat_two = cat::tuple_cat(intint{0, 1}, floatfloat{2.f, 3.f});
   static_assert(
      cat::is_same<decltype(cat_two), cat::tuple<int, int, float, float>>);
   cat::verify(cat_two.first() == 0);
   cat::verify(cat_two.second() == 1);
   cat::verify(cat_two.third() == 2.f);
   cat::verify(cat_two.fourth() == 3.f);

   auto cat_one = cat::tuple_cat(intint{7, 8});
   static_assert(cat::is_same<decltype(cat_one), intint>);
   cat::verify(cat_one.first() == 7);
   cat::verify(cat_one.second() == 8);

   static_assert(cat::is_same<decltype(cat::tuple_cat(cat::tuple<int>{})),
                              cat::tuple<int>>);
   static_assert(cat::is_same<decltype(cat::tuple_cat(
                                 cat::tuple<>{}, cat::tuple<>{}, intint{1, 2})),
                              intint>);

   auto cat_three = cat::tuple_cat(cat::tuple<int>{10}, cat::tuple<char>{'a'},
                                   cat::tuple<float>{1.f});
   static_assert(
      cat::is_same<decltype(cat_three), cat::tuple<int, char, float>>);
   cat::verify(cat_three.first() == 10);
   cat::verify(cat_three.second() == 'a');
   cat::verify(cat_three.third() == 1.f);

   // Empty tuples in the pack should be identity elements.
   auto cat_mixed_empties =
      cat::tuple_cat(cat::tuple<>{}, intint{3, 4}, cat::tuple<>{},
                     floatfloat{5.f, 6.f}, cat::tuple<>{});
   static_assert(cat::is_same<decltype(cat_mixed_empties),
                              cat::tuple<int, int, float, float>>);
   cat::verify(cat_mixed_empties.first() == 3);
   cat::verify(cat_mixed_empties.second() == 4);
   cat::verify(cat_mixed_empties.third() == 5.f);
   cat::verify(cat_mixed_empties.fourth() == 6.f);

   int left_i = 1;
   int right_i = 2;
   auto cat_refs = cat::tuple_cat(cat::tuple<int&, int&>{left_i, right_i},
                                  cat::tuple<int>{3});
   static_assert(cat::is_same<decltype(cat_refs), cat::tuple<int&, int&, int>>);
   cat::verify(cat_refs.first() == 1);
   cat::verify(cat_refs.second() == 2);
   cat::verify(cat_refs.third() == 3);
   left_i = 9;
   cat::verify(cat_refs.first() == 9);

   // `const` l-value arguments should be forwarded into element types as
   // `T const&`.
   int const a_const = 11;
   int const b_const = 12;
   auto cat_const = cat::tuple_cat(cat::tuple<int const&>{a_const},
                                   cat::tuple<int const&>{b_const});
   static_assert(
      cat::is_same<decltype(cat_const), cat::tuple<int const&, int const&>>);
   cat::verify(cat_const.first() == 11);
   cat::verify(cat_const.second() == 12);

   struct not_copy {
      not_copy() = default;

      not_copy(not_copy const&) = delete;
      auto
      operator=(not_copy const&) -> not_copy& = delete;

      constexpr not_copy(not_copy&&) = default;
   };

   // `tuple_cat` of r-value `tuple<...>` should move the elements, even when
   // the element type is move-only.
   {
      not_copy m0{};
      not_copy m1{};
      auto only_moves =
         cat::tuple_cat(cat::tuple<not_copy>{static_cast<not_copy&&>(m0)},
                        cat::tuple<not_copy>{static_cast<not_copy&&>(m1)});
      static_assert(
         cat::is_same<decltype(only_moves), cat::tuple<not_copy, not_copy>>);
   }
}

$test(tuple_tie_make_forward) {
   int tied_left = 1;
   char tied_right = 'a';
   auto tied = cat::tie(tied_left, tied_right);
   tied.assign(3, 'z');
   cat::verify(tied_left == 3);
   cat::verify(tied_right == 'z');

   auto made = cat::make_tuple(4, 5.f);
   static_assert(cat::is_same<decltype(made), cat::tuple<int, float>>);
   cat::verify(made.first() == 4);
   cat::verify(made.second() == 5.f);

   auto forwarded = cat::forward_as_tuple(tied_left, tied_right);
   static_assert(cat::is_same<decltype(forwarded), cat::tuple<int&, char&>>);
   forwarded.first() = 9;
   cat::verify(tied_left == 9);

   {
      int lvalue = 10;
      int by_ref = 20;
      auto f = cat::forward_as_tuple(lvalue, 1, 2, by_ref, 3);
      static_assert(cat::is_same<decltype(f),
                                 cat::tuple<int&, int&&, int&&, int&, int&&>>);
      cat::verify(f.first() == 10);
      cat::verify(f.second() == 1);
      cat::verify(f.third() == 2);
      cat::verify(f.fourth() == 20);
      cat::verify(f.fifth() == 3);
      lvalue = 11;
      by_ref = 30;
      cat::verify(f.first() == 11);
      cat::verify(f.fourth() == 30);
   }

   {
      int lvalue = 10;
      int const const_lvalue = 20;
      auto c = cat::forward_as_tuple(lvalue, const_lvalue, 30);
      static_assert(
         cat::is_same<decltype(c), cat::tuple<int&, int const&, int&&>>);
      cat::verify(c.first() == 10);
      cat::verify(c.second() == 20);
      cat::verify(c.third() == 30);
   }

   {
      // `T&&` parameters in `forward_as_tuple` still bind gl-values, so a named
      // r-value reference is an l-value here, not an x-value.
      int x = 5;
      int&& rvalue_ref = static_cast<int&&>(x);
      int prvalue = 7;
      auto t = cat::forward_as_tuple(rvalue_ref, static_cast<int&&>(prvalue));
      static_assert(cat::is_same<decltype(t), cat::tuple<int&, int&&>>);
      cat::verify(t.first() == 5);
      cat::verify(t.second() == 7);
   }
}

$test(tuple_apply) {
   auto apply_sum = cat::apply(
      [](int left, int right) {
         return left + right;
      },
      cat::tuple<int, int>{2, 3});
   cat::verify(apply_sum == 5);
}

$test(tuple_algorithms) {
   cat::tuple<int, int, int> bulk{2, 4, 6};
   cat::verify(bulk.any_of([](int value) {
      return value == 4;
   }));
   cat::verify(!bulk.any_of([](int value) {
      return value == 5;
   }));
   cat::verify(bulk.all_of([](int value) {
      return value % 2 == 0;
   }));
   cat::verify(!bulk.all_of([](int value) {
      return value < 6;
   }));

   auto mapped = bulk.transform([](int value) {
      return value + 1;
   });
   static_assert(cat::is_same<decltype(mapped), cat::tuple<int, int, int>>);
   cat::verify(mapped.first() == 3);
   cat::verify(mapped.second() == 5);
   cat::verify(mapped.third() == 7);
   cat::verify(bulk.first() + bulk.second() + bulk.third() == 12);
}
