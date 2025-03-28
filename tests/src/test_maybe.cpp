#include <cat/maybe>
#include <cat/memory>
#include <cat/unique>
#include <cat/utility>

#include "../unit_tests.hpp"

struct movable {
   movable() = default;
   movable(movable&&) = default;
   auto
   operator=(movable&&) {};
};

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

test(maybe) {
   // Initialize empty.
   cat::maybe<int4> foo{cat::nullopt};
   cat::verify(!foo.has_value());

   cat::maybe<int4> inplace_1{};
   cat::verify(!inplace_1.has_value());

   // `int4` default-initializes to 0.
   // cat::maybe<int4> inplace_2{in_place};
   // cat::verify(inplace_2.value() == 0);

   // Assign a value.
   foo = 1;
   cat::verify(foo.has_value());

   // Remove a value.
   foo = cat::nullopt;
   cat::verify(!foo.has_value());

   // Unwrap a value.
   cat::maybe<int4> moo = 1;
   moo = 2;
   cat::verify(moo.value() == 2);

   moo = cat::nullopt;
   cat::verify(moo.value_or(100) == 100);

   // `maybe` reference.
   cat::maybe<int4&> ref(cat::nullopt);
   cat::maybe<int4&> ref_2 = cat::nullopt;

   cat::verify(!ref.has_value());
   cat::verify(!ref_2.has_value());

   // Rebind.
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
   // `ref` is rebinded to `goo_2`, instead of `3` assigning through into
   // `goo`.
   ref = goo_2;
   cat::verify(goo == 2);
   goo = 0;
   cat::verify(ref.value() == 3);

   ref = cat::nullopt;
   cat::verify(!ref.has_value());

   ref_2 = goo;
   cat::verify(ref_2.has_value());
   cat::verify(ref_2.value() == goo);

   // `maybe` with a predicate.
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

   // `maybe<void>` with a predicate.
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

   // Test the sentinel predicate.
   cat::maybe<cat::sentinel<int4, 0>> nonzero = cat::nullopt;
   cat::verify(!nonzero.has_value());

   nonzero = 1;
   cat::verify(nonzero.has_value());

   nonzero = 0;
   cat::verify(!nonzero.has_value());

   // Test `maybe_ptr`.
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

   // Converting assignments. `foo` is `int4`.
   foo = int{1};
   foo = short{2};

   // Monadic methods.
   moo = 2;

   // type converting transform.
   cat::verify(moo.transform([](int4 input) -> uint8 {
                     return static_cast<uint8>(input * 2);
                  })
                  .value()
               == 4u);

   moo.or_else([] {
      cat::exit(1);
   });

   moo = cat::nullopt;
   cat::verify(!moo.transform([](int4 input) {
                      return input * 2;
                   })
                   .has_value());

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

   positive = cat::nullopt;
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

   decltype(positive) default_predicate_1{};
   cat::verify(!default_predicate_1.has_value());

   // Test function calls.
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

   // Test monadic methods on reference types.
   int4 monadic_int_ref = 1;
   cat::maybe<void> monadic_void_ref =
      cat::maybe(monadic_int_ref).and_then(return_opt_void);
   // Be sure that this did not assign through.
   cat::verify(monadic_void_ref.has_value());

   // The default value of `int4` is `0`.
   decltype(positive) default_predicate_2{cat::in_place, 0};
   cat::verify(default_predicate_2.value() == 0);

   // Test monadic methods on move-only types.
   cat::maybe<cat::unique<int4>> monadic_move = 1;
   monadic_move = return_none(0).and_then(return_opt);
   cat::verify(!monadic_move.has_value());

   monadic_move = return_opt(1).transform(return_int);
   cat::verify(monadic_move.has_value());
   cat::verify(monadic_move.value().borrow() == 2);

   // Test copying `maybe`s into other `maybe`s.
   cat::maybe<int4> opt_original = 10;
   cat::maybe<int4> opt_copy_1 = cat::maybe(opt_original);
   cat::maybe<int4> opt_copy_2 = opt_original;
   cat::verify(opt_copy_1.value() == 10);
   cat::verify(opt_copy_2.value() == 10);

   // Getting pointers.
   foo = 1;
   int4 const& ref_foo = foo.value();
   cat::verify(&ref_foo == &foo.value());
   cat::verify(foo.p_value() == &foo.value());
   cat::verify(foo.p_value() == __builtin_addressof(foo.value()));

   // Test non-trivial reference.
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

   // `maybe const`
   cat::maybe<int4> const constant_val = 1;
   [[maybe_unused]]
   cat::maybe<int4> const constant_null = cat::nullopt;
   [[maybe_unused]]
   auto con = constant_val.value();

   // Test constant references.
   int4 nonconstant_int = 10;
   int4 const constant_int = 9;
   cat::maybe<int4 const&> constant_ref = constant_int;
   cat::verify(constant_ref.value() == 9);
   constant_ref = nonconstant_int;
   cat::verify(constant_ref.value() == 10);

   // Test move-only types.
   movable test_move;
   cat::maybe<movable> maybe_movs(mov test_move);

   // Non-trivial constructor and destructor.
   cat::maybe<maybe_non_trivial> nontrivial = maybe_non_trivial();

   // `maybe<void>` default-initializes empty:
   cat::maybe<void> optvoid;
   cat::verify(!optvoid.has_value());
   // `monostate` initializes a value:
   cat::maybe<void> optvoid_2{cat::monostate};
   cat::verify(optvoid_2.has_value());

   // `in_place` initializes a value:
   cat::maybe<void> optvoid_4{cat::in_place};
   cat::verify(optvoid_4.has_value());
   // `cat::nullopt` initializes empty:
   cat::maybe<void> optvoid_5{cat::nullopt};
   cat::verify(!optvoid_5.has_value());

   cat::maybe<maybe_non_trivial> in_place_nontrivial_1{cat::in_place};
   auto _ = in_place_nontrivial_1.verify();

   cat::maybe<maybe_non_trivial> in_place_nontrivial_2{cat::in_place, 1, 2,
                                                       'a'};
   auto _ = in_place_nontrivial_2.verify();

   // Test `maybe` in a `constexpr` context.
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

      // Test `maybe<compact<T>>`.
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

   // Assign value:
   optvoid = cat::monostate;
   cat::verify(optvoid.has_value());
   // Remove value:
   optvoid = cat::nullopt;
   cat::verify(!optvoid.has_value());

   // Test `.is()`;
   cat::maybe<int4> opt_is = 1;
   cat::verify(opt_is.is<int4>());
   cat::verify(!opt_is.is<uint8>());

   opt_is = cat::nullopt;
   cat::verify(!opt_is.is<int4>());
   cat::verify(!opt_is.is<uint8>());

   // Test `match()`.
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

   // Test matching against `cat::nullopt` when this holds a value.
   matched = true;
   cat::match(opt_match)(is_a(cat::nullopt).then_do([&]() {
      matched = false;
   }));
   cat::verify(matched);

   // Test matching against `cat::nullopt` when this does not hold a value.
   matched = false;
   opt_match = cat::nullopt;
   cat::match(opt_match)(is_a(cat::nullopt).then_do([&]() {
      matched = true;
   }));
   cat::verify(matched);

   // Test member access pattern matching syntax.
   matched = false;
   opt_match.match(is_a(cat::nullopt).then_do([&]() {
      matched = true;
   }));
   cat::verify(matched);

   // Test traits.
   static_assert(cat::is_maybe<cat::maybe<int>>);
   static_assert(cat::is_maybe<decltype(opt_original)>);

   static_assert(!cat::is_scaredy<cat::maybe<int>>);
   static_assert(!cat::is_scaredy<decltype(opt_original)>);

   // Test `prop` macro.
   auto _ = maybe_try_success().verify();
   cat::maybe fail = maybe_try_fail();
   cat::verify(!fail.has_value());
}
