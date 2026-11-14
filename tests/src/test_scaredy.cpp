#include <cat/scaredy>

#include "../unit_tests.hpp"

// Minimal result types usable for `cat::scaredy`.
struct error_type_one {
   int4 code;

   [[nodiscard]]
   constexpr auto
   error() const -> int8 {
      return this->code;
   }
};

struct error_type_two {
   int4 code;

   [[nodiscard]]
   constexpr auto
   error() const -> int8 {
      return this->code;
   }
};

struct scaredy_move_only {
   int4 value = 0;

   constexpr scaredy_move_only() = default;

   constexpr scaredy_move_only(int4 input) : value(input) {
   }

   scaredy_move_only(scaredy_move_only const&) = delete;

   constexpr scaredy_move_only(scaredy_move_only&& other) : value(other.value) {
      other.value = -1;
   }

   auto
   operator=(scaredy_move_only const&) -> scaredy_move_only& = delete;

   constexpr auto
   operator=(scaredy_move_only&& other) -> scaredy_move_only& {
      value = other.value;
      other.value = -1;
      return *this;
   }
};

struct scaredy_explicit_value {
   int4 value = 0;

   constexpr scaredy_explicit_value() = default;

   explicit constexpr scaredy_explicit_value(int4 input) : value(input) {
   }
};

struct scaredy_assignment_value {
   int4 value = 0;

   constexpr scaredy_assignment_value() = default;

   constexpr scaredy_assignment_value(int4 input) : value(input) {
   }

   constexpr scaredy_assignment_value(scaredy_assignment_value const&) =
      default;

   constexpr scaredy_assignment_value(scaredy_assignment_value&& other)
       : value(other.value) {
      other.value = -1;
   }

   constexpr auto
   operator=(scaredy_assignment_value const&)
      -> scaredy_assignment_value& = default;

   constexpr auto
   operator=(scaredy_assignment_value&& other) -> scaredy_assignment_value& {
      value = other.value;
      other.value = -1;
      return *this;
   }
};

struct scaredy_destruct_tracer {
   int4* p_live = nullptr;

   constexpr scaredy_destruct_tracer() = default;

   constexpr scaredy_destruct_tracer(int4& live [[clang::lifetimebound]])
       : p_live(&live) {
      ++live;
   }

   constexpr scaredy_destruct_tracer(scaredy_destruct_tracer const& other)
       : p_live(other.p_live) {
      if (p_live != nullptr) {
         ++*p_live;
      }
   }

   constexpr scaredy_destruct_tracer(scaredy_destruct_tracer&& other)
       : p_live(other.p_live) {
      other.p_live = nullptr;
   }

   constexpr auto
   operator=(scaredy_destruct_tracer const& other) -> scaredy_destruct_tracer& {
      if (this == &other) {
         return *this;
      }
      if (p_live != nullptr) {
         --*p_live;
      }
      p_live = other.p_live;
      if (p_live != nullptr) {
         ++*p_live;
      }
      return *this;
   }

   constexpr auto
   operator=(scaredy_destruct_tracer&& other) -> scaredy_destruct_tracer& {
      if (this == &other) {
         return *this;
      }
      if (p_live != nullptr) {
         --*p_live;
      }
      p_live = other.p_live;
      other.p_live = nullptr;
      return *this;
   }

   constexpr ~scaredy_destruct_tracer() {
      if (p_live != nullptr) {
         --*p_live;
      }
   }
};

auto
one() -> error_type_one {
   error_type_one one{1};
   return one;
}

auto
two() -> error_type_two {
   error_type_two two{2};
   return two;
}

auto
union_errors(int4 error) -> cat::scaredy<int8, error_type_one, error_type_two> {
   switch (error.raw) {
      case 0:
         return one();
      case 1:
         return two();
      case 2:
         return int8{3};
      case 3:
         return 3;
      default:
         __builtin_unreachable();
   }
}

enum class error_set : unsigned char {
   one,
   two
};

auto
scaredy_try_success() -> cat::scaredy<int, error_set> {
   cat::scaredy<int, error_set> error{0};
   int boo = $prop(error);
   return boo;
}

auto
scaredy_try_fail() -> cat::scaredy<int, error_set> {
   cat::scaredy<int, error_set> error{error_set::one};
   int boo = $prop(error);
   return boo;
}

auto
scaredy_try_success_2() -> cat::scaredy<uint4, error_set> {
   cat::scaredy<int, error_set> error{1};
   return $prop_or(error, 2_u4);
}

auto
scaredy_try_fail_2() -> cat::scaredy<cat::monostate_type, error_set> {
   cat::scaredy<int, error_set> error{error_set::one};
   return $prop_or(error, cat::monostate);
}

auto
scaredy_try_success_as_or() -> cat::maybe<cat::uint4> {
   cat::scaredy<int, error_set> error{1};
   return $prop_as_or(error, cat::nullopt, 2_u4);
}

auto
scaredy_try_fail_as_or() -> cat::maybe<void> {
   cat::scaredy<int, error_set> error{error_set::one};
   return $prop_as_or(error, cat::nullopt, cat::monostate);
}

$test(scaredy_converting_move_paths) {
   cat::scaredy<scaredy_move_only, error_type_one> move_only{
      scaredy_move_only{3}
   };
   cat::verify(move_only.value().value == 3);

   cat::scaredy<int8, error_type_one> converted{
      cat::scaredy<int4, error_type_one>{4}
   };
   cat::verify(converted.value() == 4);

   cat::scaredy<int8, error_type_one> converted_error{
      cat::scaredy<int4, error_type_one>{error_type_one{4}}
   };
   cat::verify(converted_error.is_empty());
   cat::verify(converted_error.error().code == 4);

   cat::scaredy<int8, error_type_one> assigned = 0;
   assigned = cat::scaredy<int4, error_type_one>{5};
   cat::verify(assigned.value() == 5);
   assigned = cat::scaredy<int4, error_type_one>{error_type_one{5}};
   cat::verify(assigned.is_empty());
   cat::verify(assigned.error().code == 5);
}

$test(scaredy_explicit_value_conversion) {
   using source_type = cat::scaredy<int4, error_type_one>;
   using target_type = cat::scaredy<scaredy_explicit_value, error_type_one>;

   static_assert(cat::is_constructible<target_type, source_type&>);
   static_assert(cat::is_constructible<target_type, source_type const&>);
   static_assert(cat::is_constructible<target_type, source_type&&>);
   static_assert(!cat::is_convertible<source_type&, target_type>);
   static_assert(!cat::is_convertible<source_type const&, target_type>);
   static_assert(!cat::is_convertible<source_type&&, target_type>);
   static_assert(cat::is_assignable<target_type&, source_type const&>);
   static_assert(cat::is_assignable<target_type&, source_type&&>);

   source_type source = 7;
   target_type converted(source);
   cat::verify(converted.value().value == 7);

   target_type assigned;
   assigned = source;
   cat::verify(assigned.value().value == 7);
}

$test(scaredy_nested_construction_and_assignment) {
   using inner_type = cat::scaredy<int4, error_type_one>;
   using outer_type = cat::scaredy<inner_type, error_type_one>;

   inner_type inner = 6;
   outer_type outer(inner);
   cat::verify(outer.has_value());
   cat::verify(outer.value().has_value());
   cat::verify(outer.value().value() == 6);

   inner_type inner_error = error_type_one{-1};
   outer_type outer_error(inner_error);
   cat::verify(outer_error.has_value());
   cat::verify(outer_error.value().is_empty());

   outer_type outer_rvalue(inner_type{7});
   cat::verify(outer_rvalue.has_value());
   cat::verify(outer_rvalue.value().value() == 7);

   outer_type outer_rvalue_error(inner_type{error_type_one{-1}});
   cat::verify(outer_rvalue_error.has_value());
   cat::verify(outer_rvalue_error.value().is_empty());

   outer_type assigned = error_type_one{-1};
   assigned = inner;
   cat::verify(assigned.has_value());
   cat::verify(assigned.value().has_value());
   cat::verify(assigned.value().value() == 6);

   assigned = inner_error;
   cat::verify(assigned.has_value());
   cat::verify(assigned.value().is_empty());
}

$test(scaredy_default_void_and_copy) {
   static_assert([] {
      cat::scaredy<int4, error_type_one> value;
      cat::scaredy<void, error_type_one> void_value;
      return value.has_value() && value.value() == 0 && void_value.has_value();
   }());

   cat::scaredy<void, error_type_one> void_value;
   cat::verify(void_value.has_value());
   void_value = error_type_one{-1};
   cat::verify(void_value.is_empty());
   void_value = cat::monostate;
   cat::verify(void_value.has_value());

   scaredy_assignment_value source{8};
   cat::scaredy<scaredy_assignment_value, error_type_one> target;
   target = source;
   cat::verify(source.value == 8);
   cat::verify(target.value().value == 8);

   cat::scaredy<int4, error_type_one> original = 9;
   cat::scaredy<int4, error_type_one> copied = original;
   cat::verify(original.value() == 9);
   cat::verify(copied.value() == 9);
}

$test(scaredy_monadic_parity) {
   auto mapped =
      cat::scaredy<int4, error_type_one>{3}.transform([](int4 input) -> uint8 {
         return static_cast<uint8>(input * 2);
      });
   static_assert(
      cat::is_same<decltype(mapped), cat::scaredy<uint8, error_type_one>>
   );
   cat::verify(mapped.value() == 6u);

   bool called = false;
   cat::scaredy<int4, error_type_one> failed = error_type_one{7};
   auto failed_mapped = failed.transform([&](int4 input) -> uint8 {
      called = true;
      return static_cast<uint8>(input);
   });
   cat::verify(!called);
   cat::verify(failed_mapped.is<error_type_one>());
   cat::verify(failed_mapped.error().code == 7);

   auto chained =
      cat::scaredy<int4, error_type_one>{4}.and_then([](int4 input) {
         return cat::scaredy<uint8, error_type_one>{
            static_cast<uint8>(input + 1)
         };
      });
   static_assert(
      cat::is_same<decltype(chained), cat::scaredy<uint8, error_type_one>>
   );
   cat::verify(chained.value() == 5u);

   called = false;
   auto failed_chain = failed.and_then([&](int4 input) {
      called = true;
      return cat::scaredy<uint8, error_type_one>{static_cast<uint8>(input)};
   });
   cat::verify(!called);
   cat::verify(failed_chain.is<error_type_one>());

   auto mapped_void =
      cat::scaredy<int4, error_type_one>{6}.transform([](int4) -> void {
      });
   static_assert(
      cat::is_same<decltype(mapped_void), cat::scaredy<void, error_type_one>>
   );
   cat::verify(mapped_void.has_value());

   auto propagated = cat::scaredy<int4, error_type_one>{5}.or_else([] {
      return cat::scaredy<int4, error_type_one>{99};
   });
   cat::verify(propagated.value() == 5);

   propagated = failed.or_else([] {
      return cat::scaredy<int4, error_type_one>{99};
   });
   cat::verify(propagated.value() == 99);

   cat::scaredy<int4, error_type_one, error_type_two> second_error =
      error_type_two{11};
   auto preserved = second_error.transform([](int4 input) -> uint8 {
      return static_cast<uint8>(input);
   });
   cat::verify(preserved.is<error_type_two>());
   cat::verify(preserved.error<error_type_two>().code == 11);

   using compact_type = cat::scaredy<
      cat::compact_scaredy<
         int4,
         [](int4 input) {
            return input >= 0;
         }>,
      error_type_one>;
   compact_type compact_value;
   cat::verify(compact_value.has_value());
   cat::verify(compact_value.value() == 0);

   compact_type compact_error = error_type_one{-1};
   called = false;
   auto compact_mapped = compact_error.transform([&](int4 input) -> int4 {
      called = true;
      return input * 2;
   });
   cat::verify(!called);
   cat::verify(compact_mapped.is<error_type_one>());
}

$test(scaredy_void_monadic) {
   cat::scaredy<void, error_type_one> success;
   bool called = false;
   auto mapped = success.transform([&]() -> int4 {
      called = true;
      return 12;
   });
   cat::verify(called);
   cat::verify(mapped.value() == 12);

   called = false;
   cat::scaredy<void, error_type_one> failed = error_type_one{13};
   mapped = failed.transform([&]() -> int4 {
      called = true;
      return 12;
   });
   cat::verify(!called);
   cat::verify(mapped.is<error_type_one>());

   auto chained = success.and_then([] {
      return cat::scaredy<int4, error_type_one>{14};
   });
   cat::verify(chained.value() == 14);

   auto mapped_void = success.transform([]() -> void {
   });
   static_assert(
      cat::is_same<decltype(mapped_void), cat::scaredy<void, error_type_one>>
   );
   cat::verify(mapped_void.has_value());
}

$test(scaredy_nontrivial_state_changes) {
   int4 live = 0;
   {
      cat::scaredy<scaredy_destruct_tracer, error_type_one> result{
         scaredy_destruct_tracer{live}
      };
      cat::verify(live == 1);

      result = error_type_one{-1};
      cat::verify(live == 0);

      result = scaredy_destruct_tracer{live};
      cat::verify(live == 1);
   }
   cat::verify(live == 0);
}

$test(scaredy_bool_conversion) {
   cat::scaredy<int4, error_type_one> zero = 0;
   cat::scaredy<bool, error_type_one> converted_zero(zero);
   cat::verify(converted_zero.has_value());
   cat::verify(!converted_zero.value());

   cat::scaredy<int4, error_type_one> nonzero = 5;
   cat::scaredy<bool, error_type_one> converted_nonzero(nonzero);
   cat::verify(converted_nonzero.has_value());
   cat::verify(converted_nonzero.value());

   cat::scaredy<int4, error_type_one> failed = error_type_one{6};
   cat::scaredy<bool, error_type_one> converted_error(failed);
   cat::verify(converted_error.is<error_type_one>());
   cat::verify(converted_error.error().code == 6);
}

$test(scaredy) {
   cat::scaredy result = union_errors(0);
   // The `scaredy` here adds a flag to the `int8`, which is padded out to 16
   // bytes. No storage cost exists for the error types.
   static_assert(sizeof(result) == 16);

   cat::verify(result.is_empty());
   cat::verify(result.is<error_type_one>());
   cat::verify(!result.is<int8>());

   result = union_errors(1);
   cat::verify(result.is_empty());
   cat::verify(result.is<error_type_two>());
   cat::verify(!result.is<int8>());

   result = union_errors(2);
   cat::verify(result.has_value());
   cat::verify(result.is<int8>());

   result = union_errors(3);
   cat::verify(result.has_value());
   cat::verify(result.value() == 3);
   cat::verify(result.is<int8>());

   // Test `.error()`.
   cat::scaredy<int, error_type_one> one_error = error_type_one(1);
   cat::verify(one_error.error().code == 1);
   cat::verify(one_error.error<error_type_one>().code == 1);

   cat::scaredy<int, error_type_one, error_type_two> two_error =
      error_type_one(1);
   cat::verify(two_error.error<error_type_one>().code == 1);

   // Test compact optimization.
   cat::scaredy<
      cat::compact_scaredy<
         int4,
         [](int4 input) {
            return input >= 0;
         }>,
      error_type_one>
      predicate = -1;

   // This `scaredy` adds no storage to an `int4`.
   static_assert(sizeof(predicate) == sizeof(int4));
   cat::verify(predicate.is_empty());

   predicate = -1;
   cat::verify(predicate.is_empty());

   predicate = 0;
   cat::verify(predicate.has_value());

   predicate = 10;
   cat::verify(predicate.has_value());

   predicate = error_type_one(-1);
   cat::verify(predicate.is_empty());

   // Test `.value_or()`.
   cat::scaredy<int4, error_type_one> is_error = error_type_one();
   cat::scaredy<int4, error_type_one> is_value = 2;
   cat::scaredy<int4, error_type_one> const const_is_error = error_type_one();
   cat::scaredy<int4, error_type_one> const const_is_value = 2;

   int4 fallback = is_error.value_or(1);
   cat::verify(fallback == 1);

   int4 no_fallback = is_value.value_or(1);
   cat::verify(no_fallback == 2);

   int4 const_fallback = const_is_error.value_or(1);
   cat::verify(const_fallback == 1);

   int4 no_const_fallback = const_is_value.value_or(1);
   cat::verify(no_const_fallback == 2);

   // Test monadic member functions on a mutable `scaredy`.
   auto increment = [](auto input) {
      return input + 1;
   };
   auto increment_scaredy = [](int4 input) {
      return cat::scaredy<int4, error_type_one>{input + 1};
   };

   cat::scaredy<int4, error_type_one> mut_scaredy = 1;
   auto _ = mut_scaredy.transform(increment).and_then(increment_scaredy);

   // `.transform()` returning `void`.
   mut_scaredy.transform(increment).or_else([]() {
      return;
   });

   auto _ = mut_scaredy.transform(increment).or_else([]() {
      return decltype(mut_scaredy){};
   });

   // Test monadic member functions on a `const`-qualified `scaredy`.
   cat::scaredy<int4, error_type_one> const const_scaredy = 1;
   auto _ = const_scaredy.transform(increment).and_then(increment_scaredy);

   // Test `.is()` on variant `scaredy`.
   bool matched = false;

   cat::scaredy<int4, error_type_one, error_type_two> is_variant_scaredy;
   is_variant_scaredy = 1;

   // Match it against `int4`.
   cat::match(is_variant_scaredy)(  //
      is_a<int4>().then_do([&]() {
         matched = true;
      })
   );
   cat::match(is_variant_scaredy)(  //
      is_a<error_type_one>().then_do([&]() {
         matched = false;
      })
   );
   cat::match(is_variant_scaredy)(  //
      is_a<error_type_two>().then_do([&]() {
         matched = false;
      })
   );
   // `float` can never hold true here, but it should compile.
   cat::match(is_variant_scaredy)(  //
      is_a<float>().then_do([&]() {
         matched = false;
      })
   );
   cat::verify(matched);

   // Match it against `error_type_one`.
   matched = false;
   is_variant_scaredy = error_type_one();
   cat::match(is_variant_scaredy)(  //
      is_a<error_type_one>().then_do([&]() {
         matched = true;
      })
   );
   cat::match(is_variant_scaredy)(  //
      is_a<int4>().then_do([&]() {
         matched = false;
      })
   );
   cat::match(is_variant_scaredy)(  //
      is_a<error_type_two>().then_do([&]() {
         matched = false;
      })
   );
   cat::verify(matched);

   // Test member access pattern matching syntax.
   matched = false;
   is_variant_scaredy.match(is_a<error_type_one>().then_do([&]() {
      matched = true;
   }));
   cat::verify(matched);

   // Test `.is()` on `compact` `scaredy`.
   predicate = 1;

   // Test type comparison.
   matched = false;
   cat::match(predicate)(  //
      is_a<error_type_one>().then_do([&]() {
         cat::exit(1);
      }),
      is_a<int4>().then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);

   matched = false;
   predicate = error_type_one(-1);
   cat::match(predicate)(  //
      is_a<int4>().then_do([&]() {
         cat::exit(1);
      }),
      is_a<error_type_one>().then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);

   // Test traits.
   static_assert(!cat::is_maybe<cat::scaredy<int, error_type_one>>);
   static_assert(!cat::is_maybe<decltype(result)>);

   static_assert(cat::is_scaredy<cat::scaredy<int, error_type_one>>);
   static_assert(cat::is_scaredy<decltype(result)>);

   // Test `$prop` macro.
   auto _ = scaredy_try_success().verify();
   cat::scaredy fail = scaredy_try_fail();
   cat::verify(fail.is_empty());

   // Test `$prop_or` macro for converting `scaredy` into a different type with
   // a different value.
   uint4 non_error = scaredy_try_success_2().verify();
   cat::verify(non_error == 2u);
   cat::scaredy fail2 = scaredy_try_fail_2();
   cat::verify(fail2.is_empty());

   cat::uint4 as_or_ok = scaredy_try_success_as_or().verify();
   cat::verify(as_or_ok == 2u);
   cat::verify(scaredy_try_fail_as_or().is_empty());
}

// `.get_ptr()` on `cat::scaredy`: address-of when engaged, `nullptr` otherwise.
$test(scaredy_get_ptr) {
   cat::scaredy<int4, error_type_one> ok = int4{42};
   cat::verify(ok.has_value());

   int4* p_active = ok.get_ptr();
   cat::verify(p_active != nullptr);
   cat::verify(*p_active == 42);
   cat::verify(p_active == &ok.value());

   cat::scaredy<int4, error_type_one> const const_ok = int4{43};
   int4 const* p_const = const_ok.get_ptr();
   cat::verify(p_const == &const_ok.value());
   cat::verify(*p_const == 43);

   cat::scaredy<int4, error_type_one> bad = error_type_one{-1};
   cat::verify(bad.is_empty());
   cat::verify(bad.get_ptr() == nullptr);

   // For a pointer-typed value the held pointer is forwarded directly.
   int4 referent = 7;
   cat::scaredy<int4*, error_type_one> ok_ptr = &referent;
   cat::verify(ok_ptr.get_ptr() == &referent);
   cat::scaredy<int4*, error_type_one> bad_ptr = error_type_one{1};
   cat::verify(bad_ptr.get_ptr() == nullptr);
}

struct scaredy_incomplete_type;

static_assert(!cat::detail::is_pointer_scaredy<scaredy_incomplete_type>);
static_assert(cat::detail::is_pointer_scaredy<scaredy_incomplete_type*>);
static_assert(
   cat::detail::is_pointer_scaredy<cat::compact_scaredy<
      scaredy_incomplete_type*, [](scaredy_incomplete_type* p) -> bool {
         return p != nullptr;
      }>>
);

struct scaredy_incomplete_holder {
   cat::scaredy<scaredy_incomplete_type*, error_type_one> ptr_result;
};

namespace {

enum class compact_ptr_error : unsigned char {
   null_pointer = 0u,
};

using compact_ptr_scaredy = cat::scaredy<
   cat::compact_scaredy<
      int4*,
      [](int4* p) -> bool {
         return p != nullptr;
      }>,
   compact_ptr_error>;

static_assert(sizeof(compact_ptr_scaredy) == sizeof(int4*));

struct scaredy_ptr_base {
   int4 tag = 5;
};

struct scaredy_ptr_derived : scaredy_ptr_base {
   int4 extra = 6;
};

constinit cat::idx g_scaredy_handler_hits{};

void
counting_scaredy_assert_handler(cat::source_location const& /*unused*/) {
   ++g_scaredy_handler_hits;
}

#if __has_feature(cxx_exceptions)
struct scaredy_assert_thrown {};

void
throwing_scaredy_assert_handler(cat::source_location const& /*unused*/) {
   throw scaredy_assert_thrown{};
}
#endif

auto
make_temp_ptr_scaredy(int4* p_target [[clang::lifetimebound]])
   -> cat::scaredy<int4*, error_type_one> {
   return p_target;
}

auto
make_temp_compact_ptr_scaredy(int4* p_target [[clang::lifetimebound]])
   -> compact_ptr_scaredy {
   return p_target;
}

}  // namespace

// Plain pointer `scaredy<T*, Errors...>`: construction, observers, `value_or`,
// `or_else`, `match`, `constexpr`, and temporary unwrapping without
// `-Wdangling`.
$test(scaredy_plain_pointer_specialization) {
   g_scaredy_handler_hits = 0u;

   // Default construction initializes the pointer to `nullptr` (engaged).
   cat::scaredy<int4*, error_type_one> defaulted;
   cat::verify(defaulted.has_value());
   cat::verify(defaulted.value() == nullptr);
   cat::verify(defaulted.get_ptr() == nullptr);
   cat::verify(defaulted.assert() == nullptr);
   cat::verify(defaulted.verify(&counting_scaredy_assert_handler) == nullptr);

   int4 target = 55;
   int4 fallback = 88;
   int4* p_local = &target;
   cat::scaredy<int4*, error_type_one> engaged = p_local;
   cat::verify(engaged.has_value());
   cat::verify(engaged.value() == &target);
   cat::verify(engaged.get_ptr() == &target);
   cat::verify(engaged.assert() == &target);
   cat::verify(engaged.assert(&counting_scaredy_assert_handler) == &target);
   cat::verify(engaged.verify() == &target);
   cat::verify(engaged.verify(&counting_scaredy_assert_handler) == &target);
   cat::verify(engaged.value_or(&fallback) == &target);

   // Derived-to-base pointer conversion.
   scaredy_ptr_derived derived_obj{};
   cat::scaredy<scaredy_ptr_base*, error_type_one> base_ptr = &derived_obj;
   cat::verify(base_ptr.verify()->tag == 5);

   // Incomplete-pointee holder compiles and defaults to `nullptr`.
   scaredy_incomplete_holder holder{};
   cat::verify(holder.ptr_result.has_value());
   cat::verify(holder.ptr_result.get_ptr() == nullptr);

   // Error state: `get_ptr()` is `nullptr`, `value_or()` returns fallback.
   cat::scaredy<int4*, error_type_one> failed = error_type_one{9};
   cat::verify(failed.is_empty());
   cat::verify(failed.get_ptr() == nullptr);
   cat::verify(failed.value_or(&fallback) == &fallback);

   // `or_else()` preserves engaged pointer and recovers from error.
   auto kept = engaged.or_else([&] {
      return cat::scaredy<int4*, error_type_one>{&fallback};
   });
   cat::verify(kept.value() == &target);

   auto recovered = failed.or_else([&] {
      return cat::scaredy<int4*, error_type_one>{&fallback};
   });
   cat::verify(recovered.value() == &fallback);

   // `.match()` and `cat::match(...)` on pointer `scaredy`.
   bool matched_ptr = false;
   engaged.match(
      is_a<error_type_one>().then_do([&] {
         cat::exit(1);
      }),
      is_a<int4*>().then_do([&] {
         matched_ptr = true;
      })
   );
   cat::verify(matched_ptr);

   bool matched_err = false;
   cat::match(failed)(
      is_a<int4*>().then_do([&] {
         cat::exit(1);
      }),
      is_a<error_type_one>().then_do([&] {
         matched_err = true;
      })
   );
   cat::verify(matched_err);

   // Temporary unwrapping without `-Wdangling`.
   int4* p_ver = make_temp_ptr_scaredy(&target).verify();
   int4* p_ver_h =
      make_temp_ptr_scaredy(&target).verify(&counting_scaredy_assert_handler);
   int4* p_ass = make_temp_ptr_scaredy(&target).assert();
   int4* p_ass_h =
      make_temp_ptr_scaredy(&target).assert(&counting_scaredy_assert_handler);
   int4* p_val = make_temp_ptr_scaredy(&target).value();
   int4* p_get = make_temp_ptr_scaredy(&target).get_ptr();
   cat::verify(p_ver == &target && p_ver_h == &target);
   cat::verify(p_ass == &target && p_ass_h == &target);
   cat::verify(p_val == &target && p_get == &target);
   cat::verify(g_scaredy_handler_hits == 0u);

   // `consteval` support for plain pointer `scaredy`.
   constexpr auto consteval_ok = [] consteval -> bool {
      int4 x = 19;
      cat::scaredy<int4*, error_type_one> s_null;
      if (
         !s_null.has_value() || s_null.value() != nullptr
         || s_null.get_ptr() != nullptr
      ) {
         return false;
      }
      cat::scaredy<int4*, error_type_one> s_ok = &x;
      return s_ok.has_value() && *s_ok.value() == 19 && s_ok.get_ptr() == &x
             && s_ok.value_or(nullptr) == &x;
   }();
   static_assert(consteval_ok);

#if __has_feature(cxx_exceptions)
   bool caught = false;
   try {
      auto _ = failed.verify(&throwing_scaredy_assert_handler);
   } catch (scaredy_assert_thrown const&) {
      caught = true;
   }
   cat::verify(caught);
#endif
}

// Compact pointer `scaredy<compact_scaredy<T*, pred>, Error>`: pointer-sized
// niche layout, observers, `value_or`, `or_else`, `match`, and temporary
// unwrapping.
$test(scaredy_compact_pointer_specialization) {
   g_scaredy_handler_hits = 0u;
   int4 target = 101;
   int4 fallback = 202;

   compact_ptr_scaredy engaged = &target;
   cat::verify(engaged.has_value());
   cat::verify(engaged.value() == &target);
   cat::verify(engaged.get_ptr() == &target);
   cat::verify(engaged.assert() == &target);
   cat::verify(engaged.assert(&counting_scaredy_assert_handler) == &target);
   cat::verify(engaged.verify() == &target);
   cat::verify(engaged.verify(&counting_scaredy_assert_handler) == &target);
   cat::verify(engaged.value_or(&fallback) == &target);

   compact_ptr_scaredy failed = compact_ptr_error::null_pointer;
   cat::verify(failed.is_empty());
   cat::verify(failed.error() == compact_ptr_error::null_pointer);
   cat::verify(failed.get_ptr() == nullptr);
   cat::verify(failed.value_or(&fallback) == &fallback);

   // Assigning `nullptr` directly triggers the compact predicate's empty state.
   engaged = static_cast<int4*>(nullptr);
   cat::verify(engaged.is_empty());
   cat::verify(engaged.get_ptr() == nullptr);

   engaged = &target;
   auto kept = engaged.or_else([&] {
      return compact_ptr_scaredy{&fallback};
   });
   cat::verify(kept.value() == &target);

   auto recovered = failed.or_else([&] {
      return compact_ptr_scaredy{&fallback};
   });
   cat::verify(recovered.value() == &fallback);

   bool matched_val = false;
   engaged.match(
      is_a<compact_ptr_error>().then_do([&] {
         cat::exit(1);
      }),
      is_a<int4*>().then_do([&] {
         matched_val = true;
      })
   );
   cat::verify(matched_val);

   bool matched_err = false;
   failed.match(
      is_a<int4*>().then_do([&] {
         cat::exit(1);
      }),
      is_a<compact_ptr_error>().then_do([&] {
         matched_err = true;
      })
   );
   cat::verify(matched_err);

   // Temporary unwrapping without `-Wdangling`.
   int4* p_ver = make_temp_compact_ptr_scaredy(&target).verify();
   int4* p_ver_h = make_temp_compact_ptr_scaredy(&target).verify(
      &counting_scaredy_assert_handler
   );
   int4* p_ass = make_temp_compact_ptr_scaredy(&target).assert();
   int4* p_ass_h = make_temp_compact_ptr_scaredy(&target).assert(
      &counting_scaredy_assert_handler
   );
   int4* p_val = make_temp_compact_ptr_scaredy(&target).value();
   int4* p_get = make_temp_compact_ptr_scaredy(&target).get_ptr();
   cat::verify(p_ver == &target && p_ver_h == &target);
   cat::verify(p_ass == &target && p_ass_h == &target);
   cat::verify(p_val == &target && p_get == &target);
   cat::verify(g_scaredy_handler_hits == 0u);

#if __has_feature(cxx_exceptions)
   bool caught = false;
   try {
      auto _ = failed.verify(&throwing_scaredy_assert_handler);
   } catch (scaredy_assert_thrown const&) {
      caught = true;
   }
   cat::verify(caught);
#endif
}

// Value and `void` `scaredy` `assert()` / `verify()` overloads, `noexcept`
// specifiers, and `operator=` return types.
$test(scaredy_checked_accessors_and_noexcept) {
   constexpr bool no_exc = !__has_feature(cxx_exceptions);
   g_scaredy_handler_hits = 0u;

   cat::scaredy<int4, error_type_one> ok_val = 17;
   cat::scaredy<int4, error_type_one> bad_val = error_type_one{3};
   cat::scaredy<void, error_type_one> ok_void;
   cat::scaredy<void, error_type_one> bad_void = error_type_one{4};
   int4 pointee = 9;
   cat::scaredy<int4*, error_type_one> ok_ptr = &pointee;
   compact_ptr_scaredy ok_compact_ptr = &pointee;

   cat::verify(&ok_val.assert() == &ok_val.value());
   cat::verify(
      &ok_val.assert(&counting_scaredy_assert_handler) == &ok_val.value()
   );
   cat::verify(&ok_val.verify() == &ok_val.value());
   cat::verify(
      &ok_val.verify(&counting_scaredy_assert_handler) == &ok_val.value()
   );

   ok_void.value();
   ok_void.assert();
   ok_void.assert(&counting_scaredy_assert_handler);
   ok_void.verify();
   ok_void.verify(&counting_scaredy_assert_handler);
   cat::verify(g_scaredy_handler_hits == 0u);

   // Disengaged `scaredy<void, ...>` invokes the custom handler safely.
   bad_void.verify(&counting_scaredy_assert_handler);
   cat::verify(g_scaredy_handler_hits == 1u);
#ifndef NDEBUG
   bad_void.assert(&counting_scaredy_assert_handler);
   cat::verify(g_scaredy_handler_hits == 2u);
#endif

#if __has_feature(cxx_exceptions)
   bool caught_val = false;
   try {
      auto _ = bad_val.verify(&throwing_scaredy_assert_handler);
   } catch (scaredy_assert_thrown const&) {
      caught_val = true;
   }
   cat::verify(caught_val);
#endif

   // Assignment operators return `scaredy<T, Errors...>&`, not `scaredy_base&`.
   static_assert(
      cat::is_same<decltype(ok_val = 2), cat::scaredy<int4, error_type_one>&>
   );
   static_assert(cat::is_same<
                 decltype(ok_val = error_type_one{1}),
                 cat::scaredy<int4, error_type_one>&>);
   static_assert(
      cat::is_same<
         decltype(ok_ptr = &pointee), cat::scaredy<int4*, error_type_one>&>
   );
   static_assert(
      cat::is_same<decltype(ok_compact_ptr = &pointee), compact_ptr_scaredy&>
   );

   // `noexcept` invariants across value, `void`, plain-pointer, and
   // compact-pointer `scaredy`.
   static_assert(noexcept(ok_val.value()) == no_exc);
   static_assert(noexcept(ok_val.assert()) == no_exc);
   static_assert(
      noexcept(ok_val.assert(&counting_scaredy_assert_handler)) == no_exc
   );
   static_assert(noexcept(ok_val.verify()) == no_exc);
   static_assert(
      noexcept(ok_val.verify(&counting_scaredy_assert_handler)) == no_exc
   );
   static_assert(noexcept(bad_val.error()) == no_exc);

   static_assert(noexcept(ok_void.value()) == no_exc);
   static_assert(noexcept(ok_void.assert()) == no_exc);
   static_assert(
      noexcept(ok_void.assert(&counting_scaredy_assert_handler)) == no_exc
   );
   static_assert(noexcept(ok_void.verify()) == no_exc);
   static_assert(
      noexcept(ok_void.verify(&counting_scaredy_assert_handler)) == no_exc
   );

   static_assert(noexcept(ok_ptr.value()) == no_exc);
   static_assert(noexcept(ok_ptr.assert()) == no_exc);
   static_assert(
      noexcept(ok_ptr.assert(&counting_scaredy_assert_handler)) == no_exc
   );
   static_assert(noexcept(ok_ptr.verify()) == no_exc);
   static_assert(
      noexcept(ok_ptr.verify(&counting_scaredy_assert_handler)) == no_exc
   );
   static_assert(noexcept(ok_ptr.get_ptr()));

   static_assert(noexcept(ok_compact_ptr.value()) == no_exc);
   static_assert(noexcept(ok_compact_ptr.assert()) == no_exc);
   static_assert(
      noexcept(ok_compact_ptr.assert(&counting_scaredy_assert_handler))
      == no_exc
   );
   static_assert(noexcept(ok_compact_ptr.verify()) == no_exc);
   static_assert(
      noexcept(ok_compact_ptr.verify(&counting_scaredy_assert_handler))
      == no_exc
   );
   static_assert(noexcept(ok_compact_ptr.get_ptr()));
}
