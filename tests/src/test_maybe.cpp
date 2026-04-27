#include <cat/maybe>
#include <cat/memory>
#include <cat/tuple>
#include <cat/unique>
#include <cat/utility>

#include "../unit_tests.hpp"

struct movable {
   movable() = default;
   movable(movable&&) = default;
   auto
   operator=(movable&&) {};
};

// NOLINTNEXTLINE We use a global mutable variable for this single-threaded
// test.
int4 maybe_counter = 0;

struct maybe_non_trivial {
   maybe_non_trivial() {
      ++maybe_counter;
   }

   maybe_non_trivial(maybe_non_trivial const&) {
      ++maybe_counter;
   }

   maybe_non_trivial(maybe_non_trivial&&) {
      ++maybe_counter;
   }

   ~maybe_non_trivial() {
      ++maybe_counter;
   }

   maybe_non_trivial(int, int, char) {
   }
};

struct maybe_const_non_trivial {
   constexpr maybe_const_non_trivial() {  // NOLINT
   }

   constexpr maybe_const_non_trivial(
      maybe_const_non_trivial const&) {  // NOLINT
   }

   constexpr maybe_const_non_trivial(maybe_const_non_trivial&&) {
   }
};

auto
maybe_try_success() -> cat::maybe<int> {
   cat::maybe<int> error{0};
   int boo = prop(error);
   return boo;
}

auto
maybe_try_fail() -> cat::maybe<int> {
   cat::maybe<int> error{cat::nullopt};
   int boo = prop(error);
   return boo;
}

// Default-construction, nullopt assignment, value assignment, value() and
// value_or() on a value-typed `maybe`.
test(maybe_basic_value) {
   cat::maybe<int4> foo{cat::nullopt};
   cat::verify(!foo.has_value());

   cat::maybe<int4> inplace_1{};
   cat::verify(!inplace_1.has_value());

   foo = 1;
   cat::verify(foo.has_value());

   foo = cat::nullopt;
   cat::verify(!foo.has_value());

   cat::maybe<int4> moo = 1;
   moo = 2;
   cat::verify(moo.value() == 2);

   moo = cat::nullopt;
   cat::verify(moo.value_or(100) == 100);
}

// Reference rebinding semantics: assignment rebinds the held pointer rather
// than assigning through to the referent. P2988R12 `optional<T&>`. Reference
// rebinding semantics.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r12.pdf
test(maybe_reference_rebinding) {
   cat::maybe<int4&> ref(cat::nullopt);
   cat::maybe<int4&> ref_2 = cat::nullopt;
   cat::verify(!ref.has_value());
   cat::verify(!ref_2.has_value());

   int4 goo = 1;
   cat::maybe<int4&> boo = goo;
   ref = boo;
   boo = cat::nullopt;
   // Because `boo` was rebinded when assigned `cat::nullopt`, `ref` should
   // still hold a value.
   cat::verify(ref.has_value());
   cat::verify(ref.value() == 1);

   goo = 2;
   cat::verify(ref.value() == 2);

   int4 goo_2 = 3;
   // `ref` is rebinded to `goo_2`, instead of `3` assigning through into `goo`.
   ref = goo_2;
   cat::verify(goo == 2);
   goo = 0;
   cat::verify(ref.value() == 3);

   ref = cat::nullopt;
   cat::verify(!ref.has_value());

   ref_2 = goo;
   cat::verify(ref_2.has_value());
   cat::verify(ref_2.value() == goo);
}

// `maybe<compact<T, predicate, sentinel>>` packs the empty state into the
// otherwise-unused `sentinel` value of `T`.
test(maybe_compact_predicate) {
   cat::maybe<cat::compact<int4,
                           [](int4 input) -> bool {
                              return input >= 0;
                           },
                           -1>>
      positive(cat::nullopt);
   cat::verify(!positive.has_value());

   positive = -10;
   cat::verify(!positive.has_value());

   positive = 0;
   cat::verify(positive.has_value());
   auto _ = positive.or_exit();

   positive = 10;
   cat::verify(positive.has_value());

   positive = cat::nullopt;
   cat::verify(!positive.has_value());

   // The default value of `int4` is `0`, which the predicate accepts.
   decltype(positive) default_predicate{cat::in_place, 0};
   cat::verify(default_predicate.value() == 0);

   decltype(positive) default_predicate_empty{};
   cat::verify(!default_predicate_empty.has_value());
}

// Compact predicate over a void wrapper. `monotype_storage` lets the engagement
// bit live in the predicate's storage instead of a side flag.
test(maybe_compact_void) {
   cat::maybe<cat::compact<cat::monotype_storage<int, 0>,
                           [](int input) -> bool {
                              return input >= 0;
                           },
                           -1>>
      predicate_void(cat::nullopt);
   cat::verify(!predicate_void.has_value());
   predicate_void = cat::monostate;
   cat::verify(predicate_void.has_value());
   auto _ = predicate_void.or_exit();
}

// `compact<T, predicate, value>` accepts a literal sentinel that converts to
// `T`. This covers the `-1` to `iword` path used by `maybe_non_negative`.
test(maybe_compact_value_conversion) {
   cat::maybe<cat::compact<iword,
                           [](iword input) -> bool {
                              return input >= 0;
                           },
                           -1>>
      non_negative(cat::nullopt);
   cat::verify(!non_negative.has_value());

   non_negative = 0;
   cat::verify(non_negative.has_value());
   cat::verify(non_negative.value() == 0);

   non_negative = -1;
   cat::verify(!non_negative.has_value());
}

constexpr auto
test_tuple_allocation_sentinel() -> cat::tuple<int4*, cat::idx> {
   return {nullptr, 0u};
}

class non_structural_slot {
   int m_state;

 public:
   constexpr non_structural_slot() : m_state(-1) {
   }

   constexpr non_structural_slot(int state) : m_state(state) {
   }

   friend constexpr auto
   operator==(non_structural_slot const& lhs, non_structural_slot const& rhs)
      -> bool {
      return lhs.m_state == rhs.m_state;
   }
};

constexpr auto
test_non_structural_slot_nullopt() -> non_structural_slot {
   return -1;
}

namespace test_maybe_niche {
struct flagged_value {
   int raw;
};

constexpr auto
has_value(flagged_value value) -> bool {
   return value.raw >= 0;
}

constexpr auto
nullopt_value() -> flagged_value {
   return {-1};
}
}  // namespace test_maybe_niche

template <>
struct cat::default_compact_trait<test_maybe_niche::flagged_value>
    : cat::identity_trait<cat::compact<test_maybe_niche::flagged_value,
                                       &test_maybe_niche::has_value,
                                       &test_maybe_niche::nullopt_value>> {};

// `sentinel` supports structural NTTP values directly.
test(maybe_sentinel_tuple) {
   using row = cat::tuple<int4*, cat::idx>;
   cat::maybe<cat::sentinel<row, row{nullptr, 0u}>> slot;
   cat::verify(!slot.has_value());

   int4 x = 7;
   slot = row{&x, 1u};
   cat::verify(slot.has_value());
   cat::verify(slot.value().first() == &x);
   cat::verify(slot.value().second() == 1u);

   slot = cat::nullopt;
   cat::verify(!slot.has_value());
}

test(maybe_default_compact_trait_extension) {
   using test_maybe_niche::flagged_value;

   static_assert(sizeof(cat::maybe<flagged_value>) == sizeof(flagged_value));

   cat::maybe<flagged_value> value = cat::nullopt;
   cat::verify(!value.has_value());

   value = flagged_value{3};
   cat::verify(value.has_value());
   cat::verify(value.value().raw == 3);

   value = flagged_value{-1};
   cat::verify(!value.has_value());
}

// `sentinel_fn` supports non-structural wrapped types in `maybe`.
test(maybe_sentinel_fn_non_structural) {
   cat::maybe<
      cat::sentinel_fn<non_structural_slot, test_non_structural_slot_nullopt>>
      slot;
   cat::verify(!slot.has_value());

   slot = non_structural_slot(7);
   cat::verify(slot.has_value());
   cat::verify(slot.value() == non_structural_slot(7));

   slot = cat::nullopt;
   cat::verify(!slot.has_value());
}

// `sentinel<T, value>` is the simpler compact form: `value` represents the
// empty state.
test(maybe_sentinel_predicate) {
   cat::maybe<cat::sentinel<int4, 0>> nonzero = cat::nullopt;
   cat::verify(!nonzero.has_value());

   nonzero = 1;
   cat::verify(nonzero.has_value());

   nonzero = 0;
   cat::verify(!nonzero.has_value());
}

test(maybe_span_basic) {
   static_assert(sizeof(cat::maybe_span<int4>) == sizeof(cat::span<int4>));

   int4 values[2] = {1, 2};
   cat::maybe_span<int4> maybe_values = cat::span<int4>(values, 2u);
   cat::verify(maybe_values.has_value());
   cat::verify(maybe_values.value().size() == 2u);
   cat::verify(maybe_values.value()[0] == 1);
   cat::verify(maybe_values.value()[1] == 2);

   // Empty spans are still values if they point to real storage.
   maybe_values = cat::span<int4>(values, 0u);
   cat::verify(maybe_values.has_value());
   cat::verify(maybe_values.value().size() == 0u);
   cat::verify(maybe_values.value().data() == values);

   maybe_values = cat::nullopt;
   cat::verify(!maybe_values.has_value());
}

// `maybe_ptr<T>` is `maybe<sentinel<T*, nullptr>>` (a pointer that can also be
// `nullopt` without growing.
test(maybe_ptr_basic) {
   int4 get_addr = 0;
   cat::maybe_ptr<int4> opt_ptr = &get_addr;
   cat::verify(opt_ptr.has_value());
   cat::verify(opt_ptr.value() == &get_addr);
   cat::verify(*opt_ptr.value() == 0);
   cat::verify(opt_ptr.p_value() == &get_addr);

   opt_ptr = cat::nullopt;
   cat::verify(!opt_ptr.has_value());
   opt_ptr = nullptr;
   cat::verify(!opt_ptr.has_value());
}

// Implicit conversions on assignment. `int4` accepts `int` and `short`.
test(maybe_converting_assign) {
   cat::maybe<int4> foo;
   foo = int{1};
   cat::verify(foo.value() == 1);
   foo = short{2};
   cat::verify(foo.value() == 2);
}

// `transform`, `and_then`, and `or_else` chained against value `maybe`s,
// covering both engaged and disengaged sources.
test(maybe_monadic_chains) {
   cat::maybe<int4> moo = 2;

   // Type-converting transform.
   cat::verify(moo.transform([](int4 input) -> uint8 {
                     return static_cast<uint8>(input * 2);
                  })
                  .value()
               == 4u);

   // `or_else` on an engaged source must not invoke the callable.
   moo.or_else([] {
      cat::exit(1);
   });

   moo = cat::nullopt;
   cat::verify(!moo.transform([](int4 input) {
                      return input * 2;
                   })
                   .has_value());

   // `and_then` on a disengaged source must not invoke the callable.
   auto _ = moo.and_then([](int4 input) -> cat::maybe<int4> {
      cat::exit(1);
      return input;
   });

   cat::verify(!moo.transform([](int4 input) {
                      return input * 2;
                   })
                   .and_then([](int4 input) -> cat::maybe<int4> {
                      return input;
                   })
                   .has_value());
}

// Same chain shapes against a compact-predicate `maybe` to exercise the
// non-trivial storage path.
test(maybe_monadic_compact) {
   cat::maybe<cat::compact<int4,
                           [](int4 input) -> bool {
                              return input >= 0;
                           },
                           -1>>
      positive(cat::nullopt);

   cat::verify(!positive
                   .transform([](int4 input) -> int4 {
                      return input * 2;
                   })
                   .has_value());

   cat::verify(!positive
                   .transform([](int4 input) {
                      return input * 2;
                   })
                   .and_then([](int4 input) -> cat::maybe<int4> {
                      return input;
                   })
                   .has_value());
}

// Chains across function-typed callables, including `maybe<void>` results
// produced by `transform(return_void)`.
test(maybe_monadic_callables) {
   auto return_int = [](int4 input) -> int4 {
      return input + 1;
   };
   auto return_none = [](int4) -> cat::maybe<int4> {
      return cat::nullopt;
   };
   auto return_opt = [](int4 input) -> cat::maybe<int4> {
      return input;
   };
   auto return_void = [](int4) -> void {
   };
   auto return_opt_void = [](int4) -> cat::maybe<void> {
      return cat::monostate;
   };
   auto nothing = []() -> void {
   };
   auto maybe_nothing = []() -> cat::maybe<void> {
      return cat::nullopt;
   };

   cat::maybe<int4> foo = 1;
   foo.transform(return_int).and_then(return_opt_void).or_else(nothing);

   auto _ = foo.transform(return_int)
               .and_then(return_opt_void)
               .or_else(maybe_nothing);

   cat::maybe<int4> monadic_int;
   monadic_int = return_none(0).and_then(return_opt);
   cat::verify(!monadic_int.has_value());

   monadic_int = return_opt(1).transform(return_int);
   cat::verify(monadic_int.has_value());
   cat::verify(monadic_int.value() == 2);

   cat::maybe<void> monadic_void =
      return_opt(1).transform(return_int).transform(return_void);
   cat::verify(monadic_void.has_value());
}

// `and_then` from a `maybe<T&>` source unwraps the reference for the callable
// without writing through to the referent.
test(maybe_monadic_reference_source) {
   auto return_opt_void = [](int4) -> cat::maybe<void> {
      return cat::monostate;
   };

   int4 monadic_int_ref = 1;
   cat::maybe<void> monadic_void_ref =
      cat::maybe(monadic_int_ref).and_then(return_opt_void);
   cat::verify(monadic_void_ref.has_value());
}

// Monadic chain that flows a move-only value through `and_then` and
// `transform`.
test(maybe_monadic_move_only) {
   auto return_none = [](int4) -> cat::maybe<int4> {
      return cat::nullopt;
   };
   auto return_opt = [](int4 input) -> cat::maybe<int4> {
      return input;
   };
   auto return_int = [](int4 input) -> int4 {
      return input + 1;
   };

   cat::maybe<cat::unique<int4>> monadic_move = 1;
   monadic_move = return_none(0).and_then(return_opt);
   cat::verify(!monadic_move.has_value());

   monadic_move = return_opt(1).transform(return_int);
   cat::verify(monadic_move.has_value());
   cat::verify(monadic_move.value().borrow() == 2);
}

// Copy-construction and copy-init between `maybe<T>` instances of the same
// shape.
test(maybe_copy) {
   cat::maybe<int4> opt_original = 10;
   cat::maybe<int4> opt_copy_1 = cat::maybe(opt_original);
   cat::maybe<int4> opt_copy_2 = opt_original;
   cat::verify(opt_copy_1.value() == 10);
   cat::verify(opt_copy_2.value() == 10);
}

// `p_value()` returns the address of the held value (and equals
// `__builtin_addressof(value())`).
test(maybe_pointer_access) {
   cat::maybe<int4> foo = 1;
   int4 const& ref_foo = foo.value();
   cat::verify(&ref_foo == &foo.value());
   cat::verify(foo.p_value() == &foo.value());
   cat::verify(foo.p_value() == __builtin_addressof(foo.value()));
}

// All const/mut combinations of `maybe<T&>` over a non-trivially-destructible
// `T` compile and bind correctly.
test(maybe_nontrivial_references) {
   maybe_non_trivial nontrivial_val;
   cat::maybe<maybe_non_trivial&> nontrivial_ref_default;
   nontrivial_ref_default = nontrivial_val;
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial&> nontrivial_ref = nontrivial_val;

   maybe_non_trivial const const_nontrivial_val;
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial&> const mut_const_nontrivial_ref_default;
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial&> const mut_const_nontrivial_ref =
      nontrivial_val;

   [[maybe_unused]]
   cat::maybe<maybe_non_trivial const&> const_mut_nontrivial_ref_default;
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial const&> const_mut_nontrivial_ref =
      nontrivial_val;
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial const&> const_mut_nontrivial_ref_2 =
      const_nontrivial_val;
   const_mut_nontrivial_ref = const_nontrivial_val;

   [[maybe_unused]]
   cat::maybe<maybe_non_trivial const&> const
      const_const_nontrivial_ref_default;
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial const&> const const_const_nontrivial_ref =
      nontrivial_val;
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial const&> const const_const_nontrivial_ref_2 =
      const_nontrivial_val;
}

// Top-level `const` qualifier on the `maybe` itself.
test(maybe_const_qualified) {
   cat::maybe<int4> const constant_val = 1;
   [[maybe_unused]]
   cat::maybe<int4> const constant_null = cat::nullopt;
   [[maybe_unused]]
   auto con = constant_val.value();
   cat::verify(con == 1);
}

// `maybe<int4 const&>` binds both const and non-const lvalues.
test(maybe_const_references) {
   int4 nonconstant_int = 10;
   int4 const constant_int = 9;
   cat::maybe<int4 const&> constant_ref = constant_int;
   cat::verify(constant_ref.value() == 9);
   constant_ref = nonconstant_int;
   cat::verify(constant_ref.value() == 10);
}

// Move-only value type round-trips into a `maybe`.
test(maybe_move_only) {
   movable test_move;
   [[maybe_unused]]
   cat::maybe<movable> maybe_movs(mov test_move);
}

// Non-trivial destructor must run when a wrapped value goes out of scope.
test(maybe_nontrivial_value) {
   [[maybe_unused]]
   cat::maybe<maybe_non_trivial> nontrivial = maybe_non_trivial();
}

// `maybe<void>` construction modes: default, `monostate`, `in_place`, and
// `nullopt`, plus `in_place` for non-trivial value types.
test(maybe_void_construction) {
   cat::maybe<void> optvoid;
   cat::verify(!optvoid.has_value());
   cat::maybe<void> optvoid_2{cat::monostate};
   cat::verify(optvoid_2.has_value());
   cat::maybe<void> optvoid_4{cat::in_place};
   cat::verify(optvoid_4.has_value());
   cat::maybe<void> optvoid_5{cat::nullopt};
   cat::verify(!optvoid_5.has_value());

   cat::maybe<maybe_non_trivial> in_place_nontrivial_1{cat::in_place};
   auto _ = in_place_nontrivial_1.verify();

   cat::maybe<maybe_non_trivial> in_place_nontrivial_2{cat::in_place, 1, 2,
                                                       'a'};
   auto _ = in_place_nontrivial_2.verify();
}

// Assigning `monostate` engages a `maybe<void>`, `nullopt` disengages it.
test(maybe_void_assignment) {
   cat::maybe<void> optvoid;
   optvoid = cat::monostate;
   cat::verify(optvoid.has_value());
   optvoid = cat::nullopt;
   cat::verify(!optvoid.has_value());
}

// `maybe` is usable in a `constexpr` context across the value, compact, and
// `maybe_ptr` storage paths.
test(maybe_constexpr) {
   [] consteval {
      [[maybe_unused]]
      constexpr cat::maybe<int> const_int_default;

      // TODO: Enable these `verify()` calls when they support `constexpr`.

      constexpr cat::maybe<maybe_const_non_trivial> const_nontrivial_default;
      // cat::verify(!const_nontrivial_default.has_value());

      [[maybe_unused]]
      constexpr cat::maybe<maybe_const_non_trivial> const_nontrivial =
         maybe_const_non_trivial();
      // cat::verify(const_nontrivial.has_value());

      [[maybe_unused]]
      constexpr cat::maybe<maybe_const_non_trivial> const_nontrivial_in_place =
         {cat::in_place, maybe_const_non_trivial()};
      // cat::verify(const_nontrivial_in_place.has_value());

      constexpr cat::maybe_ptr<void> const_optptr = nullptr;
      cat::maybe_ptr<void> optptr = nullptr;
      optptr = nullptr;
      optptr = const_optptr;
      [[maybe_unused]]
      cat::maybe_ptr<void> optptr2 = optptr;
      [[maybe_unused]]
      cat::maybe_ptr<void> optptr3 = const_optptr;
      cat::maybe_ptr<void> optptr4;

      [[maybe_unused]]
      constexpr cat::maybe_ptr<maybe_non_trivial> const_nontrivial_optptr =
         nullptr;
      [[maybe_unused]]
      constexpr cat::maybe_ptr<maybe_non_trivial>
         const_nontrivial_default_optptr;
      [[maybe_unused]]
      cat::maybe_ptr<maybe_non_trivial> nontrivial_optptr = nullptr;
      [[maybe_unused]]
      cat::maybe_ptr<maybe_non_trivial> nontrivial_default_optptr;
   }();
}

// `.is<T>()` answers type membership questions used by `match()`.
test(maybe_is_trait) {
   cat::maybe<int4> opt_is = 1;
   cat::verify(opt_is.is<int4>());
   cat::verify(!opt_is.is<uint8>());

   opt_is = cat::nullopt;
   cat::verify(!opt_is.is<int4>());
   cat::verify(!opt_is.is<uint8>());
}

// Pattern-match against value, type, and `nullopt`, using both the free
// function and the member-access syntax.
test(maybe_match) {
   cat::maybe<int4> opt_match = 1;

   bool matched = false;
   cat::match(opt_match)(is_a(1).then_do([&]() {
      matched = true;
   }));
   cat::match(opt_match)(is_a(2).then_do([&]() {
      matched = false;
   }));
   cat::verify(matched);

   matched = false;
   cat::match(opt_match)(is_a<int4>().then_do([&]() {
      matched = true;
   }));
   cat::match(opt_match)(is_a<uint8>().then_do([&]() {
      matched = false;
   }));
   cat::verify(matched);

   // Engaged maybe should not match `nullopt`.
   matched = true;
   cat::match(opt_match)(is_a(cat::nullopt).then_do([&]() {
      matched = false;
   }));
   cat::verify(matched);

   // Disengaged maybe should match `nullopt`.
   matched = false;
   opt_match = cat::nullopt;
   cat::match(opt_match)(is_a(cat::nullopt).then_do([&]() {
      matched = true;
   }));
   cat::verify(matched);

   matched = false;
   opt_match.match(is_a(cat::nullopt).then_do([&]() {
      matched = true;
   }));
   cat::verify(matched);
}

// `cat::is_maybe` and `cat::is_scaredy` correctly classify `maybe<T>`.
test(maybe_type_traits) {
   static_assert(cat::is_maybe<cat::maybe<int>>);
   static_assert(cat::is_maybe<cat::maybe<int4>>);
   static_assert(!cat::is_scaredy<cat::maybe<int>>);
   static_assert(!cat::is_scaredy<cat::maybe<int4>>);
}

// The `prop` macro propagates an empty `maybe` like a `?` operator, and passes
// through values otherwise.
test(maybe_prop_macro) {
   auto _ = maybe_try_success().verify();
   cat::maybe fail = maybe_try_fail();
   cat::verify(!fail.has_value());
}

namespace {

// `tracked` records every destructor call through a counter. The reference
// tests use this to assert that `cat::maybe<T&>` never destroys its referent.
struct tracked {
   int4* p_dtor_counter;

   constexpr tracked(int4& counter) : p_dtor_counter(&counter) {
   }

   constexpr ~tracked() {
      ++*p_dtor_counter;
   }
};

auto
returns_void_maybe() -> cat::maybe<void> {
   return cat::monostate;
}

auto
returns_empty_void_maybe() -> cat::maybe<void> {
   return cat::nullopt;
}

}  // namespace

// `cat::maybe<T&>` is trivially copyable and pointer-sized. P3836R2 Trivially
// copyable `optional<T&>`.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3836r2.html
test(maybe_reference_layout) {
   static_assert(__is_trivially_copyable(cat::maybe<int4&>));
   static_assert(__is_trivially_copyable(cat::maybe<int4 const&>));
   static_assert(__is_trivially_copyable(cat::maybe<tracked&>));
   static_assert(sizeof(cat::maybe<int4&>) == sizeof(int4*));
   static_assert(sizeof(cat::maybe<tracked&>) == sizeof(tracked*));
}

// Clearing a `maybe<T&>` must not destroy the referent. P2988R12
// `optional<T&>`. Reference rebinding semantics.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r12.pdf
test(maybe_reference_no_destroy) {
   int4 dtor_counter = 0;
   {
      tracked t{dtor_counter};
      cat::maybe<tracked&> ref = t;
      cat::verify(ref.has_value());

      // Disengaging via nullopt assignment must not run `~tracked`.
      ref = cat::nullopt;
      cat::verify(!ref.has_value());
      cat::verify(dtor_counter == 0);

      // Rebind to the same object, still no destruction.
      ref = t;
      cat::verify(ref.has_value());
      cat::verify(dtor_counter == 0);

      // Reset() is also non-destructive.
      ref.reset();
      cat::verify(!ref.has_value());
      cat::verify(dtor_counter == 0);

      // emplace() rebinds without destroying anything.
      ref.emplace(t);
      cat::verify(ref.has_value());
      cat::verify(dtor_counter == 0);
   }
   // Only `t`'s own destructor at end of scope.
   cat::verify(dtor_counter == 1);
}

// Rejection of bindings that would create a temporary referent.
// `is_constructible` properly treats `= delete` overloads as not constructible
// unlike `requires` (a viable-but-deleted overload still satisfies it).
// P2988R12 `optional<T&>`. Dangling-temporary protection.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r12.pdf
test(maybe_reference_dangling) {
   // Lvalue binding succeeds.
   static_assert(cat::is_constructible<cat::maybe<int const&>, int&>);
   static_assert(cat::is_constructible<cat::maybe<int&>, int&>);
   // `int const&` binding to a prvalue would create a temporary (rejected).
   static_assert(!cat::is_constructible<cat::maybe<int const&>, int>);
   // `int&` cannot bind a prvalue at all (also rejected).
   static_assert(!cat::is_constructible<cat::maybe<int&>, int>);
}

// `swap` exchanges held pointers, not the referents. P2988R12 `optional<T&>`.
// Swap rebinds pointers.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r12.pdf
test(maybe_reference_swap) {
   int4 a = 10;
   int4 b = 20;
   cat::maybe<int4&> opt_a = a;
   cat::maybe<int4&> opt_b = b;

   opt_a.swap(opt_b);
   cat::verify(&opt_a.value() == &b);
   cat::verify(&opt_b.value() == &a);
   // The integers themselves are untouched.
   cat::verify(a == 10);
   cat::verify(b == 20);

   // Swap with a disengaged side just transfers the pointer.
   cat::maybe<int4&> empty;
   opt_a.swap(empty);
   cat::verify(!opt_a.has_value());
   cat::verify(empty.has_value());
   cat::verify(&empty.value() == &b);
}

// `value_or` for `maybe<T&>` returns by value (rather than by reference) to
// avoid dangling on a temporary fallback. P2988R12 `optional<T&>`. `value_or`
// returns by value.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r12.pdf
test(maybe_reference_value_or) {
   int4 held = 7;
   cat::maybe<int4&> engaged = held;
   int4 fallback = 99;

   // Engaged: returns the referent by value, decoupled from the reference.
   int4 got = engaged.value_or(fallback);
   cat::verify(got == 7);
   got = 0;
   cat::verify(held == 7);

   cat::maybe<int4&> empty;
   // Disengaged: returns the fallback by value.
   int4 fallback_value = empty.value_or(int4{42});
   cat::verify(fallback_value == 42);

   // The non-reference overload still returns `value_type`.
   cat::maybe<int4> v = 5;
   cat::verify(v.value_or(0) == 5);
   v = cat::nullopt;
   cat::verify(v.value_or(0) == 0);
}

// Range-based for loops over `maybe`. P3168R1 Give `std::optional` range
// support.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3168r1.html
test(maybe_range) {
   cat::maybe<int4> engaged = 11;
   int4 sum = 0;
   for (int4& x : engaged) {
      sum = sum + x;
   }
   cat::verify(sum == 11);

   cat::maybe<int4> empty;
   sum = 0;
   for (int4& x : empty) {
      sum = sum + x;
   }
   cat::verify(sum == 0);

   // The iterator pair has size 1 when engaged and 0 when not.
   cat::verify(engaged.end() - engaged.begin() == 1);
   cat::verify(empty.end() - empty.begin() == 0);

   // Reference-typed `maybe` also iterates.
   int4 referent = 33;
   cat::maybe<int4&> ref = referent;
   sum = 0;
   for (int4& x : ref) {
      sum = sum + x;
   }
   cat::verify(sum == 33);

   // Mutating through the iterator writes through to the referent.
   for (int4& x : ref) {
      x = 100;
   }
   cat::verify(referent == 100);
}

// `cat::make_maybe` factory covers both deducing and explicit-type forms.
test(maybe_make_maybe) {
   auto deduced = cat::make_maybe(int4{42});
   static_assert(cat::is_same<decltype(deduced), cat::maybe<int4>>);
   cat::verify(deduced.value() == 42);

   auto explicit_t = cat::make_maybe<int4>(int4{7});
   static_assert(cat::is_same<decltype(explicit_t), cat::maybe<int4>>);
   cat::verify(explicit_t.value() == 7);

   auto void_maybe = cat::make_maybe<void>();
   static_assert(cat::is_same<decltype(void_maybe), cat::maybe<void>>);
   cat::verify(void_maybe.has_value());
}

// Monadic methods on `maybe<void>`. The callable is invoked with no arguments,
// matching `tl::optional`. P0798R8 Monadic operations for `std::optional`.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r8.html
test(maybe_void_monadic) {
   cat::maybe<void> engaged = cat::monostate;
   cat::maybe<void> empty;

   // `transform` on engaged void invokes `f()` and produces `maybe<U>`.
   bool called = false;
   cat::maybe<int4> mapped = engaged.transform([&]() -> int4 {
      called = true;
      return 7;
   });
   cat::verify(called);
   cat::verify(mapped.value() == 7);

   // `transform` on empty void does NOT call `f()`.
   called = false;
   mapped = empty.transform([&]() -> int4 {
      called = true;
      return 7;
   });
   cat::verify(!called);
   cat::verify(!mapped.has_value());

   // `transform` returning void promotes to `maybe<void>`.
   called = false;
   cat::maybe<void> mapped_void = engaged.transform([&]() -> void {
      called = true;
   });
   cat::verify(called);
   cat::verify(mapped_void.has_value());

   // `and_then` on engaged void invokes `f()` and forwards the result.
   called = false;
   cat::maybe<int4> chained = engaged.and_then([&]() -> cat::maybe<int4> {
      called = true;
      return int4{99};
   });
   cat::verify(called);
   cat::verify(chained.value() == 99);

   // `and_then` on empty void does NOT call `f()` and returns `nullopt`.
   called = false;
   chained = empty.and_then([&]() -> cat::maybe<int4> {
      called = true;
      return int4{99};
   });
   cat::verify(!called);
   cat::verify(!chained.has_value());

   // `or_else` returning `maybe<void>` propagates `*this` when engaged.
   cat::maybe<void> propagated = engaged.or_else(returns_empty_void_maybe);
   cat::verify(propagated.has_value());
   propagated = empty.or_else(returns_void_maybe);
   cat::verify(propagated.has_value());
}

// `or_else` propagates `*this` when engaged, fixing a long-standing bug where
// the return was always `nullopt`.
test(maybe_or_else_propagates) {
   cat::maybe<int4> engaged = 5;
   cat::maybe<int4> propagated = engaged.or_else([]() -> cat::maybe<int4> {
      return int4{99};
   });
   cat::verify(propagated.value() == 5);

   cat::maybe<int4> empty;
   propagated = empty.or_else([]() -> cat::maybe<int4> {
      return int4{99};
   });
   cat::verify(propagated.value() == 99);
}

// Monadic methods across the four value categories.
test(maybe_monadic_value_categories) {
   auto add_one = [](int4 x) -> int4 {
      return x + 1;
   };

   // lvalue.
   cat::maybe<int4> lv = 1;
   cat::verify(lv.transform(add_one).value() == 2);
   // const lvalue.
   cat::maybe<int4> const clv = 2;
   cat::verify(clv.transform(add_one).value() == 3);
   // rvalue.
   cat::verify(cat::maybe<int4>{3}.transform(add_one).value() == 4);
   // const rvalue.
   auto const_rvalue = []() -> cat::maybe<int4> const {
      return cat::maybe<int4>{4};
   };
   cat::verify(const_rvalue().transform(add_one).value() == 5);
}

// `reset()` on a value `maybe` destroys the held value and disengages.
test(maybe_reset) {
   cat::maybe<int4> v = 5;
   cat::verify(v.has_value());
   v.reset();
   cat::verify(!v.has_value());

   cat::maybe<void> w = cat::monostate;
   cat::verify(w.has_value());
   w.reset();
   cat::verify(!w.has_value());
}

// `swap()` on value `maybe`s covers the four engagement combinations.
test(maybe_value_swap) {
   cat::maybe<int4> a = 1;
   cat::maybe<int4> b = 2;
   a.swap(b);
   cat::verify(a.value() == 2);
   cat::verify(b.value() == 1);

   cat::maybe<int4> empty;
   a.swap(empty);
   cat::verify(!a.has_value());
   cat::verify(empty.value() == 2);

   cat::maybe<int4> empty2;
   empty2.swap(a);
   cat::verify(!empty2.has_value());
   cat::verify(!a.has_value());
}

// `cat::maybe<bool>` round-trips, and is immune to the `optional<bool>` from
// `optional<U>` ambiguity that Barry Revzin documented for `std::optional`.
// libCat omits `operator bool()`, so the value constructor
// `maybe<bool>(maybe<int> const&)` is not viable and overload resolution falls
// through to the converting `maybe<U>` constructor, meaning `0` converts to
// `false`, not silently to `true` as it does in `std::optional`.
// https://brevzin.github.io/c++/2023/01/18/optional-construction/
test(maybe_bool) {
   cat::maybe<bool> default_b;
   cat::verify(!default_b.has_value());

   cat::maybe<bool> true_b = true;
   cat::verify(true_b.has_value());
   cat::verify(true_b.verify() == true);

   cat::maybe<bool> false_b = false;
   cat::verify(false_b.has_value());
   cat::verify(false_b.verify() == false);

   cat::maybe<bool> null_b = cat::nullopt;
   cat::verify(!null_b.has_value());

   true_b = cat::nullopt;
   cat::verify(!true_b.has_value());
   true_b = true;
   cat::verify(true_b.verify() == true);

   cat::maybe<bool> engaged = false;
   cat::verify(engaged.value_or(true) == false);
   cat::maybe<bool> empty;
   cat::verify(empty.value_or(true) == true);

   // `cat::maybe<U>` does not silently convert to `bool`, so the value
   // constructor is not viable when the source is itself a `maybe`. This is
   // what immunizes `cat::maybe<bool>` from the bug.
   static_assert(!cat::is_constructible<bool, cat::maybe<int>>);
   static_assert(!cat::is_implicitly_convertible<cat::maybe<int>, bool>);

   // The converting `maybe<U>` constructor is the only viable candidate, so the
   // inner `int` is converted to `bool`: zero -> false, non-zero -> true.
   // `std::optional` would give `true` regardless because its value constructor
   // would hijack overload resolution via `explicit operator bool()`.
   cat::maybe<int> src_zero = 0;
   cat::maybe<bool> dst_zero(src_zero);
   cat::verify(dst_zero.has_value());
   cat::verify(dst_zero.value() == false);

   cat::maybe<int> src_nonzero = 5;
   cat::maybe<bool> dst_nonzero(src_nonzero);
   cat::verify(dst_nonzero.has_value());
   cat::verify(dst_nonzero.value() == true);

   cat::maybe<int> src_empty;
   cat::maybe<bool> dst_empty(src_empty);
   cat::verify(!dst_empty.has_value());
}

// LWG 3836 / Barry Revzin, "Getting in trouble with mixed construction"
// (https://brevzin.github.io/c++/2023/01/18/optional-construction/). The
// converting `maybe<U>` constructors and assignments are guarded by
// `maybe_converts_from_any_cvref`, so the value constructor wins when the
// element type is itself constructible from the source `maybe`.
test(maybe_nested_construction) {
   cat::maybe<int> engaged_inner = 42;
   cat::maybe<int> empty_inner;

   // From an engaged `maybe<int>` lvalue: the outer must engage and hold the
   // engaged inner, *not* collapse one level via the converting constructor.
   cat::maybe<cat::maybe<int>> outer_lval(engaged_inner);
   cat::verify(outer_lval.has_value());
   cat::verify(outer_lval.value().has_value());
   cat::verify(outer_lval.value().value() == 42);

   // From a disengaged `maybe<int>` lvalue: the outer is still engaged, and the
   // wrapped inner is disengaged. Without the guard, the converting constructor
   // would have left the outer disengaged.
   cat::maybe<cat::maybe<int>> outer_empty(empty_inner);
   cat::verify(outer_empty.has_value());
   cat::verify(!outer_empty.value().has_value());

   // Same story for an rvalue `maybe<int>`.
   cat::maybe<cat::maybe<int>> outer_rval(cat::maybe<int>{7});
   cat::verify(outer_rval.has_value());
   cat::verify(outer_rval.value().has_value());
   cat::verify(outer_rval.value().value() == 7);

   cat::maybe<cat::maybe<int>> outer_rval_empty(cat::maybe<int>{});
   cat::verify(outer_rval_empty.has_value());
   cat::verify(!outer_rval_empty.value().has_value());

   // Converting copy assignment behaves the same way.
   cat::maybe<cat::maybe<int>> assigned;
   assigned = engaged_inner;
   cat::verify(assigned.has_value());
   cat::verify(assigned.value().has_value());
   cat::verify(assigned.value().value() == 42);

   assigned = empty_inner;
   cat::verify(assigned.has_value());
   cat::verify(!assigned.value().has_value());

   // The guard does not block genuine cross-type conversions, which keeps
   // generic code working: `maybe<long>` is happily constructible from
   // `maybe<int>` because `long` is not constructible from `maybe<int>`.
   static_assert(cat::is_constructible<cat::maybe<long>, cat::maybe<int>&>);
   static_assert(cat::is_constructible<cat::maybe<long>, cat::maybe<int>>);
   cat::maybe<int> src = 99;
   cat::maybe<long> widened(src);
   cat::verify(widened.has_value());
   cat::verify(widened.value() == 99l);

   cat::maybe<long> widened_assigned;
   widened_assigned = src;
   cat::verify(widened_assigned.has_value());
   cat::verify(widened_assigned.value() == 99l);
}
