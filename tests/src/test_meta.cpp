#include <cat/compare>
#include <cat/meta>
#include <cat/tuple>

#include "../unit_tests.hpp"

namespace {
void
test_is_function() {
}

struct members {
   int member_variable;

   void
   member_function() {
   }
};

template <typename T>
class templated_one {};

template <typename T>
class templated_two {};

}

$test(meta_is_structural) {
   using namespace cat;

   struct structural_type {
      int value;
      constexpr auto
      operator==(structural_type const&) const -> bool = default;
   };

   class non_structural_type {
    private:
      [[maybe_unused]]
      int _;
   };

   static_assert(is_structural<int>);
   static_assert(is_structural<int*>);
   static_assert(is_structural<structural_type>);
   static_assert(!is_structural<non_structural_type>);
}

$test(meta_is_same_either_neither) {
   using namespace cat;

   static_assert(is_same<int, int>);
   static_assert(!is_same<int, unsigned int>);

   static_assert(is_either<int, int>);
   static_assert(is_either<int, unsigned int, int>);
   static_assert(is_either<int, char, short, int, long>);
   static_assert(!is_either<int, unsigned int>);
   static_assert(!is_either<int, char, short, long>);
   static_assert(!is_either<int>);

   static_assert(is_neither<int, unsigned int>);
   static_assert(is_neither<int, char, short, long>);
   static_assert(is_neither<int>);
   static_assert(!is_neither<int, int>);
   static_assert(!is_neither<int, unsigned int, int>);
   static_assert(!is_neither<int, char, short, int, long>);
}

$test(meta_enum_class_union) {
   using namespace cat;

   enum enum_type : int {
   };

   enum class enum_class_type : int {
   };

   struct struct_type {};

   class class_type {};

   union union_type {};

   static_assert(!is_enum<int>);
   static_assert(!is_enum<struct_type>);
   static_assert(!is_enum<class_type>);
   static_assert(!is_enum<union_type>);
   static_assert(is_enum<enum_type>);
   static_assert(is_enum<enum_class_type>);

   static_assert(!is_scoped_enum<int>);
   static_assert(!is_scoped_enum<struct_type>);
   static_assert(!is_scoped_enum<class_type>);
   static_assert(!is_scoped_enum<union_type>);
   static_assert(!is_scoped_enum<enum_type>);
   static_assert(is_scoped_enum<enum_class_type>);

   static_assert(!is_class<int>);
   static_assert(!is_class<enum_type>);
   static_assert(!is_class<enum_class_type>);
   static_assert(!is_class<union_type>);
   static_assert(is_class<struct_type>);
   static_assert(is_class<class_type>);

   static_assert(is_union<union_type>);
   static_assert(!is_union<int>);
   static_assert(!is_union<enum_type>);
   static_assert(!is_union<enum_class_type>);
   static_assert(!is_union<struct_type>);
   static_assert(!is_union<class_type>);
}

$test(meta_is_pointer) {
   using namespace cat;

   static_assert(is_pointer<int*>);
   static_assert(is_pointer<int* const>);
   static_assert(!is_pointer<int>);
   static_assert(!is_pointer<int*&>);
}

$test(meta_is_arithmetic) {
   using namespace cat;

   static_assert(is_arithmetic<int>);
   static_assert(is_arithmetic<float>);
   static_assert(!is_arithmetic<void>);
}

$test(meta_reference_traits) {
   using namespace cat;

   static_assert(is_reference<int&>);
   static_assert(is_reference<int const&>);
   static_assert(!is_reference<int>);

   static_assert(is_referenceable<int>);
   static_assert(is_referenceable<int&>);
   static_assert(is_referenceable<int&&>);
   static_assert(!is_referenceable<void>);

   // TODO: Use `is_same`:
   static_assert(!is_reference<remove_reference<int>>);
   static_assert(!is_reference<remove_reference<int&>>);
   static_assert(!is_reference<remove_reference<int&&>>);
   static_assert(!is_reference<remove_reference<int const&>>);
   static_assert(!is_reference<remove_reference<int const&&>>);

   // TODO: Test `decay`.

   static_assert(is_reference<add_lvalue_reference<int>>);
   static_assert(is_reference<add_lvalue_reference<int&>>);
   static_assert(is_reference<add_lvalue_reference<int&&>>);
   static_assert(is_reference<add_lvalue_reference<int const&>>);
   static_assert(is_reference<add_lvalue_reference<int const&&>>);

   static_assert(is_reference<add_rvalue_reference<int>>);
   static_assert(is_reference<add_rvalue_reference<int&>>);
   static_assert(is_reference<add_rvalue_reference<int&&>>);

   static_assert(is_reference<add_rvalue_reference<int>>);
   static_assert(is_reference<add_rvalue_reference<int const&>>);
   static_assert(is_reference<add_rvalue_reference<int const&&>>);

   static_assert(!is_lvalue_reference<int>);
   static_assert(is_lvalue_reference<int&>);
   static_assert(is_lvalue_reference<int const&>);
   static_assert(!is_lvalue_reference<int&&>);
   static_assert(!is_lvalue_reference<int const&&>);

   static_assert(is_lvalue_reference<add_lvalue_reference<int>>);
   static_assert(is_lvalue_reference<add_lvalue_reference<int&>>);
   static_assert(is_lvalue_reference<add_lvalue_reference<int&&>>);

   static_assert(!is_rvalue_reference<int>);
   static_assert(!is_rvalue_reference<int&>);
   static_assert(is_rvalue_reference<int&&>);
   static_assert(is_rvalue_reference<int const&&>);

   static_assert(is_rvalue_reference<add_rvalue_reference<int>>);
   // This is supposed to hold false, even with `std::add_rvalue_reference_t`:
   static_assert(!is_rvalue_reference<add_rvalue_reference<int&>>);
   static_assert(is_rvalue_reference<add_rvalue_reference<int&&>>);
}

$test(meta_value_category) {
   using namespace cat;

   // `is_forwarding_move` matches rvalue-ref types (after `T&&`
   // collapsing) and top-level non-const value types (which deduce as
   // rvalue arguments under perfect-forwarding). Const value types are
   // excluded so a `is_forwarding_move auto&&` parameter cannot shadow
   // a `T const&` overload.
   static_assert(is_forwarding_move<int>);
   static_assert(!is_forwarding_move<int const>);
   static_assert(!is_forwarding_move<int&>);
   static_assert(!is_forwarding_move<int const&>);
   static_assert(is_forwarding_move<int&&>);
   static_assert(is_forwarding_move<int const&&>);
}

// Used by `meta_value_category_forwarding`. A local class cannot declare
// member templates, so the templated forwarding ctor forces this type to
// live at file scope.
namespace {
struct forwarding_sink {
   int kind = 0;
   forwarding_sink() = default;

   forwarding_sink(forwarding_sink const& /*other*/) : kind(1) {
   }

   // This should constrain move constructor so that it doesn't shadow non-const
   // `forwarding_sink&`.
   forwarding_sink(cat::is_forwarding_move auto&& /*other*/) : kind(2) {
   }

   constexpr auto
   access(this cat::is_lvalue_reference auto&& self) -> int {
      return self.kind + 10;
   }

   constexpr auto
   access(this cat::is_forwarding_move auto&& self) -> int {
      return self.kind + 20;
   }
};
}  // namespace

$test(meta_value_category_forwarding) {
   using namespace cat;

   // `is_lvalue_reference` constraining a forwarding-reference parameter
   // binds only to l-value callers, r-value arguments (including const
   // r-values, which can still dangle a returned view) are rejected via
   // SFINAE. This is the canonical "borrowing observer" pattern that
   // `cat::container::subspan` and friends use to refuse to return a view
   // into an r-value. Generic lambdas are the closure exception to the
   // local-class member-template ban, so this fits inside the test.
   auto borrow = []<is_lvalue_reference T>(T&& /*x*/) {
   };

   static_assert(is_invocable<decltype(borrow), int&>);
   static_assert(is_invocable<decltype(borrow), int const&>);
   static_assert(!is_invocable<decltype(borrow), int>);
   static_assert(!is_invocable<decltype(borrow), int const>);
   static_assert(!is_invocable<decltype(borrow), int&&>);
   static_assert(!is_invocable<decltype(borrow), int const&&>);

   int x = 5;
   borrow(x);

   // `is_forwarding_move` keeps the templated forwarding ctor from
   // hijacking the copy ctor: l-value arguments route to the copy ctor
   // (kind 1), r-values route to the forwarding ctor (kind 2). Without the
   // constraint, the template's identity reference binding would beat the
   // copy ctor's cv-adjusted binding for l-values.
   forwarding_sink a;
   forwarding_sink lcopy(a);
   cat::verify(lcopy.kind == 1);

   forwarding_sink const b;
   forwarding_sink lconstcopy(b); // NOLINT
   cat::verify(lconstcopy.kind == 1);

   forwarding_sink rmove(cat::move(a));
   cat::verify(rmove.kind == 2);

   forwarding_sink rconstmove(cat::move(b)); // NOLINT
   cat::verify(rconstmove.kind == 1);

   forwarding_sink other(42);
   cat::verify(other.kind == 2);

   // The same constraints work on member function parameters via deducing
   // `this`. Two `access` overloads partition l-value and non-const
   // r-value callers: l-value calls dispatch to the `is_lvalue_reference`
   // overload (`kind + 10`) and non-const r-value calls dispatch to the
   // `is_forwarding_move` overload (`kind + 20`). Const r-values
   // (`cat::move(b)`) deliberately fall in the gap and are ill-formed,
   // mirroring the dangling-prevention behavior of `subspan`.
   cat::verify(lcopy.access() == 11);
   cat::verify(b.access() == 10);
   cat::verify(cat::move(rmove).access() == 22);
}

$test(meta_const_volatile) {
   using namespace cat;

   static_assert(is_const<int const>);
   static_assert(!is_const<int>);
   static_assert(!is_const<int const&>);
   static_assert(!is_const<int&&>);
   static_assert(!is_const<int const*>);

   static_assert(is_const<add_const<int>>);
   static_assert(is_const<add_const<int const>>);
   static_assert(!is_const<remove_const<int>>);
   static_assert(!is_const<remove_const<int const>>);

   static_assert(is_const_ref<int const>);
   static_assert(!is_const_ref<int>);
   static_assert(is_const_ref<int const&>);
   static_assert(!is_const_ref<int const*>);

   static_assert(is_const_ref<add_const<int>>);
   static_assert(is_const_ref<add_const<int const>>);
   static_assert(!is_const_ref<remove_const<int>>);
   static_assert(!is_const_ref<remove_const<int const>>);

   static_assert(is_same<add_const<int>, int const>);
   static_assert(is_same<add_const<int&>, int const&>);
   static_assert(is_same<add_const<int&&>, int const&&>);
   static_assert(is_same<add_const<int const>, int const>);
   static_assert(is_same<add_const<int const&>, int const&>);
   static_assert(is_same<add_const<int const&&>, int const&&>);

   static_assert(is_same<add_const<int volatile>, int const volatile>);
   static_assert(is_same<add_const<int volatile&>, int const volatile&>);
   static_assert(is_same<add_const<int volatile&&>, int const volatile&&>);
   static_assert(is_same<add_const<int const volatile>, int const volatile>);
   static_assert(is_same<add_const<int const volatile&>, int const volatile&>);
   static_assert(
      is_same<add_const<int const volatile&&>, int const volatile&&>);

   static_assert(is_same<add_volatile<int>, int volatile>);
   static_assert(is_same<add_volatile<int&>, int volatile&>);
   static_assert(is_same<add_volatile<int&&>, int volatile&&>);
   static_assert(is_same<add_volatile<int volatile>, int volatile>);
   static_assert(is_same<add_volatile<int volatile&>, int volatile&>);
   static_assert(is_same<add_volatile<int volatile&&>, int volatile&&>);

   static_assert(is_same<add_volatile<int const>, int const volatile>);
   static_assert(is_same<add_volatile<int const&>, int const volatile&>);
   static_assert(is_same<add_volatile<int const&&>, int const volatile&&>);
   static_assert(is_same<add_volatile<int const volatile>, int const volatile>);
   static_assert(
      is_same<add_volatile<int const volatile&>, int const volatile&>);
   static_assert(
      is_same<add_volatile<int const volatile&&>, int const volatile&&>);
}

$test(meta_integral_signedness) {
   using namespace cat;

   class class_type {};

   struct signed_type {
      int data;
      constexpr signed_type(int value) : data(value) {};

      constexpr auto
      operator<(signed_type other) const -> bool {
         return data < other.data;
      }
   };

   static_assert(is_signed<int>);
   static_assert(is_signed<signed_type>);
   static_assert(!is_signed<unsigned int>);
   static_assert(!is_signed<class_type>);

   static_assert(is_unsigned<unsigned int>);
   static_assert(is_unsigned<class_type>);
   static_assert(!is_unsigned<int>);
   static_assert(!is_unsigned<signed_type>);

   static_assert(is_integral<int>);
   static_assert(is_integral<int4>);
   static_assert(is_integral<uint8>);
   static_assert(is_integral<int const>);
   static_assert(is_integral<int4 const>);
   static_assert(is_integral<uint8 const>);

   static_assert(is_signed_integral<int>);
   static_assert(is_signed_integral<int4>);
   static_assert(!is_signed_integral<uint8>);
   static_assert(is_signed_integral<int const>);
   static_assert(is_signed_integral<int4 const>);
   static_assert(!is_signed_integral<uint8 const>);

   static_assert(!is_unsigned_integral<int>);
   static_assert(!is_unsigned_integral<int4>);
   static_assert(is_unsigned_integral<uint8>);
   static_assert(!is_unsigned_integral<int const>);
   static_assert(!is_unsigned_integral<int4 const>);
   static_assert(is_unsigned_integral<uint8 const>);
}

$test(meta_copy_type_traits) {
   using namespace cat;

   static_assert(is_same<copy_const_from<int, int>, int>);
   static_assert(is_same<copy_const_from<int const, int>, int const>);
   static_assert(is_same<copy_const_from<int, int const>, int>);
   static_assert(is_same<copy_const_from<int const&, int>, int const>);
   static_assert(is_same<copy_const_from<int const&&, int>, int const>);
   static_assert(is_same<copy_const_from<int&, int const>, int>);
   static_assert(is_same<copy_const_from<int&&, int const>, int>);

   static_assert(is_same<copy_cv_from<int, int>, int>);
   static_assert(is_same<copy_cv_from<int, int const>, int>);
   static_assert(is_same<copy_cv_from<int const, int>, int const>);
   static_assert(is_same<copy_cv_from<int const&, int>, int const>);
   static_assert(is_same<copy_cv_from<int const&&, int>, int const>);
   static_assert(is_same<copy_cv_from<int&, int const>, int>);
   static_assert(is_same<copy_cv_from<int&&, int const>, int>);

   static_assert(is_same<copy_ref_from<int, int>, int>);
   static_assert(is_same<copy_ref_from<int const, int>, int>);
   static_assert(is_same<copy_ref_from<int const&, int>, int&>);
   static_assert(is_same<copy_ref_from<int const&&, int>, int&&>);
   static_assert(is_same<copy_ref_from<int, int const>, int const>);
   static_assert(is_same<copy_ref_from<int&, int const>, int const&>);
   static_assert(is_same<copy_ref_from<int&&, int const>, int const&&>);

   static_assert(is_same<copy_cvref_from<int, int>, int>);
   static_assert(is_same<copy_cvref_from<int const, int>, int const>);
   static_assert(is_same<copy_cvref_from<int const&, int>, int const&>);
   static_assert(is_same<copy_cvref_from<int const&&, int>, int const&&>);
   static_assert(is_same<copy_cvref_from<int, int const>, int>);
   static_assert(is_same<copy_cvref_from<int&, int const>, int&>);
   static_assert(is_same<copy_cvref_from<int&&, int const>, int&&>);
   static_assert(is_same<copy_cvref_from<int const, int&>, int const>);
}

$test(meta_function_enum_and_common_type) {
   using namespace cat;

   auto lambda = []() {
      return 0;
   };

   static_assert(!is_function<int>);
   static_assert(!is_function<int const>);
   static_assert(is_function<int(int)>);
   static_assert(is_function<decltype(test_is_function)>);
   static_assert(!is_function<decltype(lambda)>);

   auto p_decayed_lambda = +[] {
   };
   static_assert(is_pointer<decltype(p_decayed_lambda)>);
   static_assert(is_pointer<decltype(+[] {
   })>);
   static_assert(!is_pointer<decltype([] {
   })>);

   enum enum_type : int {
   };
   enum class enum_class_type : int {
   };

   static_assert(is_same<underlying_type<enum_type>, int>);
   static_assert(is_same<underlying_type<enum_class_type>, int>);

   static_assert(is_sizable<int>);
   static_assert(!is_sizable<void>);

   static_assert(is_same<common_type<int, long int>, long int>);
   static_assert(is_same<common_type<long int>, long int>);
   static_assert(is_same<common_type<int, int>, int>);
   static_assert(is_same<common_type<int const, int>, int>);
   static_assert(is_same<common_type<int, long int, unsigned long int>,
                         unsigned long int>);
}

$test(meta_common_reference) {
   using namespace cat;

   static_assert(is_same<common_reference<int, int>, int>);
   static_assert(is_same<common_reference<int&, int&>, int&>);
   static_assert(is_same<common_reference<int&&, int&&>, int&&>);
   static_assert(is_same<common_reference<int, int&>, int>);
   static_assert(is_same<common_reference<int&, int>, int>);
   static_assert(is_same<common_reference<int&&, int>, int>);
   static_assert(is_same<common_reference<int, int&&>, int>);

   static_assert(is_same<common_reference<int const, int>, int>);
   static_assert(is_same<common_reference<int const&, int&>, int const&>);
   static_assert(is_same<common_reference<int const&&, int&&>, int const&&>);
   static_assert(is_same<common_reference<int, int const>, int>);
   static_assert(is_same<common_reference<int&, int const&>, int const&>);
   static_assert(is_same<common_reference<int&&, int const&&>, int const&&>);

   static_assert(is_same<common_reference<int&, int&&>, int const&>);
   static_assert(is_same<common_reference<int const&&, int&>, int const&>);
   static_assert(is_same<common_reference<int&, int const&&>, int const&>);
   static_assert(is_same<common_reference<int const&, int&&>, int const&>);
   static_assert(is_same<common_reference<int, int, int&>, int>);

   // `cat::reference_wrapper<T>` participates in `common_reference` as a
   // transparent proxy for `T&`.
   static_assert(is_same<common_reference<reference_wrapper<int>, int&>, int&>);
   static_assert(is_same<common_reference<int&, reference_wrapper<int>>, int&>);
   static_assert(
      is_same<common_reference<reference_wrapper<int>, reference_wrapper<int>>,
              int&>);
   static_assert(
      is_same<common_reference<reference_wrapper<int const>, int const&>,
              int const&>);
   static_assert(is_same<common_reference<reference_wrapper<int>, int const&>,
                         int const&>);
   // `is_reference_wrapper` holds for any `cat::reference_wrapper<T>`,
   // including partial specializations.
   static_assert(is_reference_wrapper<reference_wrapper<int>>);
   static_assert(is_reference_wrapper<reference_wrapper<int> const&>);
   static_assert(!is_reference_wrapper<int>);
   static_assert(!is_reference_wrapper<int*>);

   // TODO: This is supposed to work. It is blocked by the `tuple` conversion
   // operator.
   // static_assert(
   //     is_same<common_reference<tuple<int, double>, tuple<int&, double&>>,
   //             tuple<int, double>>);
   // using test_tuple_common_ref = common_reference<tuple<int, double>,
   //     tuple<int&, double&>>;
}

$test(meta_common_comparison) {
   using namespace cat;

   static_assert(is_same<std::common_comparison_category_t<std::strong_ordering,
                                                           std::weak_ordering>,
                         std::weak_ordering>);

   static_assert(
      is_same<std::common_comparison_category_t<std::weak_ordering,
                                                std::strong_ordering>,
              std::weak_ordering>);

   static_assert(
      is_same<std::common_comparison_category_t<std::partial_ordering,
                                                std::weak_ordering>,
              std::partial_ordering>);

   static_assert(
      is_same<std::common_comparison_category_t<std::weak_ordering,
                                                std::partial_ordering>,
              std::partial_ordering>);

   static_assert(
      is_same<std::common_comparison_category_t<std::partial_ordering,
                                                std::strong_ordering>,
              std::partial_ordering>);

   static_assert(
      is_same<std::common_comparison_category_t<std::strong_ordering,
                                                std::partial_ordering>,
              std::partial_ordering>);

   // Reduces across three or more arguments. The fold weakens
   // pairwise from the left.
   static_assert(
      is_same<
         std::common_comparison_category_t<
            std::strong_ordering, std::strong_ordering, std::strong_ordering>,
         std::strong_ordering>);

   static_assert(
      is_same<
         std::common_comparison_category_t<
            std::strong_ordering, std::weak_ordering, std::strong_ordering>,
         std::weak_ordering>);

   static_assert(
      is_same<
         std::common_comparison_category_t<
            std::strong_ordering, std::partial_ordering, std::weak_ordering>,
         std::partial_ordering>);

   // Empty pack: identity for the weakening operation is the strongest
   // category. Matches `std::common_comparison_category_t<>`.
   static_assert(
      is_same<std::common_comparison_category_t<>, std::strong_ordering>);

   // Single comparison-category argument is itself.
   static_assert(
      is_same<std::common_comparison_category_t<std::strong_ordering>,
              std::strong_ordering>);
   static_assert(is_same<std::common_comparison_category_t<std::weak_ordering>,
                         std::weak_ordering>);
   static_assert(
      is_same<std::common_comparison_category_t<std::partial_ordering>,
              std::partial_ordering>);

   // `[cmp.common]`: any non-comparison-category argument collapses the
   // result to `void`, matching `std::common_comparison_category`.
   static_assert(is_same<std::common_comparison_category_t<int>, void>);
   static_assert(
      is_same<std::common_comparison_category_t<std::strong_ordering, int>,
              void>);
}

// P0870R7 `is_convertible_without_narrowing<From, To>`. Holds when `From`
// is implicitly convertible to `To` and the conversion is not narrowing.
$test(meta_is_convertible_without_narrowing) {
   using namespace cat;

   // Widening within a category is non-narrowing.
   static_assert(is_convertible_without_narrowing<short, int>);
   static_assert(is_convertible_without_narrowing<int, long>);
   static_assert(is_convertible_without_narrowing<float, double>);

   // Narrowing within a category.
   static_assert(!is_convertible_without_narrowing<long, int>);
   static_assert(!is_convertible_without_narrowing<double, float>);

   // Crossing the integer / floating-point boundary is narrowing unless
   // the source is a constant expression that round-trips.
   static_assert(!is_convertible_without_narrowing<double, int>);
   static_assert(!is_convertible_without_narrowing<int, double>);

   // Unrelated types fail the `is_convertible` precondition.
   static_assert(!is_convertible_without_narrowing<float, char const*>);

   // P3146 motivating case. The conversion through `cat::constant<42>`'s
   // `constexpr operator int()` is a constant expression that round-trips
   // losslessly into `float`, so the trait holds.
   static_assert(is_convertible_without_narrowing<constant<42>, float>);

   // libCat safe-integer widenings within the same signedness.
   static_assert(is_convertible_without_narrowing<int1, int2>);
   static_assert(is_convertible_without_narrowing<int2, int4>);
   static_assert(is_convertible_without_narrowing<int4, int8>);
   static_assert(is_convertible_without_narrowing<uint1, uint2>);
   static_assert(is_convertible_without_narrowing<uint2, uint4>);
   static_assert(is_convertible_without_narrowing<uint4, uint8>);

   // Same-type identity conversions never narrow.
   static_assert(is_convertible_without_narrowing<int4, int4>);
   static_assert(is_convertible_without_narrowing<uint4, uint4>);
   static_assert(is_convertible_without_narrowing<float4, float4>);

   // libCat safe-integer narrowings: rejected either at the
   // `is_convertible` precondition (when the converting constructor is
   // `explicit`) or by the narrowing check itself. Either way the trait
   // is `false`.
   static_assert(!is_convertible_without_narrowing<int8, int4>);
   static_assert(!is_convertible_without_narrowing<int4, int2>);
   static_assert(!is_convertible_without_narrowing<int2, int1>);
   static_assert(!is_convertible_without_narrowing<uint8, uint4>);
   static_assert(!is_convertible_without_narrowing<uint4, uint2>);
   static_assert(!is_convertible_without_narrowing<uint2, uint1>);

   // Signedness mismatch at the same width is narrowing in either
   // direction.
   static_assert(!is_convertible_without_narrowing<int4, uint4>);
   static_assert(!is_convertible_without_narrowing<uint4, int4>);

   // libCat safe-float cross-precision conversions are `explicit` in
   // either direction (basic_float intentionally never converts a runtime
   // float between widths implicitly), so the `is_convertible`
   // precondition fails and the trait is `false` regardless of the
   // narrowing rule.
   static_assert(!is_convertible_without_narrowing<float4, float8>);
   static_assert(!is_convertible_without_narrowing<float8, float4>);

   // Integer / floating-point boundary in either direction is narrowing
   // without a constant-expression source.
   static_assert(!is_convertible_without_narrowing<int4, float4>);
   static_assert(!is_convertible_without_narrowing<float4, int4>);
   static_assert(!is_convertible_without_narrowing<int8, float8>);
   static_assert(!is_convertible_without_narrowing<float8, int8>);
}

$test(meta_member_pointer) {
   using namespace cat;

   // Test member type traits.
   static_assert(is_member_pointer<decltype(&members::member_function)>);
   static_assert(
      is_member_function_pointer<decltype(&members::member_function)>);
   static_assert(!is_member_function_pointer<decltype(&test_is_function)>);

   members members;
   int members::* p_member_variable = &members::member_variable;
   members.*p_member_variable = 1;
   static_assert(is_member_pointer<decltype(p_member_variable)>);
   static_assert(is_member_object_pointer<decltype(p_member_variable)>);
   static_assert(is_member_object_pointer<int members::*>);
}

$test(meta_is_specialization) {
   using namespace cat;

   // Test `is_specialization`.
   static_assert(is_specialization<templated_one<int>, templated_one>);
   static_assert(!is_specialization<templated_one<int>, templated_two>);
}

$test(meta_sign_type_traits) {
   using namespace cat;

   // Test signed-ness traits.
   static_assert(is_signed<make_signed_type<unsigned>>);
   static_assert(is_signed<make_signed_type<unsigned const>>);
   static_assert(is_signed<make_signed_type<uint4>>);
   static_assert(is_signed<make_signed_type<float>>);
   static_assert(is_signed<make_signed_type<double>>);

   static_assert(!is_signed<make_unsigned_type<int>>);
   static_assert(!is_signed<make_unsigned_type<int4>>);
   static_assert(!is_signed<make_unsigned_type<int4 const>>);

   static_assert(is_signed<copy_sign_from<int4, unsigned>>);
   static_assert(is_signed<copy_sign_from<int, uint4>>);
   static_assert(is_signed<copy_sign_from<int, unsigned>>);

   static_assert(!is_signed<copy_sign_from<uint4, int>>);
   static_assert(!is_signed<copy_sign_from<unsigned, int4>>);
   static_assert(!is_signed<copy_sign_from<unsigned, int>>);
}

$test(meta_trivially_copy_construct) {
   using namespace cat;

   struct default_construct {};

   static_assert(is_copy_constructible<default_construct>);
}

namespace {
auto
add_two(int a, int b) -> int {
   return a + b;
}

struct opaque_return {};

auto
return_opaque(int x) -> opaque_return {
   (void)x;
   return {};
}

auto
is_positive(int x) -> bool {
   return x > 0;
}

struct member_receiver {
   auto
   positive(int x) const -> bool {
      return x > 0;
   }
};

struct non_convertible_to_bool {
   explicit
   operator bool() = delete;
};

auto
bad_predicate_return(int x) -> non_convertible_to_bool {
   (void)x;
   return {};
}
}  // namespace

$test(meta_is_invocable) {
   using namespace cat;
   static_assert(is_invocable<decltype(&add_two), int, int>);
   static_assert(!is_invocable<decltype(&add_two), int>);
   static_assert(!is_invocable<int>);

   static_assert(is_invocable<bool (member_receiver::*)(int) const,
                              member_receiver const&, int>);
}

$test(meta_is_predicate) {
   using namespace cat;
   static_assert(is_predicate<decltype(&is_positive), int>);
   static_assert(is_predicate<bool (member_receiver::*)(int) const,
                              member_receiver const&, int>);

   static_assert(!is_predicate<decltype(&add_two), int, int, int>);
   static_assert(!is_predicate<decltype(&return_opaque), int>);
   static_assert(!is_predicate<decltype(&bad_predicate_return), int>);
}
