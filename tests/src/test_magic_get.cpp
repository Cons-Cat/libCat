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
   three_fields s{1, 'd', true};
   auto& [p0, p1, p2] = s;
   cat::verify(p0 == 1 && p1 == 'd' && p2);
   p0 = -1;
   cat::verify(s.a == -1);
   three_fields s2{3, 'e', false};
   auto [q0, q1, q2] = s2;
   cat::verify(q0 == 3 && q1 == 'e' && !q2);
}

$test(magic_get_get_lvalue) {
   three_fields s{1, 'z', true};
   cat::verify(cat::get<0>(s) == 1);
   cat::verify(cat::get<1>(s) == 'z');
   cat::verify(cat::get<2>(s) == true);
   cat::get<0>(s) = 9;
   cat::verify(s.a == 9);
   cat::verify(cat::get<0>(s) == 9);
}

$test(magic_get_get_const_lvalue) {
   three_fields const s{3, 'q', false};
   cat::verify(cat::get<0>(s) == 3);
   cat::verify(cat::get<1>(s) == 'q');
   cat::verify(cat::get<2>(s) == false);
}

$test(magic_get_get_rvalue) {
   three_fields s{7, 'r', true};
   cat::int4 x = cat::get<0>(cat::move(s));  // NOLINT
   cat::verify(x == 7);
}

$test(magic_get_apply) {
   three_fields s{2, 'k', true};
   auto sum = cat::apply(
      [](cat::int4 a, char b, bool c) {
         auto _ = b;
         auto _ = c;
         return a + 1;
      },
      s);
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
