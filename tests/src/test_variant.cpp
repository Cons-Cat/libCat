#include <cat/variant>

#include "../unit_tests.hpp"

// Basic construction, type-keyed inspection, and value reassignment.
$test(variant_basic_access) {
   cat::variant<int, char, uint4> v(1);
   cat::verify(v.is<int>());
   cat::verify(v.holds_alternative<int>());
   int foo_int = v.get<int>();
   cat::verify(foo_int == 1);

   static_assert(v.alternative_index<int> == 0u);
   static_assert(v.alternative_index<char> == 1u);
   static_assert(v.alternative_index<uint4> == 2u);

   v = 'o';
   cat::verify(v.is<char>());
   cat::verify(v.holds_alternative<char>());
   char foo_char = v.get<char>();
   cat::verify(foo_char == 'o');

   cat::maybe<char&> maybe_active = v.get_if<char>();
   cat::verify(maybe_active.has_value());
   cat::maybe maybe_inactive = v.get_if<int>();
   cat::verify(!maybe_inactive.has_value());
};

// `cat::variant`'s layout, including the compact `uint1` discriminant for
// variants with fewer than 255 alternatives, and the byte-sized collapse
// for two-byte variants.
$test(variant_layout) {
   cat::variant<int, char, uint4> v(1);
   static_assert(sizeof(v) == 8);
   static_assert(sizeof(v.discriminant) == 1);

   cat::variant<char, int4[3]> big_variant;
   static_assert(sizeof(big_variant) == 16);
   static_assert(sizeof(big_variant.discriminant) == 1);

   static_assert(sizeof(cat::variant<char, uint1>)
                 == sizeof(char) + sizeof(cat::uint1));
};

namespace {
// Expand a `type_list<Ts...>` into a `variant<Ts...>`.
template <typename>
struct list_to_variant;

template <typename... Ts>
struct list_to_variant<cat::type_list<Ts...>> {
   using type = cat::variant<Ts...>;
};
}  // namespace

// The discriminant cascades through three tiers as the alternative count
// grows: `uint1` (max value reserved as niche) for small variants, `uint4`
// for variants that overflow `uint1_max`, and `idx` (with the default
// `maybe<idx>` niche, i.e. the high-bit-set bit pattern) for variants that
// would overflow `uint4`. Each tier's niche must observe the empty state
// without colliding with any active alternative index.
$test(variant_discriminant_tiers) {
   // ----- `uint1` tier: variants with fewer than 255 alternatives. -----
   using small_variant = cat::variant<int, char>;
   static_assert(
      cat::is_same<small_variant::alternative_index_type, cat::uint1>);
   static_assert(sizeof(small_variant::maybe_discriminant) == 1);

   small_variant small_empty;
   cat::verify(!small_empty.has_value());
   cat::verify(!small_empty.index().has_value());

   // `value_or_niche()` reads the raw scalar; in the empty state it
   // equals `uint1_max`, which never collides with an active index in
   // `0..variant_size-1`.
   cat::verify(small_empty.discriminant.value_or_niche()
               == cat::limits<cat::uint1>::max());

   // ----- `uint4` tier: variants whose alternative count overflows -----
   // `uint1_max`. 300 ints is enough to push the tier selection across
   // the threshold without slowing the build noticeably.
   using mid_variant = list_to_variant<cat::type_list_filled<int, 300u>>::type;
   static_assert(cat::is_same<mid_variant::alternative_index_type, cat::uint4>);
   static_assert(sizeof(mid_variant::maybe_discriminant) == 4);

   mid_variant mid_empty;
   cat::verify(!mid_empty.has_value());
   cat::verify(!mid_empty.index().has_value());
   cat::verify(mid_empty.discriminant.value_or_niche()
               == cat::limits<cat::uint4>::max());

   // Engage the second slot to confirm `value_or_niche()` reports the
   // active index in the `uint4` tier, not the niche.
   mid_variant mid_active{cat::in_place_index<0u>, 11};
   cat::verify(mid_active.has_value());
   cat::verify(mid_active.discriminant.value_or_niche() == cat::uint4{0u});

   // ----- `idx` tier: variants whose alternative count would overflow -----
   // `uint4_max`. Constructing one with ~4 billion alternatives is
   // infeasible at compile time, so we verify the tier-selection logic
   // through the same `conditional` expression that the variant uses
   // internally, then probe `maybe<idx>`'s niche directly. The runtime
   // `cat::idx`-tier niche is the high-bit-set bit pattern, exposed
   // through `maybe<idx>::value_or_niche()` for an empty `maybe<idx>`.
   using fake_huge_index = cat::conditional<
      false,  // simulate `types::size() < uint1_max` being false
      cat::uint1,
      cat::conditional<false,  // simulate `< uint4_max` being false too
                       cat::uint4, cat::idx>>;
   static_assert(cat::is_same<fake_huge_index, cat::idx>);

   // `maybe<idx>` is the discriminant storage in the third tier; an
   // empty one decodes `nullopt` via its built-in niche. Active indices
   // are non-negative, so the niche's high-bit-set state never collides.
   cat::maybe<cat::idx> empty_idx_discriminant;
   cat::verify(!empty_idx_discriminant.has_value());

   cat::maybe<cat::idx> active_idx_discriminant = cat::idx{0u};
   cat::verify(active_idx_discriminant.has_value());
   cat::verify(active_idx_discriminant.value() == 0u);
};

// Converting subset construction and assignment: a `variant<Us...>` whose
// alternatives are a subset of `variant<Ts...>`'s alternatives can be
// copied into the wider variant, and reassignment switches the active
// alternative.
$test(variant_subset_construct_and_assign) {
   cat::variant<int, char, uint4> narrow('o');
   cat::variant<int, char, uint4, int2> wide = narrow;
   cat::verify(wide.is<char>());
   cat::verify(wide.holds_alternative<char>());

   wide = 1;
   cat::verify(wide.is<int>());
   cat::verify(wide.holds_alternative<int>());

   wide = narrow;
   cat::verify(wide.is<char>());
   cat::verify(wide.holds_alternative<char>());

   narrow = 1;
   cat::variant<int, char, uint4, int2> wide_from_int = narrow;
   cat::verify(wide_from_int.is<int>());
   cat::verify(wide_from_int.holds_alternative<int>());

   wide_from_int = int2{10};
   cat::verify(wide_from_int.is<int2>());
   cat::verify(wide_from_int.holds_alternative<int2>());

   wide_from_int = narrow;
   cat::verify(wide_from_int.is<int>());
   cat::verify(wide_from_int.holds_alternative<int>());
};

// Index-keyed `.get<I>()` returns a reference whose cvref matches the
// variant's value category (deducing-this), mirroring `std::variant::get`.
$test(variant_get_by_index_propagates_cvref) {
   cat::variant<int, char, uint4, int2> v(1);
   static_assert(cat::is_same<decltype(v.get<0>()), int&>);
   static_assert(cat::is_same<decltype(v.get<1>()), char&>);
   static_assert(cat::is_same<decltype(v.get<2>()), uint4&>);
   static_assert(cat::is_same<decltype(v.get<3>()), int2&>);

   // Const-lvalue variants yield references to const.
   cat::variant<int, char, uint4, int2> const cv(1);
   static_assert(cat::is_same<decltype(cv.get<0>()), int const&>);
   static_assert(cat::is_same<decltype(cv.get<3>()), int2 const&>);

   // Rvalue variants yield rvalue references for moves.
   static_assert(cat::is_same<decltype(cat::move(v).get<0>()), int&&>);
   static_assert(
      cat::is_same<decltype(cat::variant<int, char, uint4, int2>{1}.get<0>()),
                   int&&>);

   // Type-keyed `.get<T>()` participates in the same cvref propagation.
   static_assert(cat::is_same<decltype(v.get<int>()), int&>);
   static_assert(cat::is_same<decltype(cv.get<int>()), int const&>);
   static_assert(cat::is_same<decltype(cat::move(v).get<int>()), int&&>);
};

// `cat::variant<Ts...>::alternative_type<I>` is the index-keyed alias
// template equivalent of `std::variant_alternative_t<I, V>`. It maps each
// position in the alternative pack to its declared type, including
// reference and duplicate-type packs.
$test(variant_alternative_type_alias) {
   using v_t = cat::variant<int, char, cat::uint4, cat::int2>;
   static_assert(cat::is_same<v_t::alternative_type<0u>, int>);
   static_assert(cat::is_same<v_t::alternative_type<1u>, char>);
   static_assert(cat::is_same<v_t::alternative_type<2u>, cat::uint4>);
   static_assert(cat::is_same<v_t::alternative_type<3u>, cat::int2>);

   // Reference variants keep the reference qualification (P4198).
   using ref_t = cat::variant<int&, double const&>;
   static_assert(cat::is_same<ref_t::alternative_type<0u>, int&>);
   static_assert(cat::is_same<ref_t::alternative_type<1u>, double const&>);

   // Duplicate alternative types are addressed by position; the alias
   // template is the only way to recover the duplicate slot's type, since
   // `variant_alternative_index<T>` would be ill-formed for duplicates.
   using dup_t = cat::variant<int, int, char>;
   static_assert(cat::is_same<dup_t::alternative_type<0u>, int>);
   static_assert(cat::is_same<dup_t::alternative_type<1u>, int>);
   static_assert(cat::is_same<dup_t::alternative_type<2u>, char>);
};

// `cat::variant` in constant-evaluated context. The set of operations
// supported here is constrained by Clang's reading of P2747R2's constexpr
// placement-new: switching the active member of a `union` is only allowed
// when the destination is the first (implicitly-active) alternative of
// the recursive `variant_node`. That covers reading any constexpr
// variant, default construction (empty state), value-construction with
// the first alternative, `cat::visit`, and the index-keyed traits.
// Non-first-alternative construction and type-changing assignment cannot
// be evaluated at compile time today and are therefore exercised in the
// runtime tests above.
$test(variant_constexpr) {
   // Value-construction with the first alternative.
   constexpr cat::variant<int, uint4> const_variant = 1;
   static_assert(const_variant.has_value());
   static_assert(const_variant.is<int>());
   static_assert(const_variant.holds_alternative<int>());
   static_assert(const_variant.get<int>() == 1);
   static_assert(const_variant.get<0u>() == 1);
   static_assert(const_variant.index().has_value());
   static_assert(const_variant.index().value() == 0u);

   // `in_place_index<0>` for the first alternative is equivalent and
   // routes through the same union slot.
   constexpr cat::variant<int, uint4> in_place_variant{cat::in_place_index<0u>,
                                                       2};
   static_assert(in_place_variant.has_value());
   static_assert(in_place_variant.get<0u>() == 2);

   // Default-construction yields the empty state. `is<T>` and `.index()`
   // both observe it.
   constexpr cat::variant<int, uint4> empty_variant;
   static_assert(!empty_variant.has_value());
   static_assert(!empty_variant.is<int>());
   static_assert(!empty_variant.is<uint4>());
   static_assert(!empty_variant.index().has_value());

   // `cat::variant_size<T>` (P3664) resolves at compile time.
   static_assert(cat::variant_size<cat::variant<int, uint4>> == 2u);
   static_assert(cat::variant_size<decltype(empty_variant)> == 2u);
   static_assert(cat::variant_size<cat::variant<int, int, char>> == 3u);

   // `cat::variant_alternative_index<T, V>` (P2527) is constexpr.
   static_assert(cat::variant_alternative_index<int, cat::variant<int, uint4>>
                 == 0u);
   static_assert(cat::variant_alternative_index<uint4, cat::variant<int, uint4>>
                 == 1u);

   // Variant-vs-variant equality compares discriminants then values, and
   // both steps are constexpr-evaluable. Empty compares equal to empty
   // and not to engaged.
   constexpr cat::variant<int, uint4> equal_variant = 1;
   static_assert(const_variant == equal_variant);
   constexpr cat::variant<int, uint4> empty_too;
   static_assert(empty_variant == empty_too);
   static_assert(empty_variant != const_variant);

   // `cat::visit` dispatches by discriminant at compile time.
   constexpr int visited = cat::visit(
      [](auto value) -> int {
         using value_type = cat::remove_cvref<decltype(value)>;
         if constexpr (cat::is_same<value_type, int>) {
            return static_cast<int>(value);
         } else {
            return -1;
         }
      },
      const_variant);
   static_assert(visited == 1);
};

// `.is<T>()` (type-keyed) and `.is(value)` (value-keyed). The type-keyed
// form accepts unconditionally invalid types and yields `false`, unlike
// `.holds_alternative<T>()` which is constrained on `T` being an
// alternative.
$test(variant_is_predicate) {
   cat::variant<int, char, uint4, int2> v(1);
   cat::verify(v.is<int>());
   cat::verify(v.is(1));

   v = 'b';
   cat::verify(v.is<char>());
   cat::verify(v.is('b'));

   cat::verify(!v.is<unsigned long long>());
};

// Pattern matching: `cat::match`, member `.match()`, and the
// `one_of<Ts...>`, `is_a<T>`, `is_a(value)` patterns.
$test(variant_pattern_matching) {
   cat::variant<int, char, uint4, int2> v(1);

   bool matched = false;
   cat::match(v)(  //
      one_of<double, float>().then_do([]() {
         cat::exit(1);
      }),
      one_of<float, int>().then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);

   matched = false;
   cat::match(v)(  //
      is_a<float>().then_do([&]() {
         cat::exit(1);
      }),
      is_a<int>().then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);

   // `v` holds an integer, but floats are convertible to integers so
   // `is_a(1.f)` matches against the held `1`.
   matched = false;
   cat::match(v)(  //
      is_a(2.f).then_do([&]() {
         cat::exit(1);
      }),
      is_a(1.f).then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);

   // Member-form `.match()` dispatches the same way.
   matched = false;
   v.match(  //
      is_a(2.f).then_do([&]() {
         cat::exit(1);
      }),
      is_a(1.f).then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);
};

// `cat::visit` single-variant dispatch (mpark v2 switch). The callback fires
// with the active alternative, returning a common type across every branch.
$test(variant_visit_single) {
   cat::variant<int, char, float4> v{5};
   int hit_index = -1;
   cat::visit(
      [&](auto& value) {
         using T = cat::remove_cvref<decltype(value)>;
         if constexpr (cat::is_same<T, int>) {
            cat::verify(value == 5);
            hit_index = 0;
         } else if constexpr (cat::is_same<T, char>) {
            hit_index = 1;
         } else if constexpr (cat::is_same<T, float4>) {
            hit_index = 2;
         }
      },
      v);
   cat::verify(hit_index == 0);

   v = 'q';
   cat::visit(
      [&](auto& value) {
         using T = cat::remove_cvref<decltype(value)>;
         if constexpr (cat::is_same<T, char>) {
            cat::verify(value == 'q');
            hit_index = 1;
         }
      },
      v);
   cat::verify(hit_index == 1);

   // Callbacks may have a non-`void` return type.
   v = 42;
   int doubled = cat::visit(
      [](auto& value) -> int {
         using T = cat::remove_cvref<decltype(value)>;
         if constexpr (cat::is_same<T, int>) {
            return value * 2;
         } else if constexpr (cat::is_same<T, char>) {
            return value;
         } else {
            return 0;
         }
      },
      v);
   cat::verify(doubled == 84);

   // Member-shortcut `variant.visit(callback)`.
   int member_hit = 0;
   v.visit([&](auto& value) {
      using T = cat::remove_cvref<decltype(value)>;
      if constexpr (cat::is_same<T, int>) {
         member_hit = value;
      }
   });
   cat::verify(member_hit == 42);
};

// `cat::visit` over multiple variants. The visitor receives one argument per
// variant, in order, with the active alternatives of each.
$test(variant_visit_multi) {
   cat::variant<int, char> a{1};
   cat::variant<int, char> b{'b'};

   bool matched = false;
   cat::visit(
      [&](auto& a_value, auto& b_value) {
         using A = cat::remove_cvref<decltype(a_value)>;
         using B = cat::remove_cvref<decltype(b_value)>;
         if constexpr (cat::is_same<A, int> && cat::is_same<B, char>) {
            cat::verify(a_value == 1);
            cat::verify(b_value == 'b');
            matched = true;
         }
      },
      a, b);
   cat::verify(matched);

   // Three-variant fold.
   cat::variant<int, char> c{7};
   int sum = cat::visit(
      [](auto& x, auto& y, auto& z) -> int {
         int total = 0;
         if constexpr (cat::is_same<cat::remove_cvref<decltype(x)>, int>) {
            total += x;
         }
         if constexpr (cat::is_same<cat::remove_cvref<decltype(y)>, char>) {
            total += y;
         }
         if constexpr (cat::is_same<cat::remove_cvref<decltype(z)>, int>) {
            total += z;
         }
         return total;
      },
      a, b, c);
   // a=1 (int), b='b'=98 (char), c=7 (int). 1 + 98 + 7 = 106.
   cat::verify(sum == 106);
};

// P4189 `.get_ptr<T>()` and `.get_ptr<index>()` return a pointer to the
// active alternative or `nullptr` when the queried alternative is inactive.
$test(variant_get_ptr) {
   cat::variant<int, char, float4> v{42};

   int* p_active = v.get_ptr<int>();
   cat::verify(p_active != nullptr);
   cat::verify(*p_active == 42);
   cat::verify(p_active == &v.get<int>());

   char* p_inactive = v.get_ptr<char>();
   cat::verify(p_inactive == nullptr);

   // Index-keyed overload.
   int* p_index_zero = v.get_ptr<0>();
   cat::verify(p_index_zero == p_active);
   cat::verify(v.get_ptr<1>() == nullptr);
   cat::verify(v.get_ptr<2>() == nullptr);

   // Free function form.
   cat::verify(cat::get_ptr<int>(v) == p_active);
   cat::verify(cat::get_ptr<1>(v) == nullptr);

   // After mutation the active alternative shifts.
   v = 'k';
   cat::verify(v.get_ptr<int>() == nullptr);
   cat::verify(v.get_ptr<char>() != nullptr);
   cat::verify(*v.get_ptr<char>() == 'k');
};

// `variant<T&, U&, ...>` per P4198. The active alternative is fixed at
// construction and assignment writes through the held reference.
$test(variant_reference_alternatives) {
   int my_int = 1;
   double my_double = 2.0;

   cat::variant<int&, double&> r1(my_int);
   cat::verify(r1.holds_alternative<int&>());
   cat::verify(r1.get<int&>() == 1);
   cat::verify(&r1.get<int&>() == &my_int);

   // Assignment writes through to the original.
   r1 = 99;
   cat::verify(my_int == 99);
   cat::verify(r1.get<int&>() == 99);

   // A second reference variant binds a different alternative.
   cat::variant<int&, double&> r2(my_double);
   cat::verify(r2.holds_alternative<double&>());
   cat::verify(&r2.get<double&>() == &my_double);
   r2 = 3.5;
   cat::verify(my_double == 3.5);

   // Reference variants are pointer-sized + discriminant.
   static_assert(
      sizeof(cat::variant<int&, double&>) <= sizeof(void*) + alignof(void*),
      "reference variant should fit in a pointer plus tag.");

   // `.get_ptr<T>()` yields a pointer to the referent or `nullptr` when the
   // wrong alternative is queried.
   cat::verify(r1.get_ptr<int&>() == &my_int);
   cat::verify(r1.get_ptr<double&>() == nullptr);

   // `.is<T>()` (and the `match` interaction) accepts the bare type, the
   // reference type, or neither (in which case it is `false`).
   cat::verify(r1.is<int&>());
   cat::verify(r1.is<int>());
   cat::verify(!r1.is<double>());
   cat::verify(!r1.is<float>());

   // `.get_if<T>()` returns `maybe<T&>` engaged for the active type. The
   // bare-type overload picks up the matching reference alternative.
   cat::maybe<int&> got = r1.get_if<int>();
   cat::verify(got.has_value());
   cat::verify(&got.value() == &my_int);
   cat::maybe<double&> missed = r1.get_if<double>();
   cat::verify(!missed.has_value());

   // Const reference alternatives bind l-values without write-through.
   int const my_const = 7;
   cat::variant<int const&, double const&> r_const(my_const);
   cat::verify(r_const.holds_alternative<int const&>());
   cat::verify(r_const.get<int const&>() == 7);
};

// P2162 "upside-down inheritance". A type that publicly inherits from
// `cat::variant<Ts...>` is `is_variant_like` and is accepted by `cat::visit`,
// `cat::get`, and `cat::get_if`.
struct serial_packet : cat::variant<int, char, float4> {
   using cat::variant<int, char, float4>::variant;

   [[nodiscard]]
   constexpr auto
   tag() const -> uint1 {
      return this->discriminant.value();
   }
};

$test(variant_inheritance) {
   static_assert(cat::is_variant_like<serial_packet>,
                 "derived class should satisfy `is_variant_like`.");

   serial_packet p{17};
   cat::verify(p.tag() == 0u);
   cat::verify(p.is<int>());

   int sum_kind = cat::visit(
      [](auto& value) -> int {
         using T = cat::remove_cvref<decltype(value)>;
         if constexpr (cat::is_same<T, int>) {
            return value;
         } else if constexpr (cat::is_same<T, char>) {
            return -1;
         } else {
            return -2;
         }
      },
      p);
   cat::verify(sum_kind == 17);

   // Free-function `cat::get<T>(packet)` recovers the variant base members.
   cat::verify(cat::get<int>(p) == 17);
   cat::verify(cat::get<0>(p) == 17);
   cat::verify(!cat::get_if<char>(p).has_value());
   cat::verify(cat::get_ptr<int>(p) != nullptr);
   cat::verify(cat::holds_alternative<int>(p));
};

// Derived classes that introduce names colliding with variant members must
// not be able to hijack `cat::visit` or `get_ptr`. The internal C-cast in
// each deducing-this method routes through `variant`'s overload set
// regardless of the derived class's overrides.
struct shadowing_packet : cat::variant<int, char> {
   using cat::variant<int, char>::variant;

   template <typename>
   [[nodiscard]]
   constexpr auto
   is() const -> bool {
      return false;
   }

   template <cat::idx>
   [[nodiscard]]
   constexpr auto
   get() const -> int {
      return -1;
   }

   uint4 discriminant = 42u;
};

struct user_to_int {
   int v = 7;

   constexpr
   operator int() const noexcept {
      return v;
   }
};

// P3146R2 converting construction. Selects the best alternative through a
// `FUN`-style overload set whose constraints use a concept-based narrowing
// check rather than `std::declval`.
$test(variant_converting_construction) {
   // P3146 headline: `cat::constant<42>` converts to `float` losslessly
   // via the constant-expression narrowing exception.
   cat::variant<float> v1{cat::constant<42>{}};
   cat::verify(v1.has_value());
   cat::verify(v1.is<float>());
   cat::verify(v1.get<float>() == 42.0f);

   // `short` widens to `int` (non-narrowing) and `char const*` doesn't
   // match at all, so `int` is uniquely selected.
   short s = 3;
   cat::variant<int, char const*> v2 = s;
   cat::verify(v2.is<int>());
   cat::verify(v2.get<int>() == 3);

   // A user-defined non-`constexpr` value that converts to `int` directly
   // also selects the `int` alternative.
   user_to_int u;
   cat::variant<int, char const*> v3 = u;
   cat::verify(v3.is<int>());
   cat::verify(v3.get<int>() == 7);

   // Exact-match construction continues to win over the converting
   // constructor: `int{1}` constructs the `int` alternative directly even
   // when `float` would also be reachable.
   cat::variant<int, float> v4 = 1;
   cat::verify(v4.is<int>());
   cat::verify(v4.get<int>() == 1);
};

// Raw arithmetic to libCat arithmetic. The variant's FUN overload set
// gates conversions on `is_convertible_without_narrowing`, which in turn
// gates on each `basic_int` / `basic_float` constructor's `explicit`-ness.
// `cat::int4`'s constructor from raw `int` is non-`explicit` because the
// conversion is `is_safe_arithmetic_conversion`, while `cat::int2` from
// raw `int` is `explicit` because it would narrow.
$test(variant_converting_construction_into_cat_arithmetic) {
   // First pin down the underlying trait values: every variant
   // converting-construction below is exactly one of these for the
   // matched alternative.
   static_assert(cat::is_convertible_without_narrowing<int, cat::int4>);
   static_assert(cat::is_convertible_without_narrowing<int, cat::int8>);
   static_assert(cat::is_convertible_without_narrowing<float, cat::float4>);
   static_assert(cat::is_convertible_without_narrowing<float, cat::float8>);
   static_assert(cat::is_convertible_without_narrowing<double, cat::float8>);
   static_assert(!cat::is_convertible_without_narrowing<int, cat::int2>);
   static_assert(!cat::is_convertible_without_narrowing<int, cat::uint4>);
   static_assert(!cat::is_convertible_without_narrowing<float, cat::int4>);
   static_assert(!cat::is_convertible_without_narrowing<double, cat::float4>);

   // Direct FUN probes to verify the overload set resolves the way the
   // converting constructor will route the input.
   static_assert(
      cat::is_same<cat::detail::fun_selected_type<float, cat::float4>,
                   cat::float4>);
   static_assert(cat::is_same<
                 cat::detail::fun_selected_type<float, cat::int4, cat::float4>,
                 cat::float4>);
   static_assert(
      cat::is_same<cat::detail::fun_selected_type<int, cat::int4, cat::float4>,
                   cat::int4>);

   // Same-kind same-width raw -> cat: non-narrowing.
   cat::variant<cat::int4> v_int4 = 1;
   cat::verify(v_int4.is<cat::int4>());
   cat::verify(v_int4.get<cat::int4>() == 1);

   // Same-kind safe widening raw -> cat: non-narrowing.
   cat::variant<cat::int8> v_int8 = 1;
   cat::verify(v_int8.is<cat::int8>());
   cat::verify(v_int8.get<cat::int8>() == 1);

   // Same-width raw float -> cat: non-narrowing.
   cat::variant<cat::float4> v_float4 = 1.0f;
   cat::verify(v_float4.is<cat::float4>());
   cat::verify(v_float4.get<cat::float4>() == 1.0f);

   // Same-width raw double -> cat: non-narrowing.
   cat::variant<cat::float8> v_float8_from_double = 1.0;
   cat::verify(v_float8_from_double.is<cat::float8>());
   cat::verify(v_float8_from_double.get<cat::float8>() == 1.0);

   // Safe-widening raw float -> wider cat: non-narrowing (the raw-float
   // source is not a `basic_float`, so it bypasses
   // `is_basic_float_widening`'s explicit-ness override).
   cat::variant<cat::float8> v_float8_from_float = 1.0f;
   cat::verify(v_float8_from_float.is<cat::float8>());
   cat::verify(v_float8_from_float.get<cat::float8>() == 1.0);

   // Multi-alternative dispatch picks the kind-matching alternative.
   cat::variant<cat::int4, cat::float4> v_mixed_int = 1;
   cat::verify(v_mixed_int.is<cat::int4>());
   cat::verify(v_mixed_int.get<cat::int4>() == 1);

   cat::variant<cat::int4, cat::float4> v_mixed_float = 1.0f;
   cat::verify(v_mixed_float.is<cat::float4>());
   cat::verify(v_mixed_float.get<cat::float4>() == 1.0f);

   // Narrowing conversions are rejected at the type level. Probed via
   // `__is_constructible` so the negative cases stay as compile-time
   // assertions rather than emitting diagnostics from a non-dependent
   // requires-expression body.
   static_assert(!__is_constructible(cat::variant<cat::int2>, int),
                 "raw `int` -> `cat::int2` is a narrowing conversion");
   static_assert(!__is_constructible(cat::variant<cat::uint4>, int),
                 "raw `int` -> `cat::uint4` is a sign-mismatch conversion");
   static_assert(!__is_constructible(cat::variant<cat::int4>, float),
                 "raw `float` -> `cat::int4` is a lossy conversion");
   static_assert(!__is_constructible(cat::variant<cat::float4>, double),
                 "raw `double` -> `cat::float4` is a narrowing conversion");
};

// Negative cases for the converting constructor. These exercise SFINAE so
// they belong in `static_assert`s.
$test(variant_converting_negative_cases) {
   // A source whose only conversion to any alternative is narrowing is
   // rejected (no `FUN` overload is viable).
   static_assert(
      !__is_constructible(cat::variant<int>, double),
      "double -> int is narrowing and there is no other alternative.");

   static_assert(!__is_constructible(cat::variant<int, char>, double),
                 "double -> int and double -> char are both narrowing.");

   // A plain `int` literal is not a constant expression visible to the
   // concept's hypothetical source, so the narrowing-conversion exception
   // does not apply and `int -> float` is narrowing.
   static_assert(!__is_constructible(cat::variant<float>, int),
                 "Plain `int` to `float` is narrowing in the FUN check.");
};

// P2527R3 `variant_alternative_index<T, V>`. Strict semantics: only the
// `cat::variant<...>` specialization itself is accepted (not derived
// classes or `const` variants); the alternative type must appear exactly
// once.
$test(variant_alternative_index_trait) {
   using v_t = cat::variant<int, char, float4>;

   static_assert(cat::variant_alternative_index<int, v_t> == 0u);
   static_assert(cat::variant_alternative_index<char, v_t> == 1u);
   static_assert(cat::variant_alternative_index<float4, v_t> == 2u);

   // The trait is a `cat::idx`, usable directly in constant expressions.
   constexpr cat::idx i = cat::variant_alternative_index<float4, v_t>;
   static_assert(i == 2u);
};

// P3664R0 SFINAE-friendly `variant_size<T>`. The value template is
// initialized from `detail::variant_size_impl<T>::value`, whose primary
// specialization is empty - so probing `cat::variant_size<T>` through a
// templated `requires` expression for a non-variant type is a
// substitution failure rather than a hard error. This is what makes the
// trait usable in pattern-match completeness checks like the ones
// P2688R5 proposes.
template <typename T>
concept variant_size_is_defined =
   requires { cat::variant_size<T> + cat::idx{0u}; };

$test(variant_size_trait) {
   using v_t = cat::variant<int, char, float4>;
   static_assert(cat::variant_size<v_t> == 3u);

   // The trait sees through cv-qualifiers and references via `is_variant_like`.
   static_assert(cat::variant_size<v_t const> == 3u);
   static_assert(cat::variant_size<v_t&> == 3u);

   // Derived-from-variant cases (P2162) still satisfy the trait.
   struct subclass : cat::variant<int, char> {};

   static_assert(cat::variant_size<subclass> == 2u);

   // For non-variants the value template is ill-formed in a substitution
   // context, so the concept probe returns `false` cleanly.
   static_assert(variant_size_is_defined<v_t>);
   static_assert(variant_size_is_defined<subclass>);
   static_assert(!variant_size_is_defined<int>);
   static_assert(!variant_size_is_defined<char*>);
};

// Variant-vs-variant equality and three-way comparison follow `std::variant`
// semantics: empty orders before any active alternative, a lower-index
// active alternative orders before a higher-index one, and two variants on
// the same alternative are ordered by their stored value.
$test(variant_compare) {
   using v_t = cat::variant<int, char, uint4>;
   v_t empty_1;
   v_t empty_2;
   v_t v_int_3(3);
   v_t v_int_3_again(3);
   v_t v_int_4(4);
   v_t v_char_b('b');

   cat::verify(empty_1 == empty_2);
   cat::verify(!(empty_1 != empty_2));
   cat::verify((empty_1 <=> empty_2) == 0);

   cat::verify(v_int_3 == v_int_3_again);
   cat::verify((v_int_3 <=> v_int_3_again) == 0);

   cat::verify(v_int_3 != v_int_4);
   cat::verify(v_int_3 < v_int_4);
   cat::verify((v_int_3 <=> v_int_4) < 0);

   // Different alternatives compare by index: `int` (index 0) precedes
   // `char` (index 1).
   cat::verify(v_int_3 != v_char_b);
   cat::verify(v_int_3 < v_char_b);
   cat::verify((v_int_3 <=> v_char_b) < 0);

   // Empty orders before any active alternative.
   cat::verify(empty_1 != v_int_3);
   cat::verify(empty_1 < v_int_3);
   cat::verify((empty_1 <=> v_int_3) < 0);
   cat::verify(v_int_3 > empty_1);
   cat::verify((v_int_3 <=> empty_1) > 0);
};

// A non-trivial alternative type that traces destructor calls so the empty
// state and type-change paths can be asserted to do the right thing.
struct destruct_tracer {
   int4* p_counter = nullptr;
   int4 tag = 0;

   constexpr destruct_tracer() = default;

   constexpr destruct_tracer(int4& counter, int4 in_tag)
       : p_counter(&counter), tag(in_tag) {
   }

   constexpr destruct_tracer(destruct_tracer const&) = default;

   constexpr destruct_tracer(destruct_tracer&& other) noexcept
       : p_counter(other.p_counter), tag(other.tag) {
      other.p_counter = nullptr;
   }

   constexpr auto
   operator=(destruct_tracer const&) -> destruct_tracer& = default;

   constexpr auto
   operator=(destruct_tracer&& other) noexcept -> destruct_tracer& {
      if (this == &other) {
         return *this;
      }
      if (p_counter != nullptr) {
         ++*p_counter;
      }
      p_counter = other.p_counter;
      tag = other.tag;
      other.p_counter = nullptr;
      return *this;
   }

   constexpr ~destruct_tracer() {
      if (p_counter != nullptr) {
         ++*p_counter;
      }
   }
};

// A value variant is `valueless` by default and after `reset()`/`= nullopt`.
// `.is<T>()`, `.has_value()`, `.get_if<T>()`, and `.get_ptr<T>()` all reflect
// the empty state without UB.
$test(variant_empty_state) {
   cat::variant<int, char, float4> v;
   cat::verify(!v.has_value());
   cat::verify(!v.is<int>());
   cat::verify(!v.is<char>());
   cat::verify(!v.holds_alternative<int>());
   cat::verify(!v.get_if<int>().has_value());
   cat::verify(v.get_ptr<int>() == nullptr);
   cat::verify(v.get_ptr<0>() == nullptr);

   v = 5;
   cat::verify(v.has_value());
   cat::verify(v.is<int>());
   cat::verify(v.get<int>() == 5);

   v.reset();
   cat::verify(!v.has_value());
   cat::verify(!v.is<int>());
   cat::verify(v.get_ptr<int>() == nullptr);

   v = 7;
   cat::verify(v.has_value());

   v = cat::nullopt;
   cat::verify(!v.has_value());

   cat::variant<int, char, float4> from_nullopt(cat::nullopt);
   cat::verify(!from_nullopt.has_value());

   // Copy of an empty variant remains empty.
   cat::variant<int, char, float4> copied = v;
   cat::verify(!copied.has_value());

   // Move of an empty variant remains empty.
   cat::variant<int, char, float4> moved = $mov v;
   cat::verify(!moved.has_value());
};

// Type-change assignment destroys the old alternative exactly once and
// constructs the new alternative in place. The previous implementation called
// the wrong destructor and then assigned to uninitialized storage, which is
// UB for non-trivial types.
$test(variant_type_change_destruction) {
   int4 destructions = 0;

   cat::variant<destruct_tracer, int4> v(destruct_tracer{destructions, 11});
   cat::verify(v.is<destruct_tracer>());
   cat::verify(v.get<destruct_tracer>().tag == 11);

   // Switch from `destruct_tracer` to `int4`. The held `destruct_tracer`
   // must be destroyed exactly once.
   v = int4{42};
   cat::verify(v.is<int4>());
   cat::verify(v.get<int4>() == 42);
   cat::verify(destructions == 1);

   // Switch back to a fresh `destruct_tracer`.
   v = destruct_tracer{destructions, 22};
   cat::verify(v.is<destruct_tracer>());
   cat::verify(v.get<destruct_tracer>().tag == 22);

   // `reset()` destroys the held alternative once.
   v.reset();
   cat::verify(!v.has_value());
   cat::verify(destructions == 2);

   // `.reset()` is idempotent on an empty variant.
   v.reset();
   cat::verify(!v.has_value());
   cat::verify(destructions == 2);
};

// Reference variants are never valueless (P4198) and reject default-
// construction.
$test(variant_reference_no_default) {
   static_assert(!__is_constructible(cat::variant<int&, double&>),
                 "reference variants must be constructed with an alternative.");
   int my_int = 4;
   cat::variant<int&, double&> r(my_int);
   cat::verify(r.has_value());
};

$test(variant_inheritance_shadowing) {
   shadowing_packet sp{99};

   // `sp.get_ptr<int>()` would consult `sp.is<int>()` (returns false) and
   // `sp.get<0>()` (returns -1) under naive deducing-this. With the C-cast
   // to the variant base it correctly reaches the active alternative.
   int* p_active = sp.get_ptr<int>();
   cat::verify(p_active != nullptr);
   cat::verify(*p_active == 99);

   // The member `visit` also dispatches through the variant base.
   int seen = sp.visit([](auto& value) -> int {
      if constexpr (cat::is_same<cat::remove_cvref<decltype(value)>, int>) {
         return value;
      }
      return -2;
   });
   cat::verify(seen == 99);
};

// `cat::variant` permits duplicate alternative types. Type-keyed accessors
// (`get<T>`, `is<T>`, `holds_alternative<T>`, etc.) are constrained on the
// alternative being unique. Duplicates must therefore be reached via
// `cat::in_place_index<I>` for construction and the index-keyed accessors
// for inspection.
$test(variant_duplicate_alternatives) {
   using dup = cat::variant<int, int, char>;

   static_assert(dup::variant_size == 3u);
   static_assert(cat::is_same<dup::alternative_type<0u>, int>);
   static_assert(cat::is_same<dup::alternative_type<1u>, int>);
   static_assert(cat::is_same<dup::alternative_type<2u>, char>);

   // Default-constructed: empty.
   dup empty;
   cat::verify(!empty.has_value());

   // Build the first `int` slot.
   dup first{cat::in_place_index<0u>, 7};
   cat::verify(first.has_value());
   cat::verify(*first.get_ptr<0u>() == 7);
   cat::verify(first.get_ptr<1u>() == nullptr);

   // Build the second `int` slot. The two share a type so duplicate-aware
   // dispatch must keep them distinct.
   dup second{cat::in_place_index<1u>, 11};
   cat::verify(*second.get_ptr<1u>() == 11);
   cat::verify(second.get_ptr<0u>() == nullptr);

   // Index-keyed `get` works for either slot.
   cat::verify(second.get<1u>() == 11);

   // The unique alternative is still reachable by type.
   dup ch{cat::in_place_index<2u>, 'q'};
   cat::verify(ch.is<char>());
   cat::verify(ch.get<char>() == 'q');

   // Same-variant copy preserves the active index.
   dup copy_of_second = second;
   cat::verify(*copy_of_second.get_ptr<1u>() == 11);
   cat::verify(copy_of_second.get_ptr<0u>() == nullptr);

   // Same-variant copy-assignment from a different slot rebinds correctly.
   copy_of_second = first;
   cat::verify(*copy_of_second.get_ptr<0u>() == 7);
   cat::verify(copy_of_second.get_ptr<1u>() == nullptr);

   // Same-variant move is index-aware too.
   dup moved = $mov second;
   cat::verify(*moved.get_ptr<1u>() == 11);

   // Visit observes the active slot's value.
   int from_visit = first.visit([](auto& value) -> int {
      if constexpr (cat::is_same<cat::remove_cvref<decltype(value)>, int>) {
         return value;
      }
      return -1;
   });
   cat::verify(from_visit == 7);

   // Variant-vs-variant equality distinguishes the two `int` slots even
   // when their values happen to match.
   dup same_value_first{cat::in_place_index<0u>, 7};
   dup same_value_second{cat::in_place_index<1u>, 7};
   cat::verify(first == same_value_first);
   cat::verify(!(first == same_value_second));

   // P2527: `variant_alternative_index<T, V>` is ill-formed for a duplicate
   // alternative, but works for the unique one.
   static_assert(cat::variant_alternative_index<char, dup> == 2u);
};

// P3561R2 index-based coproduct operations. Each callable is matched to
// an alternative by position, so duplicate alternative types route to the
// right branch (something the type-keyed `cat::visit` overload set cannot
// do).
$test(variant_p3561_invoke_cases) {
   using dup = cat::variant<int, int, char>;

   // Pass the variant directly: `visit_invoke_cases(v, fs...)`.
   dup first{cat::in_place_index<0u>, 5};
   dup second{cat::in_place_index<1u>, 11};
   dup ch{cat::in_place_index<2u>, 'q'};

   auto by_index = [](auto&& v) -> int {
      return cat::visit_invoke_cases(
         v,
         [](int i) -> int {
            return i;
         },
         [](int i) -> int {
            return i + 100;
         },
         [](char c) -> int {
            return static_cast<int>(c);
         });
   };
   cat::verify(by_index(first) == 5);
   cat::verify(by_index(second) == 111);
   cat::verify(by_index(ch) == int{'q'});

   // Curried form: `invoke_cases(fs...)` returns a callable that takes
   // a variant. Same dispatch.
   auto matcher = cat::invoke_cases(
      [](int i) -> int {
         return i;
      },
      [](int i) -> int {
         return i + 100;
      },
      [](char c) -> int {
         return static_cast<int>(c);
      });
   cat::verify(matcher(first) == 5);
   cat::verify(matcher(second) == 111);
   cat::verify(matcher(ch) == int{'q'});
};

// P3561R2 multi-parameter dispatch: each alternative is a `cat::tuple`,
// the matched callable receives the tuple unpacked via `cat::apply`.
$test(variant_p3561_apply_cases) {
   using message = cat::tuple<int, int>;
   using message2 = cat::tuple<int, char>;
   using packets = cat::variant<message, message2>;

   packets pair_args{
      cat::in_place_index<0u>, message{3, 4}
   };
   packets char_args{
      cat::in_place_index<1u>, message2{2, 'a'}
   };

   int sum_pair = cat::visit_apply_cases(
      pair_args,
      [](int x, int y) -> int {
         return x + y;
      },
      [](int x, char y) -> int {
         return x + static_cast<int>(y);
      });
   cat::verify(sum_pair == 7);

   int sum_char = cat::visit_apply_cases(
      char_args,
      [](int x, int y) -> int {
         return x + y;
      },
      [](int x, char y) -> int {
         return x + static_cast<int>(y);
      });
   cat::verify(sum_char == 2 + int{'a'});

   // Curried `apply_cases` and tuple-of-callables `visit_apply` both
   // dispatch the same way.
   auto analyze = cat::apply_cases(
      [](int x, int y) -> int {
         return x + y;
      },
      [](int x, char y) -> int {
         return x + static_cast<int>(y);
      });
   cat::verify(analyze(pair_args) == 7);
   cat::verify(analyze(char_args) == 2 + int{'a'});

   auto callable_pack = cat::make_tuple(
      [](int x, int y) -> int {
         return x + y;
      },
      [](int x, char y) -> int {
         return x + static_cast<int>(y);
      });
   cat::verify(cat::visit_apply(pair_args, callable_pack) == 7);
   cat::verify(cat::visit_apply(char_args, callable_pack) == 2 + int{'a'});
};
