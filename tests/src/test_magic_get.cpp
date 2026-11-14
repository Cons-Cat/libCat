#include <cat/tuple>

#include "../unit_tests.hpp"

namespace {
struct three_fields {
   cat::int4 a;
   char b;
   bool c;
};

struct two_const {
   int const x;
   int const y;
};
}  // namespace

// Test a plain aggregate with no libCat tuple protocol.
$test(magic_get_structured_bindings_plain_aggregate) {
   three_fields s{.a = 1, .b = 'd', .c = true};
   auto& [p0, p1, p2] = s;
   cat::verify(p0 == 1 && p1 == 'd' && p2);
   p0 = -1;
   cat::verify(s.a == -1);
   three_fields s2{.a = 3, .b = 'e', .c = false};
   auto [q0, q1, q2] = s2;
   cat::verify(q0 == 3 && q1 == 'e' && !q2);
}

$test(magic_get_get_lvalue) {
   three_fields s{.a = 1, .b = 'z', .c = true};
   cat::verify(cat::get<0>(s) == 1);
   cat::verify(cat::get<1>(s) == 'z');
   cat::verify(cat::get<2>(s));
   cat::get<0>(s) = 9;
   cat::verify(s.a == 9);
   cat::verify(cat::get<0>(s) == 9);
}

$test(magic_get_get_const_lvalue) {
   three_fields const s{.a = 3, .b = 'q', .c = false};
   cat::verify(cat::get<0>(s) == 3);
   cat::verify(cat::get<1>(s) == 'q');
   cat::verify(!cat::get<2>(s));
}

$test(magic_get_get_rvalue) {
   three_fields s{.a = 7, .b = 'r', .c = true};
   cat::int4 x = cat::get<0>(cat::move(s));  // NOLINT
   cat::verify(x == 7);
}

$test(magic_get_apply) {
   three_fields s{.a = 2, .b = 'k', .c = true};
   auto sum = cat::apply(
      [](cat::int4 a, char b, bool c) {
         auto _ = b;
         auto _ = c;
         return a + 1;
      },
      s
   );
   cat::verify(sum == 3);
}

$test(magic_get_not_cat_array) {
   cat::array<int, 2u> a{1, 2};
   static_assert(!cat::has_aggregate_get<decltype(a)>);
   cat::verify(cat::get<0>(a) == 1);
}

$test(magic_get_not_cat_tuple) {
   cat::tuple<int, bool> t{4, true};
   static_assert(!cat::has_aggregate_get<decltype(t)>);
   cat::verify(t.first() == 4);
}

namespace {
struct reference_fields {
   int& lvalue;
   int&& rvalue;
   int value;
};
}  // namespace

// Reference members are returned as declared, for any value category.
$test(magic_get_reference_members) {
   int i = 1;
   int j = 2;
   reference_fields s{.lvalue = i, .rvalue = cat::move(j), .value = 3};
   reference_fields const& c = s;

   static_assert(cat::is_same<decltype(cat::get<0>(s)), int&>);
   static_assert(cat::is_same<decltype(cat::get<0>(c)), int&>);
   static_assert(cat::is_same<decltype(cat::get<0>(cat::move(s))), int&>);
   static_assert(cat::is_same<decltype(cat::get<1>(s)), int&&>);
   static_assert(cat::is_same<decltype(cat::get<2>(s)), int&>);
   static_assert(cat::is_same<decltype(cat::get<2>(c)), int const&>);
   static_assert(cat::is_same<decltype(cat::get<2>(cat::move(s))), int&&>);

   // Not dangling: `i` outlives the temporary aggregate.
   int& r = cat::get<0>(reference_fields{.lvalue = i, .rvalue = 0, .value = 0});
   r = 5;
   cat::verify(i == 5);
}
