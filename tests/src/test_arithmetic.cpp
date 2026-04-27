#include <cat/arithmetic>
#include <cat/bit>
#include <cat/match>
#include <cat/type_list>

#include "../unit_tests.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"

template <auto value>
struct nttp {
   static constexpr auto member = value;
};

using namespace cat::literals;
using namespace cat::integers;

test(arithmetic_detail_raw_implicit_storage) {
   constexpr int int_into_float_zero = 0;
   constexpr int int_into_float_small = 42;
   constexpr int int_into_float_last_exact = 16'777'216;
   constexpr int int_into_float_first_inexact = 16'777'217;
   static_assert(cat::detail::raw_source_fits_implicit_storage<float>(
      int_into_float_zero));
   static_assert(cat::detail::raw_source_fits_implicit_storage<float>(
      int_into_float_small));
   static_assert(cat::detail::raw_source_fits_implicit_storage<float>(
      int_into_float_last_exact));
   static_assert(!cat::detail::raw_source_fits_implicit_storage<float>(
      int_into_float_first_inexact));
   static_assert(
      cat::detail::raw_source_fits_implicit_storage<cat::uint1::raw_type>(200));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::uint1::raw_type>(
         300));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::uint1::raw_type>(-1));
   static_assert(
      cat::detail::raw_source_fits_implicit_storage<cat::int1::raw_type>(100u));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::int1::raw_type>(
         300u));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::uint8::raw_type>(
         -1ll));

   static_assert(
      cat::detail::raw_source_fits_implicit_storage<cat::idx::raw_type>(200));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::idx::raw_type>(-1));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::idx::raw_type>(-1ll));

   static_assert(cat::detail::raw_source_fits_implicit_storage<
                 cat::uintptr<void>::raw_type>(200));
   static_assert(!cat::detail::raw_source_fits_implicit_storage<
                 cat::uintptr<void>::raw_type>(-1));
   static_assert(!cat::detail::raw_source_fits_implicit_storage<
                 cat::uintptr<void>::raw_type>(-1ll));

   static_assert(cat::detail::raw_source_fits_implicit_storage<
                 cat::intptr<void>::raw_type>(100));
   static_assert(cat::detail::raw_source_fits_implicit_storage<
                 cat::intptr<void>::raw_type>(-1));
}

test(arithmetic_traits_common_type) {
   static_assert(cat::is_same<cat::common_type<idx, iword>, iword>);
   static_assert(cat::is_same<cat::common_type<iword, idx>, iword>);
}

test(arithmetic_integers_are_implicit_lifetime) {
   static_assert(cat::is_implicit_lifetime<int1>);
   static_assert(cat::is_implicit_lifetime<uint1>);
   static_assert(cat::is_implicit_lifetime<int2>);
   static_assert(cat::is_implicit_lifetime<uint2>);
   static_assert(cat::is_implicit_lifetime<int4>);
   static_assert(cat::is_implicit_lifetime<uint4>);
   static_assert(cat::is_implicit_lifetime<int8>);
   static_assert(cat::is_implicit_lifetime<uint8>);
   static_assert(cat::is_implicit_lifetime<idx>);
   static_assert(cat::is_implicit_lifetime<iword>);
   static_assert(cat::is_implicit_lifetime<uword>);
   static_assert(cat::is_implicit_lifetime<float4>);
   static_assert(cat::is_implicit_lifetime<float8>);
   static_assert(cat::is_implicit_lifetime<intptr<void>>);
   static_assert(cat::is_implicit_lifetime<uintptr<void>>);
   static_assert(cat::is_implicit_lifetime<intptr<int>>);
   static_assert(cat::is_implicit_lifetime<uintptr<int>>);
   static_assert(cat::is_implicit_lifetime<intptr<float>>);
   static_assert(cat::is_implicit_lifetime<uintptr<float>>);
   static_assert(cat::is_implicit_lifetime<intptr<double>>);
}

test(arithmetic_traits_is_arithmetic) {
   static_assert(cat::is_arithmetic<char>);
   static_assert(cat::is_arithmetic<signed char>);
   static_assert(cat::is_arithmetic<unsigned char>);
   static_assert(cat::is_arithmetic<short>);
   static_assert(cat::is_arithmetic<unsigned short>);
   static_assert(cat::is_arithmetic<int>);
   static_assert(cat::is_arithmetic<unsigned int>);
   static_assert(cat::is_arithmetic<long long>);
   static_assert(cat::is_arithmetic<unsigned long long>);
   static_assert(cat::is_arithmetic<idx>);
}

test(arithmetic_traits_is_assignable) {
   static_assert(cat::is_assignable<int8, uint4>);
   static_assert(!cat::is_assignable<uint4, int2>);
   static_assert(!cat::is_assignable<uint8, int4>);
   static_assert(!cat::is_assignable<int4, uint4>);
   static_assert(!cat::is_assignable<uint4, int4>);
}

test(arithmetic_traits_is_constructible) {
   static_assert(cat::is_constructible<idx, uword>);
   static_assert(cat::is_constructible<iword, idx>);
}

test(arithmetic_traits_is_convertible) {
   static_assert(cat::is_convertible<int2, int4>);
   static_assert(cat::is_convertible<uint2, uint4>);
   static_assert(cat::is_convertible<uint2, int4>);
   static_assert(!cat::is_convertible<idx, int>);
   static_assert(!cat::is_convertible<idx, int4>);
   static_assert(cat::is_convertible<unsigned char, idx>);
   static_assert(cat::is_convertible<uint1, idx>);
   static_assert(cat::is_convertible<unsigned short, idx>);
   static_assert(cat::is_convertible<uint2, idx>);
   static_assert(cat::is_convertible<unsigned int, idx>);
   static_assert(cat::is_convertible<uint4, idx>);
   static_assert(!cat::is_convertible<uint4, int4>);
   static_assert(!cat::is_convertible<int4, uint4>);
}

test(arithmetic_traits_is_explicitly_constructible) {
   static_assert(cat::is_explicitly_constructible<float4, int>);
}

test(arithmetic_traits_is_implicitly_constructible) {
   static_assert(cat::is_implicitly_constructible<uword, uword>);
   static_assert(!cat::is_implicitly_constructible<idx, uword>);
   static_assert(cat::is_implicitly_constructible<uword, idx>);
   static_assert(cat::is_implicitly_constructible<iword, idx>);
   static_assert(!cat::is_implicitly_constructible<idx, iword>);
   static_assert(!cat::is_implicitly_constructible<int4, uint4>);
   static_assert(!cat::is_implicitly_constructible<uint4, int4>);
   static_assert(!cat::is_implicitly_constructible<float4, int>);
}

test(arithmetic_traits_is_integral) {
   static_assert(cat::is_integral<int4>);
   static_assert(cat::is_integral<uint4>);
   static_assert(cat::is_integral<intptr<void>>);
   static_assert(cat::is_integral<uintptr<void>>);
}

test(arithmetic_traits_is_lvalue_reference) {
   static_assert(!cat::is_lvalue_reference<int4 const>);
}

test(arithmetic_traits_is_safe_arithmetic) {
   static_assert(cat::is_safe_arithmetic<idx>);
}

test(arithmetic_traits_is_safe_arithmetic_comparison) {
   static_assert(!cat::is_safe_arithmetic_comparison<int4, uint4>);
   static_assert(!cat::is_safe_arithmetic_comparison<uint4, int4>);
   static_assert(!cat::is_safe_arithmetic_comparison<int2, uint2>);
   static_assert(!cat::is_safe_arithmetic_comparison<int4, float4>);
   static_assert(!cat::is_safe_arithmetic_comparison<float4, uint4>);
}

test(arithmetic_traits_is_safe_arithmetic_conversion) {
   static_assert(cat::is_safe_arithmetic_conversion<idx, iword>);
   static_assert(!cat::is_safe_arithmetic_conversion<int8, int4>);
   static_assert(!cat::is_safe_arithmetic_conversion<uint8, uint4>);
   static_assert(!cat::is_safe_arithmetic_conversion<int4, uint4>);
   static_assert(!cat::is_safe_arithmetic_conversion<uint4, int4>);
}

test(arithmetic_traits_is_same) {
   static_assert(cat::is_same<decltype(uintptr{static_cast<int*>(nullptr)}),
                              uintptr<int>>);
   static_assert(cat::is_same<decltype(uintptr{static_cast<void*>(nullptr)}),
                              uintptr<void>>);
   static_assert(cat::is_same<decltype(cat::arithmetic_ptr{
                                 static_cast<unsigned char*>(nullptr)}),
                              uintptr<unsigned char>>);

   static_assert(cat::is_same<cat::raw_arithmetic_type<int>, int>);
   static_assert(cat::is_same<cat::raw_arithmetic_type<int4>, int4::raw_type>);

   static_assert(cat::is_same<cat::int_fixed<1>, int1>);
   static_assert(cat::is_same<cat::int_fixed<2>, int2>);
   static_assert(cat::is_same<cat::int_fixed<4>, int4>);
   static_assert(cat::is_same<cat::int_fixed<8>, int8>);

   static_assert(cat::is_same<cat::uint_fixed<1>, uint1>);
   static_assert(cat::is_same<cat::uint_fixed<2>, uint2>);
   static_assert(cat::is_same<cat::uint_fixed<4>, uint4>);
   static_assert(cat::is_same<cat::uint_fixed<8>, uint8>);

   static_assert(
      cat::is_same<decltype(uword::max() - uintptr<void>{8u}), uword>);
   static_assert(cat::is_same<decltype(iword::max() - intptr<void>{8}), iword>);

   static_assert(cat::is_same<decltype(uword{1} - 1_u4), uword>);
   static_assert(cat::is_same<decltype(uint4{5} - uint2{3}), uint4>);
   static_assert(cat::is_same<decltype(uint8{9} - uint1{1}), uint8>);

   static_assert(cat::is_same<decltype(iword{9} - intptr<void>{5}), iword>);

   static_assert(cat::is_same<decltype(intptr<void>{} + 1), intptr<void>>);
   static_assert(cat::is_same<decltype(intptr<void>{} - 1), intptr<void>>);

   static_assert(cat::is_same<decltype(cat::make_unsigned(1)), unsigned int>);
   static_assert(cat::is_same<decltype(cat::make_signed(1u)), int>);
   static_assert(cat::is_same<decltype(cat::make_unsigned(1_i4)), uint4>);
   static_assert(cat::is_same<decltype(cat::make_signed(1_u4)), int4>);
   static_assert(cat::is_same<decltype(cat::make_unsigned(idx(1u))), idx>);

   static_assert(cat::is_same<decltype(1_idx + 1_sz), iword>);
   static_assert(cat::is_same<decltype(1_sz + 1_idx), iword>);
   static_assert(cat::is_same<decltype(1_idx + 1_uz), uword>);
   static_assert(cat::is_same<decltype(1_uz + 1_idx), uword>);
   static_assert(cat::is_same<decltype(1_i4 + 1_idx), idx>);
   static_assert(cat::is_same<decltype(1_idx + 1_i4), iword>);

   static_assert(cat::is_same<decltype(1._f8 + 2._f4), float8>);
   static_assert(cat::is_same<decltype(2_f4 + 3.f), float4>);
   static_assert(cat::is_same<decltype(3.f + 2_f4), float4>);
}

test(arithmetic_traits_is_signed) {
   static_assert(cat::is_signed<int>);
   static_assert(cat::is_signed<float>);
   static_assert(cat::is_signed<float4>);
   static_assert(cat::is_signed<int4>);
   static_assert(!cat::is_signed<uint4>);
}

test(arithmetic_traits_is_signed_integral) {
   static_assert(cat::is_signed_integral<int>);
   static_assert(!cat::is_signed_integral<unsigned>);
   static_assert(!cat::is_signed_integral<float>);
}

test(arithmetic_traits_is_trivially_copyable_and_default_constructible) {
   static_assert(cat::is_trivially_copyable<int1>
                 && cat::is_trivially_default_constructible<int1>);
   static_assert(cat::is_trivially_copyable<uint1>
                 && cat::is_trivially_default_constructible<uint1>);
   static_assert(cat::is_trivially_copyable<int2>
                 && cat::is_trivially_default_constructible<int2>);
   static_assert(cat::is_trivially_copyable<uint2>
                 && cat::is_trivially_default_constructible<uint2>);
   static_assert(cat::is_trivially_copyable<int4>
                 && cat::is_trivially_default_constructible<int4>);
   static_assert(cat::is_trivially_copyable<uint4>
                 && cat::is_trivially_default_constructible<uint4>);
   static_assert(cat::is_trivially_copyable<int8>
                 && cat::is_trivially_default_constructible<int8>);
   static_assert(cat::is_trivially_copyable<uint8>
                 && cat::is_trivially_default_constructible<uint8>);
   static_assert(cat::is_trivially_copyable<float4>
                 && cat::is_trivially_default_constructible<float4>);
   static_assert(cat::is_trivially_copyable<float8>
                 && cat::is_trivially_default_constructible<float8>);
}

test(arithmetic_traits_is_trivially_copyable) {
   static_assert(cat::is_trivially_copyable<int1>);
}

test(arithmetic_traits_is_trivially_relocatable) {
   static_assert(cat::is_trivially_relocatable<int1>);
   static_assert(cat::is_trivially_relocatable<uint1>);
   static_assert(cat::is_trivially_relocatable<int2>);
   static_assert(cat::is_trivially_relocatable<uint2>);
   static_assert(cat::is_trivially_relocatable<int4>);
   static_assert(cat::is_trivially_relocatable<uint4>);
   static_assert(cat::is_trivially_relocatable<int8>);
   static_assert(cat::is_trivially_relocatable<uint8>);
   static_assert(cat::is_trivially_relocatable<float4>);
   static_assert(cat::is_trivially_relocatable<float8>);
   static_assert(cat::is_trivially_relocatable<intptr<void>>);
   static_assert(cat::is_trivially_relocatable<intptr<int>>);
   static_assert(cat::is_trivially_relocatable<uintptr<void>>);
   static_assert(cat::is_trivially_relocatable<uintptr<int>>);
}

test(arithmetic_traits_is_unsigned) {
   static_assert(cat::is_unsigned<unsigned>);
   static_assert(cat::is_unsigned<idx>);
   static_assert(!cat::is_unsigned<float>);
   static_assert(!cat::is_unsigned<float4>);
   static_assert(cat::is_unsigned<uint4>);
   static_assert(!cat::is_unsigned<int4>);
}

test(arithmetic_traits_is_unsigned_integral) {
   static_assert(!cat::is_unsigned_integral<int>);
   static_assert(cat::is_unsigned_integral<unsigned>);
   static_assert(!cat::is_unsigned_integral<float>);
}

test(arithmetic_traits_is_unsafe_arithmetic) {
   static_assert(cat::is_unsafe_arithmetic<int>);
   static_assert(cat::is_unsafe_arithmetic<unsigned long>);
   static_assert(cat::is_unsafe_arithmetic<double>);
   static_assert(!cat::is_unsafe_arithmetic<int4>);
   static_assert(!cat::is_unsafe_arithmetic<uintptr<void>>);
   static_assert(!cat::is_unsafe_arithmetic<idx>);
}

test(arithmetic_traits_rvalue) {
   int4 const constant_int4 = 0;
   static_assert(cat::rvalue<decltype(constant_int4.wrap().undef())>);
   static_assert(cat::rvalue<decltype(int4{0}.wrap().undef())>);

   static_assert(cat::rvalue<decltype(constant_int4.wrap())>);
   static_assert(cat::rvalue<decltype(int4{0}.wrap())>);

   static_assert(cat::rvalue<decltype(constant_int4.sat())>);
   static_assert(cat::rvalue<decltype(int4{0}.sat())>);

   idx const constant_idx = 0;

   static_assert(cat::rvalue<decltype(constant_idx.wrap().undef())>);
   static_assert(cat::rvalue<decltype(idx{0}.wrap().undef())>);

   static_assert(cat::rvalue<decltype(constant_idx.wrap())>);
   static_assert(cat::rvalue<decltype(idx{0}.wrap())>);

   static_assert(cat::rvalue<decltype(constant_idx.sat())>);
   static_assert(cat::rvalue<decltype(idx{0}.sat())>);

   uintptr<void> const constant_uptr = nullptr;

   static_assert(cat::rvalue<decltype(constant_uptr.wrap().undef())>);
   static_assert(cat::rvalue<decltype(uintptr<void>{0}.wrap().undef())>);

   static_assert(cat::rvalue<decltype(constant_uptr.wrap())>);
   static_assert(cat::rvalue<decltype(uintptr<void>{0}.wrap())>);

   static_assert(cat::rvalue<decltype(constant_uptr.sat())>);
   static_assert(cat::rvalue<decltype(uintptr<void>{0}.sat())>);
}

test(arithmetic_constants_constexpr_helpers) {
   static_assert(cat::limits<int4>::max() ==  // NOLINT
                 cat::limits<int4::raw_type>::max());
   static_assert(cat::limits<uint8>::max() ==  // NOLINT
                 cat::limits<uint8::raw_type>::max());
   static_assert(cat::limits<float4>::max()  // NOLINTNEXTLINE
                 == cat::limits<float4::raw_type>::max());
   static_assert(cat::limits<float8>::min()
                 == cat::limits<float8::raw_type>::min());  // NOLINT
   static_assert(cat::limits<float8>::max()
                 == cat::limits<float8::raw_type>::max());  // NOLINT
   static_assert(cat::limits<float4>::min()
                 == cat::limits<float4::raw_type>::min());  // NOLINT
   static_assert(cat::limits<float4>::max()
                 == cat::limits<float4::raw_type>::max());  // NOLINT

   static_assert(idx_max == cat::limits<idx>::max());

   constexpr int int_into_float_small = 42;
   static_assert(float4(int_into_float_small).raw
                 == static_cast<float>(int_into_float_small));

   static_assert(cat::make_sign_from<int4>(1u) == 1_i4);
   static_assert(cat::make_sign_from(2, 1u) == 1_i4);

   static_assert(cat::limits<cat::iword>::max()
                 < cat::limits<cat::uword>::max());
   static_assert(cat::limits<cat::uword>::max()
                 > cat::limits<cat::iword>::max());
   static_assert(cat::limits<cat::iword>::min()
                 < cat::limits<cat::uword>::max());

   // `uword{-1}` would silently wrap a signed sentinel into the unsigned
   // domain. Express the intent through the `wrap` view explicitly.
   static_assert(cat::wrap_uint8(-1) == cat::limits<uword>::max());
}

test(arithmetic_arithmetic_ptr_deduction_from_pointers) {
   int storage{};
   int* p_storage = __builtin_addressof(storage);
   uintptr const deduced{p_storage};
   uintptr<int> const explicit_spelling{p_storage};
   cat::verify(deduced == explicit_spelling);
   intptr const deduced_signed{p_storage};
   intptr<int> const explicit_spelling_signed{p_storage};
   cat::verify(deduced_signed == explicit_spelling_signed);
}

test(arithmetic_raw_types_make_raw) {
   static_assert(cat::make_raw_arithmetic(7) == 7);
   static_assert(cat::make_raw_arithmetic(3.25) == 3.25);
   static_assert(cat::make_raw_arithmetic(int4{88}) == 88);
   static_assert(cat::make_raw_arithmetic(uint4{11u}) == 11u);
}

test(arithmetic_numeral_sizes) {
   // Test numerals' size,
   static_assert(sizeof(int1) == 1);
   static_assert(sizeof(uint1) == 1);
   static_assert(sizeof(int2) == 2);
   static_assert(sizeof(uint2) == 2);
   static_assert(sizeof(int4) == 4);
   static_assert(sizeof(uint4) == 4);
   static_assert(sizeof(int8) == 8);
   static_assert(sizeof(float4) == 4);
   static_assert(sizeof(float8) == 8);
}

test(arithmetic_int4_uint4_operations_and_ordering) {
   // Test `int4` constructors and assignment.
   int4 test_int4_1 = 1;
   int4 test_int4_2{};
   cat::verify(test_int4_2 == 0);
   test_int4_2 = 1;
   cat::verify(test_int4_2 == 1);

   // Test `uint4` constructors and assignment.
   uint4 test_uint4_1 = 1u;
   uint4 test_uint4_2;
   test_uint4_2 = 1u;

   // Test `arithmetic` operators.
   int4 int4_add = 1 + test_int4_1;
   int4_add = 1_i4 + test_int4_1;
   int4 int4_sub = 1 - test_int4_1;
   int4_sub = 1_i4 - test_int4_1;

   int8 test_int8 = 100ll - test_int4_1;
   cat::verify(test_int8 == 99);
   test_int8 = 100ll + test_int4_1;
   test_int8 = 100ll * test_int4_1;
   test_int8 = 100ll / test_int4_1;

   cat::verify(int4{1} == int4{1});

   uint4 uint4_add = 1u + test_uint4_1;
   uint4_add = 1_u4 + test_uint4_1;
   uint4 uint4_sub = 1u - test_uint4_1;
   uint4_sub = 1_u4 - test_uint4_1;

   uint8 test_uint8 = 100ull - test_uint4_1;
   cat::verify(test_uint8 == 99u);
   test_uint8 = 100ull + test_uint4_1;
   test_uint8 = 100ull * test_uint4_1;
   test_uint8 = 100ull / test_uint4_1;

   cat::verify(uint4{1} == uint4{1u});

   // Greater than.
   cat::verify(int4{1} > int4{0});
   cat::verify(int4{1} >= int4{0});
   cat::verify(int4{1} >= int4{1});

   // Less than.
   cat::verify(int4{0} < int4{1});
   cat::verify(int4{0} <= int4{0});
   cat::verify(int4{0} <= int4{1});
}

test(arithmetic_intptr_operators_raw_reassign_and_safe_arithmetic) {
   // Test `arithmetic_ptr` operators on raw numerals.
   [[maybe_unused]]
   intptr<void> intptr_add_1 = 1 + intptr<void>{0};
   intptr<void> intptr_add_2 = intptr<void>{0} + 1;
   cat::verify(intptr_add_2 == 1);
   intptr<void> intptr_add_3 = 1_i4 + intptr<void>{0};
   cat::verify(intptr_add_3 == 1);
   intptr<void> intptr_add_4 = intptr<void>{0} + 1_i4;
   cat::verify(intptr_add_4 == 1);

   [[maybe_unused]]
   intptr<void> intptr_sub_1 = 1 - intptr_add_2;
   intptr<void> intptr_sub_2 = intptr_add_2 - 1;
   cat::verify(intptr_sub_2 == 0);
   intptr<void> intptr_sub_3 = 1_i4 - intptr_add_2;
   cat::verify(intptr_sub_3 == 0);
   intptr<void> intptr_sub_4 = intptr_add_2 - 1_i4;
   cat::verify(intptr_sub_4 == 0);

   // Test `arithmetic_ptr` operators on safe `arithmetic`s.
   intptr_add_2 = 1_i4 + intptr_add_2;
   intptr_add_2 = intptr_add_2 + 1_i4;
   intptr_sub_2 = 1_i4 - intptr_add_2;
   intptr_sub_2 = intptr_add_2 - 1_i4;

   cat::verify(uword{100} - uintptr<void>{40} == uword{60});
   cat::verify(uint4{500} - uint2{400} == uint4{100});

   cat::verify(iword{120} - intptr<void>{100} == iword{20});
}

test(arithmetic_intptr_arithmetic_ptr_operators_and_compare) {
   // Test `arithmetic_ptr` operators on other `arithmetic_ptr`s.
   cat::verify((intptr<void>{0} + intptr<void>{1}) == 1);
   cat::verify((intptr<void>{0} - intptr<void>{1}) == -1);

   cat::verify(intptr<void>{1} == intptr<void>{1});

   // Greater than.
   cat::verify(intptr<void>{1} > intptr<void>{0});
   cat::verify(intptr<void>{1} >= intptr<void>{0});
   cat::verify(intptr<void>{1} >= intptr<void>{1});

   // Less than.
   cat::verify(intptr<void>{0} < intptr<void>{1});
   cat::verify(intptr<void>{0} <= intptr<void>{0});
   cat::verify(intptr<void>{0} <= intptr<void>{1});
}

test(arithmetic_implicit_promotion_chains_and_assignability) {
   // Test integer promotion.
   int1 promote_int1 = 1_i1;
   int2 promote_int2 = promote_int1;
   int4 promote_int4 = promote_int2;
   int8 promote_int8 = promote_int2;
   promote_int8 = promote_int4;
   cat::assert(promote_int8 == 1);
   cat::assert(promote_int8.raw == 1);

   uint1 promote_uint1 = 1_u1;
   uint2 promote_uint2 = promote_uint1;
   uint4 promote_uint4 = promote_uint2;
   uint8 promote_uint8 = promote_uint2;
   promote_uint8 = promote_uint4;
   cat::assert(promote_uint8 == 1u);
   cat::assert(promote_uint8.raw == 1u);

   // Promote small unsigned ints to larger signed ints.
   promote_int2 = promote_uint1;
   promote_int4 = promote_uint2;

   promote_int8 = promote_uint1;
   promote_int8 = promote_uint2;
   promote_int8 = promote_uint4;
}

test(arithmetic_implicit_conversions_raw_storage_and_pointer_arithmetic) {
   // Raw values that fit convert implicitly when widening or for same-width
   // 64-bit types: signed to unsigned (`uint8 = 300ll`) and unsigned to signed
   // (`int8 = 300ull`), with matching `operator==` and `operator<=>` both ways.
   uint8 from_ll_implicit = 300ll;
   cat::verify(from_ll_implicit == 300);
   cat::verify(from_ll_implicit == 300ll);
   cat::verify(300 == from_ll_implicit);
   cat::verify(300ll == from_ll_implicit);
   from_ll_implicit = 300_u8;
   cat::verify(from_ll_implicit == 300);
   cat::verify(from_ll_implicit == 300ll);
   cat::verify(300 == from_ll_implicit);
   cat::verify(300ll == from_ll_implicit);

   cat::uint1 narrow_unsigned1 = 200;
   cat::uint1 narrow_unsigned2 = 200u;
   narrow_unsigned2 = 200_u4;
   cat::int1 narrow_signed1 = 100;
   cat::int1 narrow_signed2 = 100u;
   narrow_signed2 = 100_u4;
   cat::int1 narrow_signed3 = -100;

   cat::uint2 narrow_unsigned3 = 2'000;
   cat::uint2 narrow_unsigned4 = 2'000u;
   narrow_unsigned4 = 2'000_u4;
   cat::int2 narrow_signed4 = 1'000;
   cat::int2 narrow_signed5 = 1'000u;
   narrow_signed5 = 1'000_u4;
   cat::int2 narrow_signed6 = -1'000;

   constexpr cat::iword max_iword = cat::limits<cat::iword>::max();
   constexpr cat::uword max_uword = cat::limits<cat::uword>::max();
   cat::verify(!(max_iword > max_uword));
   cat::verify(!(max_uword < max_iword));

   int8 from_ull_implicit = 300ull;
   cat::verify(from_ll_implicit == 300u);
   cat::verify(from_ull_implicit == 300ull);
   cat::verify(300u == from_ull_implicit);
   cat::verify(300ull == from_ull_implicit);
   from_ull_implicit = 300_i8;
   cat::verify(from_ull_implicit == 300u);
   cat::verify(from_ull_implicit == 300ull);
   cat::verify(300u == from_ull_implicit);
   cat::verify(300ull == from_ull_implicit);

   // `int4` pointer arithmetic.
   char address;
   int* p_int4 = (reinterpret_cast<int*>(&address)) + 1_i4;
   p_int4 = 1_i4 + (reinterpret_cast<int*>(&address));
   p_int4 += 1_i4;
   p_int4 = p_int4 - 1_i4;
   p_int4 -= 1_i4;

   // `arithmetic_ptr` compound assignment operators on raw pointers.
   p_int4 += intptr<void>{1};
   p_int4 -= intptr<void>{1};
   p_int4 += uintptr<void>{1u};
   p_int4 -= uintptr<void>{1u};

   // `idx` pointer arithmetic.
   idx* p_idx = (reinterpret_cast<idx*>(&address)) + 1_i4;
   p_idx = 1_idx + (reinterpret_cast<idx*>(&address));
   p_idx += 1_idx;
   p_idx = p_idx - 1_idx;
   p_idx -= 1_idx;

   // `arithmetic_ptr` compound assignment operators on raw pointers.
   p_idx += intptr<void>{1};
   p_idx -= intptr<void>{1};
   p_idx += uintptr<void>{1u};
   p_idx -= uintptr<void>{1u};
}

// Cross-sign / narrowing implicit constant conversions from `constexpr` and
// `const` integral sources whose value fits the destination storage. These go
// through the `enable_if` constructor because the source is a constant
// expression but the conversion is not `is_safe_arithmetic_conversion`.
test(arithmetic_implicit_unsigned_fixed_width_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   uint1 u1_from_int_constexpr = signed_fit;
   uint1 u1_from_int_const = signed_const_fit;
   uint1 u1_from_uint_constexpr = unsigned_fit;
   uint1 u1_from_uint_const = unsigned_const_fit;
   uint2 u2_from_int_constexpr = signed_fit;
   uint2 u2_from_int_const = signed_const_fit;
   uint2 u2_from_uint_constexpr = unsigned_fit;
   uint2 u2_from_uint_const = unsigned_const_fit;
   uint4 u4_from_int_constexpr = signed_fit;
   uint4 u4_from_int_const = signed_const_fit;
   uint4 u4_from_uint_constexpr = unsigned_fit;
   uint4 u4_from_uint_const = unsigned_const_fit;
   uint8 u8_from_int_constexpr = signed_fit;
   uint8 u8_from_int_const = signed_const_fit;
   uint8 u8_from_uint_constexpr = unsigned_fit;
   uint8 u8_from_uint_const = unsigned_const_fit;
   cat::verify(u1_from_int_constexpr == 0_u1);
   cat::verify(u1_from_int_const == 0_u1);
   cat::verify(u1_from_uint_constexpr == 0_u1);
   cat::verify(u1_from_uint_const == 0_u1);
   cat::verify(u2_from_int_constexpr == 0_u2);
   cat::verify(u2_from_int_const == 0_u2);
   cat::verify(u2_from_uint_constexpr == 0_u2);
   cat::verify(u2_from_uint_const == 0_u2);
   cat::verify(u4_from_int_constexpr == 0_u4);
   cat::verify(u4_from_int_const == 0_u4);
   cat::verify(u4_from_uint_constexpr == 0_u4);
   cat::verify(u4_from_uint_const == 0_u4);
   cat::verify(u8_from_int_constexpr == 0_u8);
   cat::verify(u8_from_int_const == 0_u8);
   cat::verify(u8_from_uint_constexpr == 0_u8);
   cat::verify(u8_from_uint_const == 0_u8);

   // `=`, `+=`, and `+` accept the same sources via the implicit `enable_if`
   // constructor that converts the operand to the wrapped type.
   uint1 u1 = 1_u1;
   u1 = signed_fit;
   u1 = signed_const_fit;
   u1 = unsigned_fit;
   u1 = unsigned_const_fit;
   u1 += signed_fit;
   u1 += signed_const_fit;
   u1 += unsigned_fit;
   u1 += unsigned_const_fit;
   cat::verify(u1 == 0_u1);
   auto u1_sum_int_constexpr = u1 + signed_fit;
   auto u1_sum_int_const = u1 + signed_const_fit;
   auto u1_sum_uint_constexpr = u1 + unsigned_fit;
   auto u1_sum_uint_const = u1 + unsigned_const_fit;
   cat::verify(u1_sum_int_constexpr == 0_u1);
   cat::verify(u1_sum_int_const == 0_u1);
   cat::verify(u1_sum_uint_constexpr == 0_u1);
   cat::verify(u1_sum_uint_const == 0_u1);

   uint2 u2 = 1_u2;
   u2 = signed_fit;
   u2 = signed_const_fit;
   u2 = unsigned_fit;
   u2 = unsigned_const_fit;
   u2 += signed_fit;
   u2 += signed_const_fit;
   u2 += unsigned_fit;
   u2 += unsigned_const_fit;
   cat::verify(u2 == 0_u2);
   auto u2_sum_int_constexpr = u2 + signed_fit;
   auto u2_sum_int_const = u2 + signed_const_fit;
   auto u2_sum_uint_constexpr = u2 + unsigned_fit;
   auto u2_sum_uint_const = u2 + unsigned_const_fit;
   cat::verify(u2_sum_int_constexpr == 0_u2);
   cat::verify(u2_sum_int_const == 0_u2);
   cat::verify(u2_sum_uint_constexpr == 0_u2);
   cat::verify(u2_sum_uint_const == 0_u2);

   uint4 u4 = 1_u4;
   u4 = signed_fit;
   u4 = signed_const_fit;
   u4 = unsigned_fit;
   u4 = unsigned_const_fit;
   u4 += signed_fit;
   u4 += signed_const_fit;
   u4 += unsigned_fit;
   u4 += unsigned_const_fit;
   cat::verify(u4 == 0_u4);
   auto u4_sum_int_constexpr = u4 + signed_fit;
   auto u4_sum_int_const = u4 + signed_const_fit;
   auto u4_sum_uint_constexpr = u4 + unsigned_fit;
   auto u4_sum_uint_const = u4 + unsigned_const_fit;
   cat::verify(u4_sum_int_constexpr == 0_u4);
   cat::verify(u4_sum_int_const == 0_u4);
   cat::verify(u4_sum_uint_constexpr == 0_u4);
   cat::verify(u4_sum_uint_const == 0_u4);

   uint8 u8 = 1_u8;
   u8 = signed_fit;
   u8 = signed_const_fit;
   u8 = unsigned_fit;
   u8 = unsigned_const_fit;
   u8 += signed_fit;
   u8 += signed_const_fit;
   u8 += unsigned_fit;
   u8 += unsigned_const_fit;
   cat::verify(u8 == 0_u8);
   auto u8_sum_int_constexpr = u8 + signed_fit;
   auto u8_sum_int_const = u8 + signed_const_fit;
   auto u8_sum_uint_constexpr = u8 + unsigned_fit;
   auto u8_sum_uint_const = u8 + unsigned_const_fit;
   cat::verify(u8_sum_int_constexpr == 0_u8);
   cat::verify(u8_sum_int_const == 0_u8);
   cat::verify(u8_sum_uint_constexpr == 0_u8);
   cat::verify(u8_sum_uint_const == 0_u8);
}

// Compile-time-known cross-signedness constant operands work for every
// arithmetic operator on the fixed-width unsigned wrappers (`uint1` ..
// `uint8`).
test(arithmetic_unsigned_fixed_width_all_operators_constexpr_const) {
   constexpr int signed_two = 2;
   int const signed_const_two = 2;
   constexpr long long_two = 2;
   long const long_const_two = 2;

   // `uint4` (typical width). Compound and binary operators with signed
   // compile-time constants of various widths.
   uint4 u4 = 12_u4;
   u4 += signed_two;
   u4 -= signed_const_two;
   u4 *= long_two;
   u4 /= long_const_two;
   u4 %= signed_const_two;
   cat::verify(u4 == 0_u4);

   // Return types follow the new width rules:
   //   * `uint4 + int` (same order 4): width promoted, LHS wins -> `uint4`.
   //   * `uint4 * long` (order 4 vs 8): `*` widens to RHS -> `iword`.
   //   * `uint4 - / % long`: keeps LHS storage -> `uint4`.
   uint4 u4_six = 6_u4;
   cat::verify((u4_six + signed_two) == 8_u4);
   cat::verify((u4_six - signed_const_two) == 4_u4);
   cat::verify((u4_six * long_two) == 12_u4);
   cat::verify((u4_six / long_const_two) == 3_u4);
   cat::verify((u4_six % signed_const_two) == 0_u4);
   static_assert(cat::is_same<decltype(u4_six + signed_two), uint4>);
   static_assert(cat::is_same<decltype(u4_six - signed_const_two), uint4>);
   static_assert(cat::is_same<decltype(u4_six * long_two), iword>);
   static_assert(cat::is_same<decltype(u4_six / long_const_two), uint4>);
   static_assert(cat::is_same<decltype(u4_six % signed_const_two), uint4>);

   // `uint1` (narrowest width). The signed constant fits via the implicit
   // `enable_if` constructor. `+` and `*` widen to RHS (`int`, order 4) ->
   // `int4`. `-`, `/`, `%` keep the LHS's width -> `uint1`.
   uint1 u1 = 12_u1;
   u1 += signed_two;
   u1 -= signed_const_two;
   u1 *= signed_two;
   u1 /= signed_const_two;
   u1 %= signed_const_two;
   cat::verify(u1 == 0_u1);

   uint1 u1_six = 6_u1;
   cat::verify((u1_six + signed_two) == 8_u1);
   cat::verify((u1_six - signed_const_two) == 4_u1);
   static_assert(cat::is_same<decltype(u1_six + signed_two), int4>);
   static_assert(cat::is_same<decltype(u1_six - signed_const_two), uint1>);
   static_assert(cat::is_same<decltype(u1_six * signed_two), int4>);
   static_assert(cat::is_same<decltype(u1_six / signed_const_two), uint1>);
   static_assert(cat::is_same<decltype(u1_six % signed_const_two), uint1>);

   // `uint8` (widest width). Mixing with same-width signed `long` keeps the
   // `uint8` LHS storage. Smaller signed `int` similarly loses to `uint8`.
   uint8 u8 = 12_u8;
   u8 += signed_two;
   u8 -= signed_const_two;
   u8 *= long_two;
   u8 /= long_const_two;
   u8 %= signed_const_two;
   cat::verify(u8 == 0_u8);

   uint8 u8_six = 6_u8;
   cat::verify((u8_six + signed_two) == 8_u8);
   cat::verify((u8_six * long_two) == 12_u8);
   static_assert(cat::is_same<decltype(u8_six + signed_two), uint8>);
   static_assert(cat::is_same<decltype(u8_six - signed_const_two), uint8>);
   static_assert(cat::is_same<decltype(u8_six * long_two), uint8>);
   static_assert(cat::is_same<decltype(u8_six / long_const_two), uint8>);
   static_assert(cat::is_same<decltype(u8_six % signed_const_two), uint8>);

   // Bitwise and shift with cross-signedness compile-time constants on `uint4`.
   // `&` / `|` use `promoted_type` and resolve to `uint4` (same order, LHS
   // wins). `<<` / `>>` always keep the LHS storage type.
   uint4 u4_mask = 0b1100_u4;
   cat::verify((u4_mask & signed_two) == 0_u4);
   cat::verify((u4_mask | signed_two) == 0b1110_u4);
   u4_mask = 0b11_u4;
   cat::verify((u4_mask << signed_two) == 0b1100_u4);
   u4_mask = 0b1100_u4;
   cat::verify((u4_mask >> signed_two) == 0b11_u4);
   static_assert(cat::is_same<decltype(u4_mask & signed_two), uint4>);
   static_assert(cat::is_same<decltype(u4_mask | signed_two), uint4>);
   static_assert(cat::is_same<decltype(u4_mask << signed_two), uint4>);
   static_assert(cat::is_same<decltype(u4_mask >> signed_two), uint4>);

   u4_mask = 0b1100_u4;
   u4_mask &= signed_const_two;
   cat::verify(u4_mask == 0_u4);
   u4_mask = 0b1100_u4;
   u4_mask |= signed_const_two;
   cat::verify(u4_mask == 0b1110_u4);
   u4_mask = 0b11_u4;
   u4_mask <<= signed_const_two;
   cat::verify(u4_mask == 0b1100_u4);
   u4_mask = 0b1100_u4;
   u4_mask >>= signed_const_two;
   cat::verify(u4_mask == 0b11_u4);
}

test(arithmetic_implicit_signed_fixed_width_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   int1 i1_from_int_constexpr = signed_fit;
   int1 i1_from_int_const = signed_const_fit;
   int1 i1_from_uint_constexpr = unsigned_fit;
   int1 i1_from_uint_const = unsigned_const_fit;
   int2 i2_from_int_constexpr = signed_fit;
   int2 i2_from_int_const = signed_const_fit;
   int2 i2_from_uint_constexpr = unsigned_fit;
   int2 i2_from_uint_const = unsigned_const_fit;
   int4 i4_from_int_constexpr = signed_fit;
   int4 i4_from_int_const = signed_const_fit;
   int4 i4_from_uint_constexpr = unsigned_fit;
   int4 i4_from_uint_const = unsigned_const_fit;
   int8 i8_from_int_constexpr = signed_fit;
   int8 i8_from_int_const = signed_const_fit;
   int8 i8_from_uint_constexpr = unsigned_fit;
   int8 i8_from_uint_const = unsigned_const_fit;
   cat::verify(i1_from_int_constexpr == 0_i1);
   cat::verify(i1_from_int_const == 0_i1);
   cat::verify(i1_from_uint_constexpr == 0_i1);
   cat::verify(i1_from_uint_const == 0_i1);
   cat::verify(i2_from_int_constexpr == 0_i2);
   cat::verify(i2_from_int_const == 0_i2);
   cat::verify(i2_from_uint_constexpr == 0_i2);
   cat::verify(i2_from_uint_const == 0_i2);
   cat::verify(i4_from_int_constexpr == 0_i4);
   cat::verify(i4_from_int_const == 0_i4);
   cat::verify(i4_from_uint_constexpr == 0_i4);
   cat::verify(i4_from_uint_const == 0_i4);
   cat::verify(i8_from_int_constexpr == 0_i8);
   cat::verify(i8_from_int_const == 0_i8);
   cat::verify(i8_from_uint_constexpr == 0_i8);
   cat::verify(i8_from_uint_const == 0_i8);

   // `=`, `+=`, and `+` accept the same sources via the implicit `enable_if`
   // constructor that converts the operand to the wrapped type.
   int1 i1 = 1_i1;
   i1 = signed_fit;
   i1 = signed_const_fit;
   i1 = unsigned_fit;
   i1 = unsigned_const_fit;
   i1 += signed_fit;
   i1 += signed_const_fit;
   i1 += unsigned_fit;
   i1 += unsigned_const_fit;
   cat::verify(i1 == 0_i1);
   auto i1_sum_int_constexpr = i1 + signed_fit;
   auto i1_sum_int_const = i1 + signed_const_fit;
   auto i1_sum_uint_constexpr = i1 + unsigned_fit;
   auto i1_sum_uint_const = i1 + unsigned_const_fit;
   cat::verify(i1_sum_int_constexpr == 0_i1);
   cat::verify(i1_sum_int_const == 0_i1);
   cat::verify(i1_sum_uint_constexpr == 0_i1);
   cat::verify(i1_sum_uint_const == 0_i1);

   int2 i2 = 1_i2;
   i2 = signed_fit;
   i2 = signed_const_fit;
   i2 = unsigned_fit;
   i2 = unsigned_const_fit;
   i2 += signed_fit;
   i2 += signed_const_fit;
   i2 += unsigned_fit;
   i2 += unsigned_const_fit;
   cat::verify(i2 == 0_i2);
   auto i2_sum_int_constexpr = i2 + signed_fit;
   auto i2_sum_int_const = i2 + signed_const_fit;
   auto i2_sum_uint_constexpr = i2 + unsigned_fit;
   auto i2_sum_uint_const = i2 + unsigned_const_fit;
   cat::verify(i2_sum_int_constexpr == 0_i2);
   cat::verify(i2_sum_int_const == 0_i2);
   cat::verify(i2_sum_uint_constexpr == 0_i2);
   cat::verify(i2_sum_uint_const == 0_i2);

   int4 i4 = 1_i4;
   i4 = signed_fit;
   i4 = signed_const_fit;
   i4 = unsigned_fit;
   i4 = unsigned_const_fit;
   i4 += signed_fit;
   i4 += signed_const_fit;
   i4 += unsigned_fit;
   i4 += unsigned_const_fit;
   cat::verify(i4 == 0_i4);
   auto i4_sum_int_constexpr = i4 + signed_fit;
   auto i4_sum_int_const = i4 + signed_const_fit;
   auto i4_sum_uint_constexpr = i4 + unsigned_fit;
   auto i4_sum_uint_const = i4 + unsigned_const_fit;
   cat::verify(i4_sum_int_constexpr == 0_i4);
   cat::verify(i4_sum_int_const == 0_i4);
   cat::verify(i4_sum_uint_constexpr == 0_i4);
   cat::verify(i4_sum_uint_const == 0_i4);

   int8 i8 = 1_i8;
   i8 = signed_fit;
   i8 = signed_const_fit;
   i8 = unsigned_fit;
   i8 = unsigned_const_fit;
   i8 += signed_fit;
   i8 += signed_const_fit;
   i8 += unsigned_fit;
   i8 += unsigned_const_fit;
   cat::verify(i8 == 0_i8);
   auto i8_sum_int_constexpr = i8 + signed_fit;
   auto i8_sum_int_const = i8 + signed_const_fit;
   auto i8_sum_uint_constexpr = i8 + unsigned_fit;
   auto i8_sum_uint_const = i8 + unsigned_const_fit;
   cat::verify(i8_sum_int_constexpr == 0_i8);
   cat::verify(i8_sum_int_const == 0_i8);
   cat::verify(i8_sum_uint_constexpr == 0_i8);
   cat::verify(i8_sum_uint_const == 0_i8);
}

// Compile-time-known cross-signedness constant operands work for every
// arithmetic operator on the fixed-width signed wrappers (`int1` .. `int8`).
test(arithmetic_signed_fixed_width_all_operators_constexpr_const) {
   constexpr unsigned int unsigned_two = 2u;
   unsigned int const unsigned_const_two = 2u;
   constexpr unsigned long ulong_two = 2ul;
   unsigned long const ulong_const_two = 2ul;

   // `int4` (typical width). Compound and binary operators with unsigned
   // compile-time constants of various widths. Return types:
   //   * `int4 + unsigned int` (same order 4): LHS wins -> `int4`.
   //   * `int4 * unsigned long` (order 4 vs 8): `*` widens to wider raw
   //     `unsigned long` -> `uword`.
   //   * `int4 - / % unsigned long`: keeps LHS shape -> `int4`.
   int4 i4 = 12_i4;
   i4 += unsigned_two;
   i4 -= unsigned_const_two;
   i4 *= ulong_two;
   i4 /= ulong_const_two;
   i4 %= unsigned_const_two;
   cat::verify(i4 == 0_i4);

   int4 i4_six = 6_i4;
   cat::verify((i4_six + unsigned_two) == 8_i4);
   cat::verify((i4_six - unsigned_const_two) == 4_i4);
   cat::verify((i4_six * ulong_two) == 12_i4);
   cat::verify((i4_six / ulong_const_two) == 3_i4);
   cat::verify((i4_six % unsigned_const_two) == 0_i4);
   static_assert(cat::is_same<decltype(i4_six + unsigned_two), int4>);
   static_assert(cat::is_same<decltype(i4_six - unsigned_const_two), int4>);
   static_assert(cat::is_same<decltype(i4_six * ulong_two), uword>);
   static_assert(cat::is_same<decltype(i4_six / ulong_const_two), int4>);
   static_assert(cat::is_same<decltype(i4_six % unsigned_const_two), int4>);

   // `int1` (narrowest width). `+` and `*` widen to RHS's `unsigned int` (order
   // 4), but `-`, `/`, `%` keep the LHS's width.
   int1 i1 = 12_i1;
   i1 += unsigned_two;
   i1 -= unsigned_const_two;
   i1 *= unsigned_two;
   i1 /= unsigned_const_two;
   i1 %= unsigned_const_two;
   cat::verify(i1 == 0_i1);

   int1 i1_six = 6_i1;
   cat::verify((i1_six + unsigned_two) == 8_i1);
   cat::verify((i1_six - unsigned_const_two) == 4_i1);
   static_assert(cat::is_same<decltype(i1_six + unsigned_two), uint4>);
   static_assert(cat::is_same<decltype(i1_six - unsigned_const_two), int1>);
   static_assert(cat::is_same<decltype(i1_six * unsigned_two), uint4>);
   static_assert(cat::is_same<decltype(i1_six / unsigned_const_two), int1>);
   static_assert(cat::is_same<decltype(i1_six % unsigned_const_two), int1>);

   // `int8` (widest width). Mixing with same-width unsigned `long` keeps the
   // `int8` LHS storage. Smaller `unsigned int` similarly loses to `int8`.
   int8 i8 = 12_i8;
   i8 += unsigned_two;
   i8 -= unsigned_const_two;
   i8 *= ulong_two;
   i8 /= ulong_const_two;
   i8 %= unsigned_const_two;
   cat::verify(i8 == 0_i8);

   int8 i8_six = 6_i8;
   cat::verify((i8_six + unsigned_two) == 8_i8);
   cat::verify((i8_six * ulong_two) == 12_i8);
   static_assert(cat::is_same<decltype(i8_six + unsigned_two), int8>);
   static_assert(cat::is_same<decltype(i8_six - unsigned_const_two), int8>);
   static_assert(cat::is_same<decltype(i8_six * ulong_two), int8>);
   static_assert(cat::is_same<decltype(i8_six / ulong_const_two), int8>);
   static_assert(cat::is_same<decltype(i8_six % unsigned_const_two), int8>);
}

test(arithmetic_implicit_word_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   constexpr long long_fit = 0;
   long const long_const_fit = 0;
   constexpr unsigned long ulong_fit = 0ul;
   unsigned long const ulong_const_fit = 0ul;
   uword uw_from_int_constexpr = signed_fit;
   uword uw_from_int_const = signed_const_fit;
   uword uw_from_uint_constexpr = unsigned_fit;
   uword uw_from_uint_const = unsigned_const_fit;
   uword uw_from_long_constexpr = long_fit;
   uword uw_from_long_const = long_const_fit;
   uword uw_from_ulong_constexpr = ulong_fit;
   uword uw_from_ulong_const = ulong_const_fit;
   cat::verify(uw_from_int_constexpr == 0_uz);
   cat::verify(uw_from_int_const == 0_uz);
   cat::verify(uw_from_uint_constexpr == 0_uz);
   cat::verify(uw_from_uint_const == 0_uz);
   cat::verify(uw_from_long_constexpr == 0_uz);
   cat::verify(uw_from_long_const == 0_uz);
   cat::verify(uw_from_ulong_constexpr == 0_uz);
   cat::verify(uw_from_ulong_const == 0_uz);

   iword iw_from_int_constexpr = signed_fit;
   iword iw_from_int_const = signed_const_fit;
   iword iw_from_uint_constexpr = unsigned_fit;
   iword iw_from_uint_const = unsigned_const_fit;
   iword iw_from_long_constexpr = long_fit;
   iword iw_from_long_const = long_const_fit;
   iword iw_from_ulong_constexpr = ulong_fit;
   iword iw_from_ulong_const = ulong_const_fit;
   cat::verify(iw_from_int_constexpr == 0_sz);
   cat::verify(iw_from_int_const == 0_sz);
   cat::verify(iw_from_uint_constexpr == 0_sz);
   cat::verify(iw_from_uint_const == 0_sz);
   cat::verify(iw_from_long_constexpr == 0_sz);
   cat::verify(iw_from_long_const == 0_sz);
   cat::verify(iw_from_ulong_constexpr == 0_sz);
   cat::verify(iw_from_ulong_const == 0_sz);

   // `=`, `+=`, and `+` accept the same sources via the implicit `enable_if`
   // constructor that converts the operand to the wrapped type.
   uword uw = 1_uz;
   uw = signed_fit;
   uw = signed_const_fit;
   uw = unsigned_fit;
   uw = unsigned_const_fit;
   uw = long_fit;
   uw = long_const_fit;
   uw = ulong_fit;
   uw = ulong_const_fit;
   uw += signed_fit;
   uw += signed_const_fit;
   uw += unsigned_fit;
   uw += unsigned_const_fit;
   uw += long_fit;
   uw += long_const_fit;
   uw += ulong_fit;
   uw += ulong_const_fit;
   cat::verify(uw == 0_uz);
   auto uw_sum_int_constexpr = uw + signed_fit;
   auto uw_sum_int_const = uw + signed_const_fit;
   auto uw_sum_uint_constexpr = uw + unsigned_fit;
   auto uw_sum_uint_const = uw + unsigned_const_fit;
   auto uw_sum_long_constexpr = uw + long_fit;
   auto uw_sum_long_const = uw + long_const_fit;
   auto uw_sum_ulong_constexpr = uw + ulong_fit;
   auto uw_sum_ulong_const = uw + ulong_const_fit;
   cat::verify(uw_sum_int_constexpr == 0_uz);
   cat::verify(uw_sum_int_const == 0_uz);
   cat::verify(uw_sum_uint_constexpr == 0_uz);
   cat::verify(uw_sum_uint_const == 0_uz);
   cat::verify(uw_sum_long_constexpr == 0_uz);
   cat::verify(uw_sum_long_const == 0_uz);
   cat::verify(uw_sum_ulong_constexpr == 0_uz);
   cat::verify(uw_sum_ulong_const == 0_uz);

   iword iw = 1_sz;
   iw = signed_fit;
   iw = signed_const_fit;
   iw = unsigned_fit;
   iw = unsigned_const_fit;
   iw = long_fit;
   iw = long_const_fit;
   iw = ulong_fit;
   iw = ulong_const_fit;
   iw += signed_fit;
   iw += signed_const_fit;
   iw += unsigned_fit;
   iw += unsigned_const_fit;
   iw += long_fit;
   iw += long_const_fit;
   iw += ulong_fit;
   iw += ulong_const_fit;
   cat::verify(iw == 0_sz);
   auto iw_sum_int_constexpr = iw + signed_fit;
   auto iw_sum_int_const = iw + signed_const_fit;
   auto iw_sum_uint_constexpr = iw + unsigned_fit;
   auto iw_sum_uint_const = iw + unsigned_const_fit;
   auto iw_sum_long_constexpr = iw + long_fit;
   auto iw_sum_long_const = iw + long_const_fit;
   auto iw_sum_ulong_constexpr = iw + ulong_fit;
   auto iw_sum_ulong_const = iw + ulong_const_fit;
   cat::verify(iw_sum_int_constexpr == 0_sz);
   cat::verify(iw_sum_int_const == 0_sz);
   cat::verify(iw_sum_uint_constexpr == 0_sz);
   cat::verify(iw_sum_uint_const == 0_sz);
   cat::verify(iw_sum_long_constexpr == 0_sz);
   cat::verify(iw_sum_long_const == 0_sz);
   cat::verify(iw_sum_ulong_constexpr == 0_sz);
   cat::verify(iw_sum_ulong_const == 0_sz);
}

// Compile-time-known cross-signedness constant operands work for every
// arithmetic operator on `iword` / `uword`. Includes the `sizeof(T)` pattern
// from `set_memory_detail` that originally motivated relaxing `promoted_type`
// to allow same-order, opposite-signedness pairs.
test(arithmetic_word_all_operators_constexpr_const) {
   // Cross-sign compile-time constants used as operands.
   constexpr int signed_two = 2;
   int const signed_const_two = 2;
   constexpr unsigned int unsigned_two = 2u;
   unsigned int const unsigned_const_two = 2u;
   constexpr long long_two = 2;
   long const long_const_two = 2;
   constexpr unsigned long ulong_two = 2ul;
   unsigned long const ulong_const_two = 2ul;

   // Cross-sign constant compound assignment on `iword`.
   iword iw = 12_sz;
   iw += unsigned_two;
   iw -= unsigned_const_two;
   iw *= ulong_two;
   iw /= ulong_const_two;
   iw %= unsigned_const_two;
   cat::verify(iw == 0_sz);

   iw = 18_sz;
   iw -= ulong_two;
   iw += ulong_const_two;
   iw *= unsigned_const_two;
   iw /= unsigned_two;
   iw %= ulong_const_two;
   cat::verify(iw == 0_sz);

   // Cross-sign constant compound assignment on `uword`.
   uword uw = 12_uz;
   uw += signed_two;
   uw -= signed_const_two;
   uw *= long_two;
   uw /= long_const_two;
   uw %= signed_const_two;
   cat::verify(uw == 0_uz);

   uw = 18_uz;
   uw -= long_two;
   uw += long_const_two;
   uw *= signed_const_two;
   uw /= signed_two;
   uw %= long_const_two;
   cat::verify(uw == 0_uz);

   // Cross-sign constant binary operators on `iword` and their return types.
   // `promoted_type<iword, U>` keeps the LHS storage when the right operand has
   // the same promotion order (same width). `iword` wins same-order ties since
   // it is the LHS of `promoted_arithmetic`.
   iword iw_six = 6_sz;
   cat::verify((iw_six + unsigned_two) == 8_sz);
   cat::verify((iw_six - unsigned_const_two) == 4_sz);
   cat::verify((iw_six * ulong_two) == 12_sz);
   cat::verify((iw_six / ulong_const_two) == 3_sz);
   cat::verify((iw_six % unsigned_const_two) == 0_sz);
   static_assert(cat::is_same<decltype(iw_six + unsigned_two), iword>);
   static_assert(cat::is_same<decltype(iw_six - unsigned_const_two), iword>);
   static_assert(cat::is_same<decltype(iw_six * ulong_two), iword>);
   static_assert(cat::is_same<decltype(iw_six / ulong_const_two), iword>);
   static_assert(cat::is_same<decltype(iw_six % unsigned_const_two), iword>);

   // Cross-sign constant binary operators on `uword` and their return types.
   uword uw_six = 6_uz;
   cat::verify((uw_six + signed_two) == 8_uz);
   cat::verify((uw_six - signed_const_two) == 4_uz);
   cat::verify((uw_six * long_two) == 12_uz);
   cat::verify((uw_six / long_const_two) == 3_uz);
   cat::verify((uw_six % signed_const_two) == 0_uz);
   static_assert(cat::is_same<decltype(uw_six + signed_two), uword>);
   static_assert(cat::is_same<decltype(uw_six - signed_const_two), uword>);
   static_assert(cat::is_same<decltype(uw_six * long_two), uword>);
   static_assert(cat::is_same<decltype(uw_six / long_const_two), uword>);
   static_assert(cat::is_same<decltype(uw_six % signed_const_two), uword>);

   // The `sizeof(T)` motivating case from `set_memory_detail`. `sizeof` yields
   // `__SIZE_TYPE__` (an `unsigned long`), which the relaxed `promoted_type`
   // now accepts as the right-hand operand of an `iword` minus.
   iword bytes = 16_sz;
   bytes -= sizeof(int);
   cat::verify(bytes == 12_sz);
   bytes += sizeof(int);
   cat::verify(bytes == 16_sz);
   static_assert(cat::is_same<decltype(bytes - sizeof(int)), iword>);

   // Bitwise and shift on `uword` with cross-signedness constants
   // (smaller-width signed operand stays in `is_safe_arithmetic_comparison`).
   // `&` and `|` keep the wider of the two operand types via `promoted_type`.
   // `<<` and `>>` always keep the LHS storage type.
   uword uw_mask = 0b1100_uz;
   cat::verify((uw_mask & signed_two) == 0_uz);
   cat::verify((uw_mask | signed_two) == 0b1110_uz);
   cat::verify((uw_mask << signed_two) == 0b11'0000_uz);
   cat::verify((uw_mask >> signed_two) == 0b11_uz);
   static_assert(cat::is_same<decltype(uw_mask & signed_two), uword>);
   static_assert(cat::is_same<decltype(uw_mask | signed_two), uword>);
   static_assert(cat::is_same<decltype(uw_mask << signed_two), uword>);
   static_assert(cat::is_same<decltype(uw_mask >> signed_two), uword>);

   uw_mask = 0b1100_uz;
   uw_mask &= signed_const_two;
   cat::verify(uw_mask == 0_uz);
   uw_mask = 0b1100_uz;
   uw_mask |= signed_const_two;
   cat::verify(uw_mask == 0b1110_uz);
   uw_mask = 0b1100_uz;
   uw_mask <<= signed_const_two;
   cat::verify(uw_mask == 0b11'0000_uz);
   uw_mask = 0b1100_uz;
   uw_mask >>= signed_const_two;
   cat::verify(uw_mask == 0b11_uz);
}

// Same pattern as `arithmetic_implicit_idx_constexpr_const`, but for the
// pointer wrappers. `arithmetic_ptr` storage is always 8-byte
// (`__INTPTR_TYPE__` / `__UINTPTR_TYPE__`), so every fixed-width raw integer
// fits implicitly when its compile-time-known value is in range. `iword` /
// `uword` are also implicitly constructible at compile time when their value
// fits the storage. Same-signedness `iword` / `uword` (e.g. `iword` ->
// `intptr`) further stay implicit at runtime via the safe arithmetic conversion
// path. Cross-signedness operands stay `explicit` at runtime.
test(arithmetic_implicit_intptr_uintptr_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   constexpr long long_fit = 0;
   long const long_const_fit = 0;
   constexpr unsigned long ulong_fit = 0ul;
   unsigned long const ulong_const_fit = 0ul;

   uintptr<void> uintptr_from_int_constexpr = signed_fit;
   uintptr<void> uintptr_from_int_const = signed_const_fit;
   uintptr<void> uintptr_from_uint_constexpr = unsigned_fit;
   uintptr<void> uintptr_from_uint_const = unsigned_const_fit;
   uintptr<void> uintptr_from_long_constexpr = long_fit;
   uintptr<void> uintptr_from_long_const = long_const_fit;
   uintptr<void> uintptr_from_ulong_constexpr = ulong_fit;
   uintptr<void> uintptr_from_ulong_const = ulong_const_fit;
   cat::verify(uintptr_from_int_constexpr == 0u);
   cat::verify(uintptr_from_int_const == 0u);
   cat::verify(uintptr_from_uint_constexpr == 0u);
   cat::verify(uintptr_from_uint_const == 0u);
   cat::verify(uintptr_from_long_constexpr == 0u);
   cat::verify(uintptr_from_long_const == 0u);
   cat::verify(uintptr_from_ulong_constexpr == 0u);
   cat::verify(uintptr_from_ulong_const == 0u);

   intptr<void> intptr_from_int_constexpr = signed_fit;
   intptr<void> intptr_from_int_const = signed_const_fit;
   intptr<void> intptr_from_uint_constexpr = unsigned_fit;
   intptr<void> intptr_from_uint_const = unsigned_const_fit;
   intptr<void> intptr_from_long_constexpr = long_fit;
   intptr<void> intptr_from_long_const = long_const_fit;
   intptr<void> intptr_from_ulong_constexpr = ulong_fit;
   intptr<void> intptr_from_ulong_const = ulong_const_fit;
   cat::verify(intptr_from_int_constexpr == 0);
   cat::verify(intptr_from_int_const == 0);
   cat::verify(intptr_from_uint_constexpr == 0);
   cat::verify(intptr_from_uint_const == 0);
   cat::verify(intptr_from_long_constexpr == 0);
   cat::verify(intptr_from_long_const == 0);
   cat::verify(intptr_from_ulong_constexpr == 0);
   cat::verify(intptr_from_ulong_const == 0);

   // `iword` / `uword` whose value fits the pointer storage may also implicitly
   // construct an `intptr` / `uintptr`.
   constexpr iword iw_fit = 0_sz;
   iword const iw_const_fit = 0_sz;
   constexpr uword uw_fit = 0_uz;
   uword const uw_const_fit = 0_uz;
   uintptr<void> uintptr_from_iword_constexpr = iw_fit;
   uintptr<void> uintptr_from_iword_const = iw_const_fit;
   uintptr<void> uintptr_from_uword_constexpr = uw_fit;
   uintptr<void> uintptr_from_uword_const = uw_const_fit;
   intptr<void> intptr_from_iword_constexpr = iw_fit;
   intptr<void> intptr_from_iword_const = iw_const_fit;
   intptr<void> intptr_from_uword_constexpr = uw_fit;
   intptr<void> intptr_from_uword_const = uw_const_fit;
   cat::verify(uintptr_from_iword_constexpr == 0u);
   cat::verify(uintptr_from_iword_const == 0u);
   cat::verify(uintptr_from_uword_constexpr == 0u);
   cat::verify(uintptr_from_uword_const == 0u);
   cat::verify(intptr_from_iword_constexpr == 0);
   cat::verify(intptr_from_iword_const == 0);
   cat::verify(intptr_from_uword_constexpr == 0);
   cat::verify(intptr_from_uword_const == 0);

   // `=`, `+=`, and `+` accept the same sources via the implicit `enable_if`
   // constructor that converts the operand to the wrapped type.
   uintptr<void> up{0u};
   up = signed_fit;
   up = signed_const_fit;
   up = unsigned_fit;
   up = unsigned_const_fit;
   up = long_fit;
   up = long_const_fit;
   up = ulong_fit;
   up = ulong_const_fit;
   up = iw_fit;
   up = iw_const_fit;
   up = uw_fit;
   up = uw_const_fit;
   up += signed_fit;
   up += signed_const_fit;
   up += unsigned_fit;
   up += unsigned_const_fit;
   up += long_fit;
   up += long_const_fit;
   up += ulong_fit;
   up += ulong_const_fit;
   up += iw_fit;
   up += iw_const_fit;
   up += uw_fit;
   up += uw_const_fit;
   cat::verify(up == 0u);
   auto up_sum_int_constexpr = up + signed_fit;
   auto up_sum_int_const = up + signed_const_fit;
   auto up_sum_uint_constexpr = up + unsigned_fit;
   auto up_sum_uint_const = up + unsigned_const_fit;
   auto up_sum_long_constexpr = up + long_fit;
   auto up_sum_long_const = up + long_const_fit;
   auto up_sum_ulong_constexpr = up + ulong_fit;
   auto up_sum_ulong_const = up + ulong_const_fit;
   auto up_sum_iword_constexpr = up + iw_fit;
   auto up_sum_iword_const = up + iw_const_fit;
   auto up_sum_uword_constexpr = up + uw_fit;
   auto up_sum_uword_const = up + uw_const_fit;
   cat::verify(up_sum_int_constexpr == 0u);
   cat::verify(up_sum_int_const == 0u);
   cat::verify(up_sum_uint_constexpr == 0u);
   cat::verify(up_sum_uint_const == 0u);
   cat::verify(up_sum_long_constexpr == 0u);
   cat::verify(up_sum_long_const == 0u);
   cat::verify(up_sum_ulong_constexpr == 0u);
   cat::verify(up_sum_ulong_const == 0u);
   cat::verify(up_sum_iword_constexpr == 0u);
   cat::verify(up_sum_iword_const == 0u);
   cat::verify(up_sum_uword_constexpr == 0u);
   cat::verify(up_sum_uword_const == 0u);

   intptr<void> ip{0};
   ip = signed_fit;
   ip = signed_const_fit;
   ip = unsigned_fit;
   ip = unsigned_const_fit;
   ip = long_fit;
   ip = long_const_fit;
   ip = ulong_fit;
   ip = ulong_const_fit;
   ip = iw_fit;
   ip = iw_const_fit;
   ip = uw_fit;
   ip = uw_const_fit;
   ip += signed_fit;
   ip += signed_const_fit;
   ip += unsigned_fit;
   ip += unsigned_const_fit;
   ip += long_fit;
   ip += long_const_fit;
   ip += ulong_fit;
   ip += ulong_const_fit;
   ip += iw_fit;
   ip += iw_const_fit;
   ip += uw_fit;
   ip += uw_const_fit;
   cat::verify(ip == 0);
   auto ip_sum_int_constexpr = ip + signed_fit;
   auto ip_sum_int_const = ip + signed_const_fit;
   auto ip_sum_uint_constexpr = ip + unsigned_fit;
   auto ip_sum_uint_const = ip + unsigned_const_fit;
   auto ip_sum_long_constexpr = ip + long_fit;
   auto ip_sum_long_const = ip + long_const_fit;
   auto ip_sum_ulong_constexpr = ip + ulong_fit;
   auto ip_sum_ulong_const = ip + ulong_const_fit;
   auto ip_sum_iword_constexpr = ip + iw_fit;
   auto ip_sum_iword_const = ip + iw_const_fit;
   auto ip_sum_uword_constexpr = ip + uw_fit;
   auto ip_sum_uword_const = ip + uw_const_fit;
   cat::verify(ip_sum_int_constexpr == 0);
   cat::verify(ip_sum_int_const == 0);
   cat::verify(ip_sum_uint_constexpr == 0);
   cat::verify(ip_sum_uint_const == 0);
   cat::verify(ip_sum_long_constexpr == 0);
   cat::verify(ip_sum_long_const == 0);
   cat::verify(ip_sum_ulong_constexpr == 0);
   cat::verify(ip_sum_ulong_const == 0);
   cat::verify(ip_sum_iword_constexpr == 0);
   cat::verify(ip_sum_iword_const == 0);
   cat::verify(ip_sum_uword_constexpr == 0);
   cat::verify(ip_sum_uword_const == 0);

   // Runtime conversions follow the generic constructor's
   // `is_safe_arithmetic_conversion` predicate. For `intptr<void>` (signed long
   // storage) any same-sign or smaller-unsigned source is safe, so it stays
   // implicit. For `uintptr<void>` (unsigned long storage) only unsigned,
   // equal-or-narrower sources stay implicit. Sources that fail the safety
   // predicate fall to the `explicit` catch-all and only convert when their
   // compile-time value fits.
   static_assert(cat::is_convertible<int, intptr<void>>);
   static_assert(cat::is_convertible<unsigned int, intptr<void>>);
   static_assert(cat::is_convertible<long, intptr<void>>);
   static_assert(!cat::is_convertible<unsigned long, intptr<void>>);
   static_assert(cat::is_convertible<unsigned int, uintptr<void>>);
   static_assert(cat::is_convertible<unsigned long, uintptr<void>>);
   static_assert(!cat::is_convertible<int, uintptr<void>>);
   static_assert(!cat::is_convertible<long, uintptr<void>>);
   // `iword` / `uword` mirror `long` / `unsigned long`.
   static_assert(cat::is_convertible<iword, intptr<void>>);
   static_assert(!cat::is_convertible<uword, intptr<void>>);
   static_assert(cat::is_convertible<uword, uintptr<void>>);
   static_assert(!cat::is_convertible<iword, uintptr<void>>);
   // Direct-initialization (`explicit`) still works for every pairing.
   static_assert(cat::is_constructible<intptr<void>, int>);
   static_assert(cat::is_constructible<intptr<void>, unsigned int>);
   static_assert(cat::is_constructible<intptr<void>, long>);
   static_assert(cat::is_constructible<intptr<void>, unsigned long>);
   static_assert(cat::is_constructible<uintptr<void>, int>);
   static_assert(cat::is_constructible<uintptr<void>, unsigned int>);
   static_assert(cat::is_constructible<uintptr<void>, long>);
   static_assert(cat::is_constructible<uintptr<void>, unsigned long>);
   static_assert(cat::is_constructible<intptr<void>, iword>);
   static_assert(cat::is_constructible<intptr<void>, uword>);
   static_assert(cat::is_constructible<uintptr<void>, iword>);
   static_assert(cat::is_constructible<uintptr<void>, uword>);
}

// Compile-time-known cross-signedness constant operands work for the
// offset-style arithmetic operators on `intptr` / `uintptr` (`+=`, `-=`, `+`,
// `-`). Multiplicative and modulo operators are intentionally not tested here.
// `arithmetic_ptr` predates the relaxed `promoted_type` and would need a
// separate cleanup to participate.
test(arithmetic_intptr_uintptr_all_operators_constexpr_const) {
   constexpr int signed_two = 2;
   int const signed_const_two = 2;
   constexpr unsigned int unsigned_two = 2u;
   unsigned int const unsigned_const_two = 2u;
   constexpr long long_two = 2;
   long const long_const_two = 2;
   constexpr unsigned long ulong_two = 2ul;
   unsigned long const ulong_const_two = 2ul;

   // Cross-sign constant compound `+=` / `-=` on `intptr<void>`.
   intptr<void> ip(12);
   ip += unsigned_two;
   ip -= unsigned_const_two;
   ip += ulong_two;
   ip -= ulong_const_two;
   cat::verify(ip == 12);

   // Cross-sign constant compound `+=` / `-=` on `uintptr<void>`.
   uintptr<void> up(12u);
   up += signed_two;
   up -= signed_const_two;
   up += long_two;
   up -= long_const_two;
   cat::verify(up == 12u);

   // Cross-sign constant binary `+` / `-` on `intptr<void>`. `arithmetic_ptr`'s
   // `add` / `subtract_by` always return `arithmetic_ptr` (order 9 wins against
   // every other integer type), so the result keeps the LHS pointer type
   // regardless of operand width or signedness.
   intptr<void> ip_six(6);
   cat::verify((ip_six + unsigned_two) == 8);
   cat::verify((ip_six - unsigned_const_two) == 4);
   cat::verify((ip_six + ulong_two) == 8);
   cat::verify((ip_six - ulong_const_two) == 4);
   static_assert(cat::is_same<decltype(ip_six + unsigned_two), intptr<void>>);
   static_assert(
      cat::is_same<decltype(ip_six - unsigned_const_two), intptr<void>>);
   static_assert(cat::is_same<decltype(ip_six + ulong_two), intptr<void>>);
   static_assert(
      cat::is_same<decltype(ip_six - ulong_const_two), intptr<void>>);

   // Cross-sign constant binary `+` / `-` on `uintptr<void>`. Same reasoning as
   // above.
   uintptr<void> up_six(6u);
   cat::verify((up_six + signed_two) == 8u);
   cat::verify((up_six - signed_const_two) == 4u);
   cat::verify((up_six + long_two) == 8u);
   cat::verify((up_six - long_const_two) == 4u);
   static_assert(cat::is_same<decltype(up_six + signed_two), uintptr<void>>);
   static_assert(
      cat::is_same<decltype(up_six - signed_const_two), uintptr<void>>);
   static_assert(cat::is_same<decltype(up_six + long_two), uintptr<void>>);
   static_assert(
      cat::is_same<decltype(up_six - long_const_two), uintptr<void>>);
}

test(arithmetic_implicit_idx_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   constexpr long long_fit = 0;
   long const long_const_fit = 0;
   constexpr unsigned long ulong_fit = 0ul;
   unsigned long const ulong_const_fit = 0ul;
   idx idx_from_int_constexpr = signed_fit;
   idx idx_from_int_const = signed_const_fit;
   idx idx_from_uint_constexpr = unsigned_fit;
   idx idx_from_uint_const = unsigned_const_fit;
   idx idx_from_long_constexpr = long_fit;
   idx idx_from_long_const = long_const_fit;
   idx idx_from_ulong_constexpr = ulong_fit;
   idx idx_from_ulong_const = ulong_const_fit;
   cat::verify(idx_from_int_constexpr == 0_idx);
   cat::verify(idx_from_int_const == 0_idx);
   cat::verify(idx_from_uint_constexpr == 0_idx);
   cat::verify(idx_from_uint_const == 0_idx);
   cat::verify(idx_from_long_constexpr == 0_idx);
   cat::verify(idx_from_long_const == 0_idx);
   cat::verify(idx_from_ulong_constexpr == 0_idx);
   cat::verify(idx_from_ulong_const == 0_idx);

   // `iword` / `uword` whose value fits the `index` storage may also implicitly
   // construct an `idx`. Runtime `iword` / `uword` operands stay `explicit`
   // (validated by the failing-by-default `requires` checks below).
   constexpr iword iw_fit = 0_sz;
   iword const iw_const_fit = 0_sz;
   constexpr uword uw_fit = 0_uz;
   uword const uw_const_fit = 0_uz;
   idx idx_from_iword_constexpr = iw_fit;
   idx idx_from_iword_const = iw_const_fit;
   idx idx_from_uword_constexpr = uw_fit;
   idx idx_from_uword_const = uw_const_fit;
   cat::verify(idx_from_iword_constexpr == 0_idx);
   cat::verify(idx_from_iword_const == 0_idx);
   cat::verify(idx_from_uword_constexpr == 0_idx);
   cat::verify(idx_from_uword_const == 0_idx);

   // `=`, `+=`, and `+` accept the same sources via the implicit `enable_if`
   // constructor that converts the operand to the wrapped type.
   idx ix = 1_idx;
   ix = signed_fit;
   ix = signed_const_fit;
   ix = unsigned_fit;
   ix = unsigned_const_fit;
   ix = long_fit;
   ix = long_const_fit;
   ix = ulong_fit;
   ix = ulong_const_fit;
   ix = iw_fit;
   ix = iw_const_fit;
   ix = uw_fit;
   ix = uw_const_fit;
   ix += signed_fit;
   ix += signed_const_fit;
   ix += unsigned_fit;
   ix += unsigned_const_fit;
   ix += long_fit;
   ix += long_const_fit;
   ix += ulong_fit;
   ix += ulong_const_fit;
   ix += iw_fit;
   ix += iw_const_fit;
   ix += uw_fit;
   ix += uw_const_fit;
   cat::verify(ix == 0_idx);
   auto ix_sum_int_constexpr = ix + signed_fit;
   auto ix_sum_int_const = ix + signed_const_fit;
   auto ix_sum_uint_constexpr = ix + unsigned_fit;
   auto ix_sum_uint_const = ix + unsigned_const_fit;
   auto ix_sum_long_constexpr = ix + long_fit;
   auto ix_sum_long_const = ix + long_const_fit;
   auto ix_sum_ulong_constexpr = ix + ulong_fit;
   auto ix_sum_ulong_const = ix + ulong_const_fit;
   auto ix_sum_iword_constexpr = ix + iw_fit;
   auto ix_sum_iword_const = ix + iw_const_fit;
   auto ix_sum_uword_constexpr = ix + uw_fit;
   auto ix_sum_uword_const = ix + uw_const_fit;
   cat::verify(ix_sum_int_constexpr == 0_idx);
   cat::verify(ix_sum_int_const == 0_idx);
   cat::verify(ix_sum_uint_constexpr == 0_idx);
   cat::verify(ix_sum_uint_const == 0_idx);
   cat::verify(ix_sum_long_constexpr == 0_idx);
   cat::verify(ix_sum_long_const == 0_idx);
   cat::verify(ix_sum_ulong_constexpr == 0_idx);
   cat::verify(ix_sum_ulong_const == 0_idx);
   cat::verify(ix_sum_iword_constexpr == 0_idx);
   cat::verify(ix_sum_iword_const == 0_idx);
   cat::verify(ix_sum_uword_constexpr == 0_idx);
   cat::verify(ix_sum_uword_const == 0_idx);

   // Runtime `iword` / `uword` operands must still require an `explicit`
   // construction. Only compile-time-known values that fit may convert
   // implicitly. `is_convertible` checks the implicit path.
   static_assert(!cat::is_convertible<iword, idx>);
   static_assert(!cat::is_convertible<uword, idx>);
   // Direct-initialization (`explicit`) still works for both.
   static_assert(cat::is_constructible<idx, iword>);
   static_assert(cat::is_constructible<idx, uword>);
}

// `idx` provides `+`, `-`, `*`, `/`, `%` (no bitwise). `-=` and `--` are
// deleted because subtraction can underflow an `index`. Subtraction yields the
// signed distance (`iword`) instead. `/=` only works with an unsigned right
// operand (signed division returns `iword`). Cross-signedness compile-time
// constants flow through the supported operators via the implicit `enable_if`
// constructor.
test(arithmetic_idx_all_operators_constexpr_const) {
   constexpr int signed_two = 2;
   int const signed_const_two = 2;
   constexpr unsigned int unsigned_two = 2u;
   unsigned int const unsigned_const_two = 2u;
   constexpr long long_two = 2;
   long const long_const_two = 2;
   constexpr unsigned long ulong_two = 2ul;
   unsigned long const ulong_const_two = 2ul;

   // Compound `+=` accepts every cross-signedness compile-time constant.
   idx ix = 12_idx;
   ix += signed_two;
   ix += signed_const_two;
   ix += unsigned_two;
   ix += unsigned_const_two;
   ix += long_two;
   ix += long_const_two;
   ix += ulong_two;
   ix += ulong_const_two;
   cat::verify(ix == 28_idx);

   // Compound `*=` and `%=` on unsigned compile-time constants. Mixed-sign
   // inputs would still resolve since `multiply` / `modulo_by` return `idx`.
   ix = 6_idx;
   ix *= signed_two;
   cat::verify(ix == 12_idx);
   ix *= unsigned_const_two;
   cat::verify(ix == 24_idx);
   ix %= unsigned_const_two;
   cat::verify(ix == 0_idx);

   // Compound `/=` on an unsigned operand keeps `idx` storage. The signed
   // overload is correctly absent (it would return `iword`).
   ix = 24_idx;
   ix /= unsigned_two;
   cat::verify(ix == 12_idx);
   ix /= ulong_const_two;
   cat::verify(ix == 6_idx);

   // Binary `+` return types follow the `index::add` overloads:
   //   * unsigned operand: `promoted_type<index, U>`. `idx` (order 7) wins
   //     against `unsigned int` (order 4) but loses to `unsigned long`
   //     (order 8), which promotes to `uword`.
   //   * signed operand: always `iword` (signed distance) since the result may
   //     underflow `index`.
   ix = 12_idx;
   auto sum_int_constexpr = ix + signed_two;
   auto sum_int_const = ix + signed_const_two;
   auto sum_uint_constexpr = ix + unsigned_two;
   auto sum_uint_const = ix + unsigned_const_two;
   auto sum_long_constexpr = ix + long_two;
   auto sum_long_const = ix + long_const_two;
   auto sum_ulong_constexpr = ix + ulong_two;
   auto sum_ulong_const = ix + ulong_const_two;
   cat::verify(sum_int_constexpr == 14_idx);
   cat::verify(sum_int_const == 14_idx);
   cat::verify(sum_uint_constexpr == 14_idx);
   cat::verify(sum_uint_const == 14_idx);
   cat::verify(sum_long_constexpr == 14_idx);
   cat::verify(sum_long_const == 14_idx);
   cat::verify(sum_ulong_constexpr == 14_idx);
   cat::verify(sum_ulong_const == 14_idx);
   static_assert(cat::is_same<decltype(ix + signed_two), iword>);
   static_assert(cat::is_same<decltype(ix + signed_const_two), iword>);
   static_assert(cat::is_same<decltype(ix + unsigned_two), idx>);
   static_assert(cat::is_same<decltype(ix + unsigned_const_two), idx>);
   static_assert(cat::is_same<decltype(ix + long_two), iword>);
   static_assert(cat::is_same<decltype(ix + long_const_two), iword>);
   static_assert(cat::is_same<decltype(ix + ulong_two), uword>);
   static_assert(cat::is_same<decltype(ix + ulong_const_two), uword>);

   // Binary `-` yields `iword` (the signed distance), regardless of operand
   // signedness, because `index` cannot represent a negative result.
   auto diff_int_constexpr = ix - signed_two;
   auto diff_uint_const = ix - unsigned_const_two;
   auto diff_ulong_const = ix - ulong_const_two;
   cat::verify(diff_int_constexpr == 10_sz);
   cat::verify(diff_uint_const == 10_sz);
   cat::verify(diff_ulong_const == 10_sz);
   static_assert(cat::is_same<decltype(ix - signed_two), iword>);
   static_assert(cat::is_same<decltype(ix - unsigned_const_two), iword>);
   static_assert(cat::is_same<decltype(ix - ulong_const_two), iword>);

   // Binary `*` and `%` always return `idx` (the `index::multiply` /
   // `modulo_by` overloads cast through wide integers and rebuild an index).
   // Binary `/` with an unsigned operand also stays in `idx`, while a signed
   // operand yields `iword` since a negative divisor would underflow `index`.
   auto product_signed_const = ix * signed_const_two;
   auto quotient_ulong_const = ix / ulong_const_two;
   auto remainder_signed_const = ix % signed_const_two;
   cat::verify(product_signed_const == 24_idx);
   cat::verify(quotient_ulong_const == 6_idx);
   cat::verify(remainder_signed_const == 0_idx);
   static_assert(cat::is_same<decltype(ix * signed_const_two), idx>);
   static_assert(cat::is_same<decltype(ix / ulong_const_two), idx>);
   static_assert(cat::is_same<decltype(ix % signed_const_two), idx>);

   auto signed_quotient_constexpr = ix / signed_two;
   auto signed_quotient_const = ix / long_const_two;
   cat::verify(signed_quotient_constexpr == 6_sz);
   cat::verify(signed_quotient_const == 6_sz);
   static_assert(cat::is_same<decltype(ix / signed_two), iword>);
   static_assert(cat::is_same<decltype(ix / long_const_two), iword>);
}

// The same conversions as
// `arithmetic_implicit_unsigned_fixed_width_constexpr_const` except the
// destination has `.sat()` applied. Assignment, compound `+=`, and binary `+`
// all accept the cross-signedness constant operand when its compile-time value
// fits the wrapped storage.
test(arithmetic_implicit_unsigned_fixed_width_sat_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;

   uint1 u1 = 1_u1;
   u1.sat() = signed_fit;
   cat::verify(u1 == 0_u1);
   u1.sat() = signed_const_fit;
   u1.sat() = unsigned_fit;
   u1.sat() = unsigned_const_fit;
   u1.sat() += signed_fit;
   u1.sat() += signed_const_fit;
   u1.sat() += unsigned_fit;
   u1.sat() += unsigned_const_fit;
   cat::verify(u1 == 0_u1);
   auto u1_sum_int_constexpr = u1.sat() + signed_fit;
   auto u1_sum_int_const = u1.sat() + signed_const_fit;
   auto u1_sum_uint_constexpr = u1.sat() + unsigned_fit;
   auto u1_sum_uint_const = u1.sat() + unsigned_const_fit;
   cat::verify(u1_sum_int_constexpr == 0_u1);
   cat::verify(u1_sum_int_const == 0_u1);
   cat::verify(u1_sum_uint_constexpr == 0_u1);
   cat::verify(u1_sum_uint_const == 0_u1);

   uint2 u2 = 1_u2;
   u2.sat() = signed_fit;
   u2.sat() = signed_const_fit;
   u2.sat() = unsigned_fit;
   u2.sat() = unsigned_const_fit;
   u2.sat() += signed_fit;
   u2.sat() += signed_const_fit;
   u2.sat() += unsigned_fit;
   u2.sat() += unsigned_const_fit;
   cat::verify(u2 == 0_u2);
   auto u2_sum_int_constexpr = u2.sat() + signed_fit;
   auto u2_sum_int_const = u2.sat() + signed_const_fit;
   auto u2_sum_uint_constexpr = u2.sat() + unsigned_fit;
   auto u2_sum_uint_const = u2.sat() + unsigned_const_fit;
   cat::verify(u2_sum_int_constexpr == 0_u2);
   cat::verify(u2_sum_int_const == 0_u2);
   cat::verify(u2_sum_uint_constexpr == 0_u2);
   cat::verify(u2_sum_uint_const == 0_u2);

   uint4 u4 = 1_u4;
   u4.sat() = signed_fit;
   u4.sat() = signed_const_fit;
   u4.sat() = unsigned_fit;
   u4.sat() = unsigned_const_fit;
   u4.sat() += signed_fit;
   u4.sat() += signed_const_fit;
   u4.sat() += unsigned_fit;
   u4.sat() += unsigned_const_fit;
   cat::verify(u4 == 0_u4);
   auto u4_sum_int_constexpr = u4.sat() + signed_fit;
   auto u4_sum_int_const = u4.sat() + signed_const_fit;
   auto u4_sum_uint_constexpr = u4.sat() + unsigned_fit;
   auto u4_sum_uint_const = u4.sat() + unsigned_const_fit;
   cat::verify(u4_sum_int_constexpr == 0_u4);
   cat::verify(u4_sum_int_const == 0_u4);
   cat::verify(u4_sum_uint_constexpr == 0_u4);
   cat::verify(u4_sum_uint_const == 0_u4);

   uint8 u8 = 1_u8;
   u8.sat() = signed_fit;
   u8.sat() = signed_const_fit;
   u8.sat() = unsigned_fit;
   u8.sat() = unsigned_const_fit;
   u8.sat() += signed_fit;
   u8.sat() += signed_const_fit;
   u8.sat() += unsigned_fit;
   u8.sat() += unsigned_const_fit;
   cat::verify(u8 == 0_u8);
   auto u8_sum_int_constexpr = u8.sat() + signed_fit;
   auto u8_sum_int_const = u8.sat() + signed_const_fit;
   auto u8_sum_uint_constexpr = u8.sat() + unsigned_fit;
   auto u8_sum_uint_const = u8.sat() + unsigned_const_fit;
   cat::verify(u8_sum_int_constexpr == 0_u8);
   cat::verify(u8_sum_int_const == 0_u8);
   cat::verify(u8_sum_uint_constexpr == 0_u8);
   cat::verify(u8_sum_uint_const == 0_u8);
}

// The same conversions as
// `arithmetic_implicit_signed_fixed_width_constexpr_const` except the
// destination has `.sat()` applied.
test(arithmetic_implicit_signed_fixed_width_sat_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;

   int1 i1 = 1_i1;
   i1.sat() = signed_fit;
   i1.sat() = signed_const_fit;
   i1.sat() = unsigned_fit;
   i1.sat() = unsigned_const_fit;
   i1.sat() += signed_fit;
   i1.sat() += signed_const_fit;
   i1.sat() += unsigned_fit;
   i1.sat() += unsigned_const_fit;
   cat::verify(i1 == 0_i1);
   auto i1_sum_int_constexpr = i1.sat() + signed_fit;
   auto i1_sum_int_const = i1.sat() + signed_const_fit;
   auto i1_sum_uint_constexpr = i1.sat() + unsigned_fit;
   auto i1_sum_uint_const = i1.sat() + unsigned_const_fit;
   cat::verify(i1_sum_int_constexpr == 0_i1);
   cat::verify(i1_sum_int_const == 0_i1);
   cat::verify(i1_sum_uint_constexpr == 0_i1);
   cat::verify(i1_sum_uint_const == 0_i1);

   int2 i2 = 1_i2;
   i2.sat() = signed_fit;
   i2.sat() = signed_const_fit;
   i2.sat() = unsigned_fit;
   i2.sat() = unsigned_const_fit;
   i2.sat() += signed_fit;
   i2.sat() += signed_const_fit;
   i2.sat() += unsigned_fit;
   i2.sat() += unsigned_const_fit;
   cat::verify(i2 == 0_i2);
   auto i2_sum_int_constexpr = i2.sat() + signed_fit;
   auto i2_sum_int_const = i2.sat() + signed_const_fit;
   auto i2_sum_uint_constexpr = i2.sat() + unsigned_fit;
   auto i2_sum_uint_const = i2.sat() + unsigned_const_fit;
   cat::verify(i2_sum_int_constexpr == 0_i2);
   cat::verify(i2_sum_int_const == 0_i2);
   cat::verify(i2_sum_uint_constexpr == 0_i2);
   cat::verify(i2_sum_uint_const == 0_i2);

   int4 i4 = 1_i4;
   i4.sat() = signed_fit;
   i4.sat() = signed_const_fit;
   i4.sat() = unsigned_fit;
   i4.sat() = unsigned_const_fit;
   i4.sat() += signed_fit;
   i4.sat() += signed_const_fit;
   i4.sat() += unsigned_fit;
   i4.sat() += unsigned_const_fit;
   cat::verify(i4 == 0_i4);
   auto i4_sum_int_constexpr = i4.sat() + signed_fit;
   auto i4_sum_int_const = i4.sat() + signed_const_fit;
   auto i4_sum_uint_constexpr = i4.sat() + unsigned_fit;
   auto i4_sum_uint_const = i4.sat() + unsigned_const_fit;
   cat::verify(i4_sum_int_constexpr == 0_i4);
   cat::verify(i4_sum_int_const == 0_i4);
   cat::verify(i4_sum_uint_constexpr == 0_i4);
   cat::verify(i4_sum_uint_const == 0_i4);

   int8 i8 = 1_i8;
   i8.sat() = signed_fit;
   i8.sat() = signed_const_fit;
   i8.sat() = unsigned_fit;
   i8.sat() = unsigned_const_fit;
   i8.sat() += signed_fit;
   i8.sat() += signed_const_fit;
   i8.sat() += unsigned_fit;
   i8.sat() += unsigned_const_fit;
   cat::verify(i8 == 0_i8);
   auto i8_sum_int_constexpr = i8.sat() + signed_fit;
   auto i8_sum_int_const = i8.sat() + signed_const_fit;
   auto i8_sum_uint_constexpr = i8.sat() + unsigned_fit;
   auto i8_sum_uint_const = i8.sat() + unsigned_const_fit;
   cat::verify(i8_sum_int_constexpr == 0_i8);
   cat::verify(i8_sum_int_const == 0_i8);
   cat::verify(i8_sum_uint_constexpr == 0_i8);
   cat::verify(i8_sum_uint_const == 0_i8);
}

// The same conversions as `arithmetic_implicit_word_constexpr_const` except the
// destination has `.sat()` applied.
test(arithmetic_implicit_word_sat_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   constexpr long long_fit = 0;
   long const long_const_fit = 0;
   constexpr unsigned long ulong_fit = 0ul;
   unsigned long const ulong_const_fit = 0ul;

   uword uw = 1_uz;
   uw.sat() = signed_fit;
   uw.sat() = signed_const_fit;
   uw.sat() = unsigned_fit;
   uw.sat() = unsigned_const_fit;
   uw.sat() = long_fit;
   uw.sat() = long_const_fit;
   uw.sat() = ulong_fit;
   uw.sat() = ulong_const_fit;
   uw.sat() += signed_fit;
   uw.sat() += signed_const_fit;
   uw.sat() += unsigned_fit;
   uw.sat() += unsigned_const_fit;
   uw.sat() += long_fit;
   uw.sat() += long_const_fit;
   uw.sat() += ulong_fit;
   uw.sat() += ulong_const_fit;
   cat::verify(uw == 0_uz);
   auto uw_sum_int_constexpr = uw.sat() + signed_fit;
   auto uw_sum_int_const = uw.sat() + signed_const_fit;
   auto uw_sum_uint_constexpr = uw.sat() + unsigned_fit;
   auto uw_sum_uint_const = uw.sat() + unsigned_const_fit;
   auto uw_sum_long_constexpr = uw.sat() + long_fit;
   auto uw_sum_long_const = uw.sat() + long_const_fit;
   auto uw_sum_ulong_constexpr = uw.sat() + ulong_fit;
   auto uw_sum_ulong_const = uw.sat() + ulong_const_fit;
   cat::verify(uw_sum_int_constexpr == 0_uz);
   cat::verify(uw_sum_int_const == 0_uz);
   cat::verify(uw_sum_uint_constexpr == 0_uz);
   cat::verify(uw_sum_uint_const == 0_uz);
   cat::verify(uw_sum_long_constexpr == 0_uz);
   cat::verify(uw_sum_long_const == 0_uz);
   cat::verify(uw_sum_ulong_constexpr == 0_uz);
   cat::verify(uw_sum_ulong_const == 0_uz);

   iword iw = 1_sz;
   iw.sat() = signed_fit;
   iw.sat() = signed_const_fit;
   iw.sat() = unsigned_fit;
   iw.sat() = unsigned_const_fit;
   iw.sat() = long_fit;
   iw.sat() = long_const_fit;
   iw.sat() = ulong_fit;
   iw.sat() = ulong_const_fit;
   iw.sat() += signed_fit;
   iw.sat() += signed_const_fit;
   iw.sat() += unsigned_fit;
   iw.sat() += unsigned_const_fit;
   iw.sat() += long_fit;
   iw.sat() += long_const_fit;
   iw.sat() += ulong_fit;
   iw.sat() += ulong_const_fit;
   cat::verify(iw == 0_sz);
   auto iw_sum_int_constexpr = iw.sat() + signed_fit;
   auto iw_sum_int_const = iw.sat() + signed_const_fit;
   auto iw_sum_uint_constexpr = iw.sat() + unsigned_fit;
   auto iw_sum_uint_const = iw.sat() + unsigned_const_fit;
   auto iw_sum_long_constexpr = iw.sat() + long_fit;
   auto iw_sum_long_const = iw.sat() + long_const_fit;
   auto iw_sum_ulong_constexpr = iw.sat() + ulong_fit;
   auto iw_sum_ulong_const = iw.sat() + ulong_const_fit;
   cat::verify(iw_sum_int_constexpr == 0_sz);
   cat::verify(iw_sum_int_const == 0_sz);
   cat::verify(iw_sum_uint_constexpr == 0_sz);
   cat::verify(iw_sum_uint_const == 0_sz);
   cat::verify(iw_sum_long_constexpr == 0_sz);
   cat::verify(iw_sum_long_const == 0_sz);
   cat::verify(iw_sum_ulong_constexpr == 0_sz);
   cat::verify(iw_sum_ulong_const == 0_sz);
}

// The same conversions as `arithmetic_implicit_idx_constexpr_const` except the
// destination has `.sat()` applied.
test(arithmetic_implicit_idx_sat_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   constexpr long long_fit = 0;
   long const long_const_fit = 0;
   constexpr unsigned long ulong_fit = 0ul;
   unsigned long const ulong_const_fit = 0ul;

   idx ix = 1_idx;
   ix.sat() = signed_fit;
   ix.sat() = signed_const_fit;
   ix.sat() = unsigned_fit;
   ix.sat() = unsigned_const_fit;
   ix.sat() = long_fit;
   ix.sat() = long_const_fit;
   ix.sat() = ulong_fit;
   ix.sat() = ulong_const_fit;
   ix.sat() += signed_fit;
   ix.sat() += signed_const_fit;
   ix.sat() += unsigned_fit;
   ix.sat() += unsigned_const_fit;
   ix.sat() += long_fit;
   ix.sat() += long_const_fit;
   ix.sat() += ulong_fit;
   ix.sat() += ulong_const_fit;
   cat::verify(ix == 0_idx);
   auto ix_sum_int_constexpr = ix.sat() + signed_fit;
   auto ix_sum_int_const = ix.sat() + signed_const_fit;
   auto ix_sum_uint_constexpr = ix.sat() + unsigned_fit;
   auto ix_sum_uint_const = ix.sat() + unsigned_const_fit;
   auto ix_sum_long_constexpr = ix.sat() + long_fit;
   auto ix_sum_long_const = ix.sat() + long_const_fit;
   auto ix_sum_ulong_constexpr = ix.sat() + ulong_fit;
   auto ix_sum_ulong_const = ix.sat() + ulong_const_fit;
   cat::verify(ix_sum_int_constexpr == 0_idx);
   cat::verify(ix_sum_int_const == 0_idx);
   cat::verify(ix_sum_uint_constexpr == 0_idx);
   cat::verify(ix_sum_uint_const == 0_idx);
   cat::verify(ix_sum_long_constexpr == 0_idx);
   cat::verify(ix_sum_long_const == 0_idx);
   cat::verify(ix_sum_ulong_constexpr == 0_idx);
   cat::verify(ix_sum_ulong_const == 0_idx);
}

// The same conversions as
// `arithmetic_implicit_unsigned_fixed_width_constexpr_const` except the
// destination has `.wrap()` applied.
test(arithmetic_implicit_unsigned_fixed_width_wrap_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;

   uint1 u1 = 1_u1;
   u1.wrap() = signed_fit;
   cat::verify(u1 == 0_u1);
   u1.wrap() = signed_const_fit;
   u1.wrap() = unsigned_fit;
   u1.wrap() = unsigned_const_fit;
   u1.wrap() += signed_fit;
   u1.wrap() += signed_const_fit;
   u1.wrap() += unsigned_fit;
   u1.wrap() += unsigned_const_fit;
   cat::verify(u1 == 0_u1);
   auto u1_sum_int_constexpr = u1.wrap() + signed_fit;
   auto u1_sum_int_const = u1.wrap() + signed_const_fit;
   auto u1_sum_uint_constexpr = u1.wrap() + unsigned_fit;
   auto u1_sum_uint_const = u1.wrap() + unsigned_const_fit;
   cat::verify(u1_sum_int_constexpr == 0_u1);
   cat::verify(u1_sum_int_const == 0_u1);
   cat::verify(u1_sum_uint_constexpr == 0_u1);
   cat::verify(u1_sum_uint_const == 0_u1);

   uint2 u2 = 1_u2;
   u2.wrap() = signed_fit;
   u2.wrap() = signed_const_fit;
   u2.wrap() = unsigned_fit;
   u2.wrap() = unsigned_const_fit;
   u2.wrap() += signed_fit;
   u2.wrap() += signed_const_fit;
   u2.wrap() += unsigned_fit;
   u2.wrap() += unsigned_const_fit;
   cat::verify(u2 == 0_u2);
   auto u2_sum_int_constexpr = u2.wrap() + signed_fit;
   auto u2_sum_int_const = u2.wrap() + signed_const_fit;
   auto u2_sum_uint_constexpr = u2.wrap() + unsigned_fit;
   auto u2_sum_uint_const = u2.wrap() + unsigned_const_fit;
   cat::verify(u2_sum_int_constexpr == 0_u2);
   cat::verify(u2_sum_int_const == 0_u2);
   cat::verify(u2_sum_uint_constexpr == 0_u2);
   cat::verify(u2_sum_uint_const == 0_u2);

   uint4 u4 = 1_u4;
   u4.wrap() = signed_fit;
   u4.wrap() = signed_const_fit;
   u4.wrap() = unsigned_fit;
   u4.wrap() = unsigned_const_fit;
   u4.wrap() += signed_fit;
   u4.wrap() += signed_const_fit;
   u4.wrap() += unsigned_fit;
   u4.wrap() += unsigned_const_fit;
   cat::verify(u4 == 0_u4);
   auto u4_sum_int_constexpr = u4.wrap() + signed_fit;
   auto u4_sum_int_const = u4.wrap() + signed_const_fit;
   auto u4_sum_uint_constexpr = u4.wrap() + unsigned_fit;
   auto u4_sum_uint_const = u4.wrap() + unsigned_const_fit;
   cat::verify(u4_sum_int_constexpr == 0_u4);
   cat::verify(u4_sum_int_const == 0_u4);
   cat::verify(u4_sum_uint_constexpr == 0_u4);
   cat::verify(u4_sum_uint_const == 0_u4);

   uint8 u8 = 1_u8;
   u8.wrap() = signed_fit;
   u8.wrap() = signed_const_fit;
   u8.wrap() = unsigned_fit;
   u8.wrap() = unsigned_const_fit;
   u8.wrap() += signed_fit;
   u8.wrap() += signed_const_fit;
   u8.wrap() += unsigned_fit;
   u8.wrap() += unsigned_const_fit;
   cat::verify(u8 == 0_u8);
   auto u8_sum_int_constexpr = u8.wrap() + signed_fit;
   auto u8_sum_int_const = u8.wrap() + signed_const_fit;
   auto u8_sum_uint_constexpr = u8.wrap() + unsigned_fit;
   auto u8_sum_uint_const = u8.wrap() + unsigned_const_fit;
   cat::verify(u8_sum_int_constexpr == 0_u8);
   cat::verify(u8_sum_int_const == 0_u8);
   cat::verify(u8_sum_uint_constexpr == 0_u8);
   cat::verify(u8_sum_uint_const == 0_u8);
}

// The same conversions as
// `arithmetic_implicit_signed_fixed_width_constexpr_const` except the
// destination has `.wrap()` applied.
test(arithmetic_implicit_signed_fixed_width_wrap_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;

   int1 i1 = 1_i1;
   i1.wrap() = signed_fit;
   i1.wrap() = signed_const_fit;
   i1.wrap() = unsigned_fit;
   i1.wrap() = unsigned_const_fit;
   i1.wrap() += signed_fit;
   i1.wrap() += signed_const_fit;
   i1.wrap() += unsigned_fit;
   i1.wrap() += unsigned_const_fit;
   cat::verify(i1 == 0_i1);
   auto i1_sum_int_constexpr = i1.wrap() + signed_fit;
   auto i1_sum_int_const = i1.wrap() + signed_const_fit;
   auto i1_sum_uint_constexpr = i1.wrap() + unsigned_fit;
   auto i1_sum_uint_const = i1.wrap() + unsigned_const_fit;
   cat::verify(i1_sum_int_constexpr == 0_i1);
   cat::verify(i1_sum_int_const == 0_i1);
   cat::verify(i1_sum_uint_constexpr == 0_i1);
   cat::verify(i1_sum_uint_const == 0_i1);

   int2 i2 = 1_i2;
   i2.wrap() = signed_fit;
   i2.wrap() = signed_const_fit;
   i2.wrap() = unsigned_fit;
   i2.wrap() = unsigned_const_fit;
   i2.wrap() += signed_fit;
   i2.wrap() += signed_const_fit;
   i2.wrap() += unsigned_fit;
   i2.wrap() += unsigned_const_fit;
   cat::verify(i2 == 0_i2);
   auto i2_sum_int_constexpr = i2.wrap() + signed_fit;
   auto i2_sum_int_const = i2.wrap() + signed_const_fit;
   auto i2_sum_uint_constexpr = i2.wrap() + unsigned_fit;
   auto i2_sum_uint_const = i2.wrap() + unsigned_const_fit;
   cat::verify(i2_sum_int_constexpr == 0_i2);
   cat::verify(i2_sum_int_const == 0_i2);
   cat::verify(i2_sum_uint_constexpr == 0_i2);
   cat::verify(i2_sum_uint_const == 0_i2);

   int4 i4 = 1_i4;
   i4.wrap() = signed_fit;
   i4.wrap() = signed_const_fit;
   i4.wrap() = unsigned_fit;
   i4.wrap() = unsigned_const_fit;
   i4.wrap() += signed_fit;
   i4.wrap() += signed_const_fit;
   i4.wrap() += unsigned_fit;
   i4.wrap() += unsigned_const_fit;
   cat::verify(i4 == 0_i4);
   auto i4_sum_int_constexpr = i4.wrap() + signed_fit;
   auto i4_sum_int_const = i4.wrap() + signed_const_fit;
   auto i4_sum_uint_constexpr = i4.wrap() + unsigned_fit;
   auto i4_sum_uint_const = i4.wrap() + unsigned_const_fit;
   cat::verify(i4_sum_int_constexpr == 0_i4);
   cat::verify(i4_sum_int_const == 0_i4);
   cat::verify(i4_sum_uint_constexpr == 0_i4);
   cat::verify(i4_sum_uint_const == 0_i4);

   int8 i8 = 1_i8;
   i8.wrap() = signed_fit;
   i8.wrap() = signed_const_fit;
   i8.wrap() = unsigned_fit;
   i8.wrap() = unsigned_const_fit;
   i8.wrap() += signed_fit;
   i8.wrap() += signed_const_fit;
   i8.wrap() += unsigned_fit;
   i8.wrap() += unsigned_const_fit;
   cat::verify(i8 == 0_i8);
   auto i8_sum_int_constexpr = i8.wrap() + signed_fit;
   auto i8_sum_int_const = i8.wrap() + signed_const_fit;
   auto i8_sum_uint_constexpr = i8.wrap() + unsigned_fit;
   auto i8_sum_uint_const = i8.wrap() + unsigned_const_fit;
   cat::verify(i8_sum_int_constexpr == 0_i8);
   cat::verify(i8_sum_int_const == 0_i8);
   cat::verify(i8_sum_uint_constexpr == 0_i8);
   cat::verify(i8_sum_uint_const == 0_i8);
}

// The same conversions as `arithmetic_implicit_word_constexpr_const` except the
// destination has `.wrap()` applied.
test(arithmetic_implicit_word_wrap_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   constexpr long long_fit = 0;
   long const long_const_fit = 0;
   constexpr unsigned long ulong_fit = 0ul;
   unsigned long const ulong_const_fit = 0ul;

   uword uw = 1_uz;
   uw.wrap() = signed_fit;
   uw.wrap() = signed_const_fit;
   uw.wrap() = unsigned_fit;
   uw.wrap() = unsigned_const_fit;
   uw.wrap() = long_fit;
   uw.wrap() = long_const_fit;
   uw.wrap() = ulong_fit;
   uw.wrap() = ulong_const_fit;
   uw.wrap() += signed_fit;
   uw.wrap() += signed_const_fit;
   uw.wrap() += unsigned_fit;
   uw.wrap() += unsigned_const_fit;
   uw.wrap() += long_fit;
   uw.wrap() += long_const_fit;
   uw.wrap() += ulong_fit;
   uw.wrap() += ulong_const_fit;
   cat::verify(uw == 0_uz);
   auto uw_sum_int_constexpr = uw.wrap() + signed_fit;
   auto uw_sum_int_const = uw.wrap() + signed_const_fit;
   auto uw_sum_uint_constexpr = uw.wrap() + unsigned_fit;
   auto uw_sum_uint_const = uw.wrap() + unsigned_const_fit;
   auto uw_sum_long_constexpr = uw.wrap() + long_fit;
   auto uw_sum_long_const = uw.wrap() + long_const_fit;
   auto uw_sum_ulong_constexpr = uw.wrap() + ulong_fit;
   auto uw_sum_ulong_const = uw.wrap() + ulong_const_fit;
   cat::verify(uw_sum_int_constexpr == 0_uz);
   cat::verify(uw_sum_int_const == 0_uz);
   cat::verify(uw_sum_uint_constexpr == 0_uz);
   cat::verify(uw_sum_uint_const == 0_uz);
   cat::verify(uw_sum_long_constexpr == 0_uz);
   cat::verify(uw_sum_long_const == 0_uz);
   cat::verify(uw_sum_ulong_constexpr == 0_uz);
   cat::verify(uw_sum_ulong_const == 0_uz);

   iword iw = 1_sz;
   iw.wrap() = signed_fit;
   iw.wrap() = signed_const_fit;
   iw.wrap() = unsigned_fit;
   iw.wrap() = unsigned_const_fit;
   iw.wrap() = long_fit;
   iw.wrap() = long_const_fit;
   iw.wrap() = ulong_fit;
   iw.wrap() = ulong_const_fit;
   iw.wrap() += signed_fit;
   iw.wrap() += signed_const_fit;
   iw.wrap() += unsigned_fit;
   iw.wrap() += unsigned_const_fit;
   iw.wrap() += long_fit;
   iw.wrap() += long_const_fit;
   iw.wrap() += ulong_fit;
   iw.wrap() += ulong_const_fit;
   cat::verify(iw == 0_sz);
   auto iw_sum_int_constexpr = iw.wrap() + signed_fit;
   auto iw_sum_int_const = iw.wrap() + signed_const_fit;
   auto iw_sum_uint_constexpr = iw.wrap() + unsigned_fit;
   auto iw_sum_uint_const = iw.wrap() + unsigned_const_fit;
   auto iw_sum_long_constexpr = iw.wrap() + long_fit;
   auto iw_sum_long_const = iw.wrap() + long_const_fit;
   auto iw_sum_ulong_constexpr = iw.wrap() + ulong_fit;
   auto iw_sum_ulong_const = iw.wrap() + ulong_const_fit;
   cat::verify(iw_sum_int_constexpr == 0_sz);
   cat::verify(iw_sum_int_const == 0_sz);
   cat::verify(iw_sum_uint_constexpr == 0_sz);
   cat::verify(iw_sum_uint_const == 0_sz);
   cat::verify(iw_sum_long_constexpr == 0_sz);
   cat::verify(iw_sum_long_const == 0_sz);
   cat::verify(iw_sum_ulong_constexpr == 0_sz);
   cat::verify(iw_sum_ulong_const == 0_sz);
}

// The same conversions as `arithmetic_implicit_idx_constexpr_const` except the
// destination has `.wrap()` applied.
test(arithmetic_implicit_idx_wrap_constexpr_const) {
   constexpr int signed_fit = 0;
   int const signed_const_fit = 0;
   constexpr unsigned int unsigned_fit = 0u;
   unsigned int const unsigned_const_fit = 0u;
   constexpr long long_fit = 0;
   long const long_const_fit = 0;
   constexpr unsigned long ulong_fit = 0ul;
   unsigned long const ulong_const_fit = 0ul;

   idx ix = 1_idx;
   ix.wrap() = signed_fit;
   ix.wrap() = signed_const_fit;
   ix.wrap() = unsigned_fit;
   ix.wrap() = unsigned_const_fit;
   ix.wrap() = long_fit;
   ix.wrap() = long_const_fit;
   ix.wrap() = ulong_fit;
   ix.wrap() = ulong_const_fit;
   ix.wrap() += signed_fit;
   ix.wrap() += signed_const_fit;
   ix.wrap() += unsigned_fit;
   ix.wrap() += unsigned_const_fit;
   ix.wrap() += long_fit;
   ix.wrap() += long_const_fit;
   ix.wrap() += ulong_fit;
   ix.wrap() += ulong_const_fit;
   cat::verify(ix == 0_idx);
   auto ix_sum_int_constexpr = ix.wrap() + signed_fit;
   auto ix_sum_int_const = ix.wrap() + signed_const_fit;
   auto ix_sum_uint_constexpr = ix.wrap() + unsigned_fit;
   auto ix_sum_uint_const = ix.wrap() + unsigned_const_fit;
   auto ix_sum_long_constexpr = ix.wrap() + long_fit;
   auto ix_sum_long_const = ix.wrap() + long_const_fit;
   auto ix_sum_ulong_constexpr = ix.wrap() + ulong_fit;
   auto ix_sum_ulong_const = ix.wrap() + ulong_const_fit;
   cat::verify(ix_sum_int_constexpr == 0_idx);
   cat::verify(ix_sum_int_const == 0_idx);
   cat::verify(ix_sum_uint_constexpr == 0_idx);
   cat::verify(ix_sum_uint_const == 0_idx);
   cat::verify(ix_sum_long_constexpr == 0_idx);
   cat::verify(ix_sum_long_const == 0_idx);
   cat::verify(ix_sum_ulong_constexpr == 0_idx);
   cat::verify(ix_sum_ulong_const == 0_idx);
}

// `cat::overflow_reference` is a public type, like `cat::bit_reference`. It is
// normally obtained from `.undef()`, `.wrap()`, and `.sat()` accessors, but it
// can also be constructed and named directly. This test treats it as a
// first-class public API: naming the type, constructing it, storing it, passing
// it to a function, and reading/writing through it.
test(arithmetic_overflow_reference_public_type) {
   // The accessors return `cat::overflow_reference` instances. Mutable lvalues
   // produce a non-const reference to the wrapped type. Const lvalues produce a
   // const reference.
   uint4 u4 = 5_u4;
   uint4 const u4_const = 5_u4;
   static_assert(cat::is_same<
                 decltype(u4.wrap()),
                 cat::overflow_reference<uint4, cat::overflow_policies::wrap>>);
   static_assert(
      cat::is_same<
         decltype(u4.sat()),
         cat::overflow_reference<uint4, cat::overflow_policies::saturate>>);
   static_assert(
      cat::is_same<
         decltype(u4.undef()),
         cat::overflow_reference<uint4, cat::overflow_policies::undefined>>);
   static_assert(
      cat::is_same<
         decltype(u4_const.wrap()),
         cat::overflow_reference<uint4 const, cat::overflow_policies::wrap>>);

   int4 i4 = 5_i4;
   static_assert(cat::is_same<
                 decltype(i4.wrap()),
                 cat::overflow_reference<int4, cat::overflow_policies::wrap>>);

   idx ix = 5_idx;
   static_assert(
      cat::is_same<decltype(ix.wrap()),
                   cat::overflow_reference<idx, cat::overflow_policies::wrap>>);

   uintptr<void> uptr = nullptr;
   static_assert(
      cat::is_same<
         decltype(uptr.wrap()),
         cat::overflow_reference<uintptr<void>, cat::overflow_policies::wrap>>);

   // Direct construction by naming the type. Wrapped value is `u4`, behaves
   // exactly like `u4.wrap()` would.
   cat::overflow_reference<uint4, cat::overflow_policies::wrap> u4_wrap_ref(u4);
   u4 = cat::uint4_max;
   u4_wrap_ref += 1u;
   cat::verify(u4 == 0_u4);

   // The same wrapped value can be observed through a different policy.
   cat::overflow_reference<uint4, cat::overflow_policies::saturate> u4_sat_ref(
      u4);
   u4 = cat::uint4_max;
   u4_sat_ref += 1u;
   cat::verify(u4 == cat::uint4_max);

   // Saturate on signed overflow.
   int4 i4_storage = cat::int4_max;
   cat::overflow_reference<int4, cat::overflow_policies::saturate> i4_sat_ref(
      i4_storage);
   i4_sat_ref += 100;
   cat::verify(i4_storage == cat::int4_max);

   // Wrap on signed overflow.
   i4_storage = cat::int4_max;
   cat::overflow_reference<int4, cat::overflow_policies::wrap> i4_wrap_ref(
      i4_storage);
   i4_wrap_ref += 1;
   cat::verify(i4_storage == cat::int4_min);

   // `index` and `arithmetic_ptr` work the same way.
   idx ix_storage = 5_idx;
   cat::overflow_reference<idx, cat::overflow_policies::wrap> ix_wrap_ref(
      ix_storage);
   ix_wrap_ref += 10u;
   cat::verify(ix_storage == 15_idx);

   uintptr<void> uptr_storage{0xdeadul};
   cat::overflow_reference<uintptr<void>, cat::overflow_policies::wrap>
      uptr_wrap_ref(uptr_storage);
   uptr_wrap_ref += 1u;
   cat::verify(uptr_storage == 0xdeaeul);

   // A const wrapped value yields a read-only proxy. Reads work. Writes are not
   // part of the public surface for const wrappers.
   uint4 const u4_read_only = 42_u4;
   cat::overflow_reference<uint4 const, cat::overflow_policies::wrap>
      u4_const_ref(u4_read_only);
   cat::verify(u4_const_ref == 42_u4);
   cat::verify(static_cast<uint4>(u4_const_ref) == 42_u4);

   // `cat::overflow_reference` can be passed across function boundaries just
   // like `cat::bit_reference`.
   constexpr auto bump =
      [](cat::overflow_reference<uint4, cat::overflow_policies::wrap> ref) {
         ref += 1u;
      };
   uint4 bumped = cat::uint4_max;
   bump(bumped.wrap());
   cat::verify(bumped == 0_u4);

   // Round-tripping through the proxy preserves the value.
   uint4 round_trip = 7_u4;
   cat::overflow_reference<uint4, cat::overflow_policies::saturate>
      round_trip_ref(round_trip);
   round_trip_ref = 9_u4;
   cat::verify(round_trip == 9_u4);
   cat::verify(static_cast<uint4>(round_trip_ref) == 9_u4);
}

test(arithmetic_intptr_constructors_conversions_and_dereference) {
   // Test `intptr` constructors and assignment.
   intptr<void> intptr_1 = nullptr;
   intptr<void> intptr_2 = nullptr;
   intptr_1 = intptr_1 + intptr_2;

   // Test `arithmetic_ptr` conversions.
   uintptr<void> uintptr_1 = static_cast<uintptr<void>>(intptr_1);
   [[maybe_unused]]
   uintptr<void>::raw_type raw_uintptr =
      static_cast<uintptr<void>::raw_type>(uintptr_1);

   [[maybe_unused]]
   intptr<void> intptr_3 = static_cast<intptr<void>>(uintptr_1);

   // Test `arithmetic_ptr` dereferencing operators.
   int4 integer = 0;
   intptr<int4> int_intptr = &integer;
   *int_intptr = 1;
   cat::verify(integer == 1);

   uint4 uinteger = 0u;
   intptr<uint4> uint_intptr = &uinteger;
   *uint_intptr = 1u;
   cat::verify(uinteger == 1u);

   cat::verify(int_intptr->min() == integer.min());
   cat::verify(int_intptr->max() == integer.max());

   cat::verify(uint_intptr->min() == uinteger.min());
   cat::verify(uint_intptr->max() == uinteger.max());
}

test(arithmetic_uintptr_increment_and_decrement) {
   // Test `intptr` increment and decrement operators.
   uintptr<void> uincptr{0xb07a70e5};
   cat::verify(++uincptr == 0xb07a70e6);
   cat::verify(uincptr++ == 0xb07a70e7);
   cat::verify(--uincptr == 0xb07a70e6);
   cat::verify(uincptr-- == 0xb07a70e5);
}

test(arithmetic_spaceship_three_way_and_inequality) {
   // Test `<=>`.
   int4 int_less = 0;
   int4 int_more = 2;

   [[maybe_unused]]
   bool is_less = (int_less < int_more);
   cat::verify(is_less);
   is_less = ((0 <=> int_more) < 0);  // NOLINT
   cat::verify(is_less);
   is_less = (0 < int_more);
   cat::verify(is_less);
   is_less = (int_less < 2);
   cat::verify(is_less);

   [[maybe_unused]]
   bool is_more = (int_more > int_less);
   is_more = ((0 <=> int_less) == 0);  // NOLINT
   cat::verify(is_more);
   is_more = (0 < int_more);
   cat::verify(is_more);
   is_more = (int_less < 2);
   cat::verify(is_more);

   // Test generated `!=`.
   cat::verify(4_i4 == 4_i4);
   cat::verify(4_i4 != 2_i4);

   cat::verify(4_u4 == 4_u4);
   cat::verify(4_u4 != 2_u4);

   cat::verify(4_f4 == 4_f4);
   cat::verify(4_f4 != 2_f4);
}

test(arithmetic_cross_sign_comparisons) {
   // `arithmetic` cross-signedness at runtime (the case used in `set_memory`
   // for `iword >= sizeof(...)` where the right operand is `unsigned long`).
   iword runtime_signed = 16;
   unsigned long runtime_unsigned = 8ul;
   cat::verify(runtime_signed >= runtime_unsigned);
   cat::verify(runtime_signed > runtime_unsigned);
   cat::verify(!(runtime_signed < runtime_unsigned));
   cat::verify(runtime_signed != runtime_unsigned);

   // Negative signed left, unsigned right: must compare strictly less.
   iword negative_signed = -1;
   cat::verify(negative_signed < runtime_unsigned);
   cat::verify(!(negative_signed == runtime_unsigned));
   cat::verify(negative_signed != runtime_unsigned);

   // Unsigned left, negative signed right: must compare strictly greater.
   uword runtime_uword = 5u;
   long negative_long = -1;
   cat::verify(runtime_uword > negative_long);
   cat::verify(!(runtime_uword == negative_long));

   // Mixed widths and mixed signedness, both directions.
   int4 small_signed = 2;
   uint8 wide_unsigned = 3ul;
   cat::verify(small_signed < wide_unsigned);
   cat::verify(small_signed != wide_unsigned);
   uint4 small_unsigned = 4u;
   int8 wide_signed = -1ll;
   cat::verify(small_unsigned > wide_signed);
   cat::verify(small_unsigned != wide_signed);

   // `arithmetic` vs another wrapped `arithmetic` of opposite signedness.
   iword iw = 100;
   uword uw = 100u;
   cat::verify(iw == uw);
   cat::verify(!(iw < uw));
   cat::verify(!(iw > uw));

   // `index` against negative signed values: previously the unsigned cast
   // through `common_type` made these compare as `unsigned long` and silently
   // returned the wrong answer.
   idx idx_zero = 0u;
   idx idx_one = 1u;
   cat::verify(idx_zero > -1);
   cat::verify(idx_zero != -1);
   cat::verify(idx_one > -1);
   cat::verify(idx_one > -100);

   // `index` runtime comparisons against raw integer types (the `set_memory`
   // pattern again, this time with an `index` wrapper).
   idx runtime_idx = 16u;
   cat::verify(runtime_idx >= sizeof(unsigned long));
   cat::verify(runtime_idx > 8u);
   cat::verify(runtime_idx == 16ul);

   // `arithmetic_ptr` cross-signedness.
   intptr<void> signed_ptr{16};
   cat::verify(signed_ptr >= 8ul);
   cat::verify(signed_ptr > 0ul);
   cat::verify(signed_ptr != 0ul);
   uintptr<void> unsigned_ptr{16ul};
   cat::verify(unsigned_ptr > -1);
   cat::verify(unsigned_ptr != -1);

   // `overflow_reference` (via `.sat()`/`.wrap()`) cross-signedness at runtime.
   // Both `<=>` and `==` must route through the helpers.
   iword sat_signed = 16;
   uword sat_unsigned = 8u;
   cat::verify(sat_signed.sat() >= sat_unsigned);
   cat::verify(sat_signed.sat() != sat_unsigned);
   cat::verify(sat_signed.wrap() >= sat_unsigned);
   cat::verify(sat_unsigned.sat() > -1);
   cat::verify(sat_unsigned.wrap() != -1);

   // `overflow_reference` wrapping `arithmetic_ptr` against unsigned literals.
   intptr<void> ref_ptr{16};
   cat::verify(ref_ptr.sat() >= sizeof(unsigned long));
   cat::verify(ref_ptr.wrap() != 0ul);
}

test(arithmetic_match_on_type_and_value) {
   // Test matching numerals.
   int4 match_int = 1;
   bool matched = false;

   // Match type.
   cat::match(match_int)(  //
      is_a<uint4>().then_do([]() {
         cat::exit(1);
      }),
      is_a<int4>().then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);

   // Match value.
   matched = false;
   cat::match(match_int)(  //
      is_a(0).then_do([]() {
         cat::exit(1);
      }),
      is_a(1).then_do([&]() {
         matched = true;
      }));
   cat::verify(matched);
}

test(arithmetic_unary_operators) {
   // Test unary operators.
   [[maybe_unused]]
   int4 negative_int4 = -1_i4;
   [[maybe_unused]]
   float4 negative_float4 = -1_f4;
   [[maybe_unused]]
   int4 positive_int4 = +1_i4;
   [[maybe_unused]]
   float4 positive_float4 = +1_f4;
   // `operator~` is only generated for unsigned integral `arithmetic<T>`.
   [[maybe_unused]]
   uint4 negated_uint4 = ~1_u4;
}

test(arithmetic_nttp_numerals) {
   // Test using numerals non-type template parameters.
   [[maybe_unused]]
   nttp<1_i1> nttp_int1{};
   [[maybe_unused]]
   nttp<1_u1> nttp_uint1{};
   [[maybe_unused]]
   nttp<1_i2> nttp_int2{};
   [[maybe_unused]]
   nttp<1_u2> nttp_uint2{};
   [[maybe_unused]]
   nttp<1_i4> nttp_int4{};
   [[maybe_unused]]
   nttp<1_u4> nttp_uint4{};
   [[maybe_unused]]
   nttp<1_i8> nttp_int8{};
   [[maybe_unused]]
   nttp<1_u8> nttp_uint8{};
   [[maybe_unused]]
   nttp<1_f4> nttp_float4{};
   [[maybe_unused]]
   nttp<1_f8> nttp_float8{};
}

test(arithmetic_sat_add_unsigned_fixed_width) {
   // Test unsigned saturating addition.
   static_assert(cat::sat_add(cat::uint1_max - 3u, 1_u1) < cat::uint1_max);
   static_assert(cat::sat_add(cat::uint1_max - 1u, 1_u1) == cat::uint1_max);
   static_assert(cat::sat_add(cat::uint1_max - 1_u1, 1_u1) == cat::uint1_max);
   static_assert(cat::sat_add(cat::uint1_max.raw, 1_u1) == cat::uint1_max);
   static_assert(cat::sat_add(cat::uint1_max, 100_u1) == cat::uint1_max);

   cat::verify(cat::sat_add(cat::uint1_max - 3u, 1_u1) < cat::uint1_max);
   cat::verify(cat::sat_add(cat::uint1_max - 1u, 1_u1) == cat::uint1_max);
   cat::verify(cat::sat_add(cat::uint1_max - 1u, 1u) == cat::uint1_max);
   cat::verify(cat::sat_add(cat::uint1_max.raw, 1_u1) == cat::uint1_max);
   cat::verify(cat::sat_add(cat::uint1_max, 100_u1) == cat::uint1_max);

   static_assert(cat::sat_add(cat::uint2_max - 3u, 2_u2) < cat::uint2_max);
   static_assert(cat::sat_add(cat::uint2_max - 2u, 2_u2) == cat::uint2_max);
   static_assert(cat::sat_add(cat::uint2_max - 1_u2, 2_u2) == cat::uint2_max);
   static_assert(cat::sat_add(cat::uint2_max.raw, 2_u2) == cat::uint2_max);
   static_assert(cat::sat_add(cat::uint2_max, 100_u2) == cat::uint2_max);

   cat::verify(cat::sat_add(cat::uint2_max - 3u, 2_u2) < cat::uint2_max);
   cat::verify(cat::sat_add(cat::uint2_max - 2u, 2_u2) == cat::uint2_max);
   cat::verify(cat::sat_add(cat::uint2_max - 1_u2, 2_u2) == cat::uint2_max);
   cat::verify(cat::sat_add(cat::uint2_max.raw, 2_u2) == cat::uint2_max);
   cat::verify(cat::sat_add(cat::uint2_max, 100_u2) == cat::uint2_max);

   static_assert(cat::sat_add(cat::uint4_max - 3u, 2_u4) < cat::uint4_max);
   static_assert(cat::sat_add(cat::uint4_max - 2u, 2_u4) == cat::uint4_max);
   static_assert(cat::sat_add(cat::uint4_max - 1u, 2u) == cat::uint4_max);
   static_assert(cat::sat_add(cat::uint4_max.raw, 2_u4) == cat::uint4_max);
   static_assert(cat::sat_add(cat::uint4_max, 100u) == cat::uint4_max);

   cat::verify(cat::sat_add(cat::uint4_max - 3u, 2_u4) < cat::uint4_max);
   cat::verify(cat::sat_add(cat::uint4_max - 2u, 2_u4) == cat::uint4_max);
   cat::verify(cat::sat_add(cat::uint4_max - 1u, 2u) == cat::uint4_max);
   cat::verify(cat::sat_add(cat::uint4_max.raw, 2_u4) == cat::uint4_max);
   cat::verify(cat::sat_add(cat::uint4_max, 100u) == cat::uint4_max);

   static_assert(cat::sat_add(cat::uint8_max - 3u, 2_u8) < cat::uint8_max);
   static_assert(cat::sat_add(cat::uint8_max - 2u, 2_u8) == cat::uint8_max);
   static_assert(cat::sat_add(cat::uint8_max - 1u, 2u) == cat::uint8_max);
   static_assert(cat::sat_add(cat::uint8_max.raw, 2_u8) == cat::uint8_max);
   static_assert(cat::sat_add(cat::uint8_max, 100u) == cat::uint8_max);

   cat::verify(cat::sat_add(cat::uint8_max - 3u, 2_u8) < cat::uint8_max);
   cat::verify(cat::sat_add(cat::uint8_max - 2u, 2_u8) == cat::uint8_max);
   cat::verify(cat::sat_add(cat::uint8_max - 1u, 2u) == cat::uint8_max);
   cat::verify(cat::sat_add(cat::uint8_max.raw, 2_u8) == cat::uint8_max);
   cat::verify(cat::sat_add(cat::uint8_max, 100u) == cat::uint8_max);
}

test(arithmetic_sat_add_signed_fixed_width) {
   // Test signed saturating addition.
   static_assert(cat::sat_add(cat::int1_max - 3_i1, 1_i1) < cat::int1_max);
   static_assert(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
   static_assert(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
   static_assert(cat::sat_add(cat::int1_max.raw, 1_i1) == cat::int1_max);
   static_assert(cat::sat_add(cat::int1_max, 100_i1) == cat::int1_max);
   cat::verify(cat::sat_add(cat::int1_max - 3, 1_i1) < cat::int1_max);
   cat::verify(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
   cat::verify(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
   cat::verify(cat::sat_add(cat::int1_max.raw, 1_i1) == cat::int1_max);
   cat::verify(cat::sat_add(cat::int1_max, 100_i1) == cat::int1_max);

   static_assert(cat::sat_add(cat::int2_max - 3_i2, 2_i2) < cat::int2_max);
   static_assert(cat::sat_add(cat::int2_max - 2_i2, 2_i2) == cat::int2_max);
   static_assert(cat::sat_add(cat::int2_max - 1_i2, 2_i2) == cat::int2_max);
   static_assert(cat::sat_add(cat::int2_max.raw, 2_i2) == cat::int2_max);
   static_assert(cat::sat_add(cat::int2_max, 100_i2) == cat::int2_max);

   cat::verify(cat::sat_add(cat::int2_max - 3_i2, 2_i2) < cat::int2_max);
   cat::verify(cat::sat_add(cat::int2_max - 2_i2, 2_i2) == cat::int2_max);
   cat::verify(cat::sat_add(cat::int2_max - 1_i2, 2_i2) == cat::int2_max);
   cat::verify(cat::sat_add(cat::int2_max.raw, 2_i2) == cat::int2_max);
   cat::verify(cat::sat_add(cat::int2_max, 100_i2) == cat::int2_max);

   static_assert(cat::sat_add(cat::int4_max - 3, 2_i4) < cat::int4_max);
   static_assert(cat::sat_add(cat::int4_max - 2, 2_i4) == cat::int4_max);
   static_assert(cat::sat_add(cat::int4_max - 1, 2) == cat::int4_max);
   static_assert(cat::sat_add(cat::int4_max.raw, 2_i4) == cat::int4_max);
   static_assert(cat::sat_add(cat::int4_max, 100) == cat::int4_max);

   cat::verify(cat::sat_add(cat::int4_max - 3, 2_i4) < cat::int4_max);
   cat::verify(cat::sat_add(cat::int4_max - 2, 2_i4) == cat::int4_max);
   cat::verify(cat::sat_add(cat::int4_max - 1, 2) == cat::int4_max);
   cat::verify(cat::sat_add(cat::int4_max.raw, 2_i4) == cat::int4_max);
   cat::verify(cat::sat_add(cat::int4_max, 100) == cat::int4_max);

   static_assert(cat::sat_add(cat::int8_max - 3, 2_i8) < cat::int8_max);
   static_assert(cat::sat_add(cat::int8_max - 2, 2_i8) == cat::int8_max);
   static_assert(cat::sat_add(cat::int8_max - 1, 2) == cat::int8_max);
   static_assert(cat::sat_add(cat::int8_max.raw, 2_i8) == cat::int8_max);
   static_assert(cat::sat_add(cat::int8_max, 100) == cat::int8_max);

   cat::verify(cat::sat_add(cat::int8_max - 3, 2_i8) < cat::int8_max);
   cat::verify(cat::sat_add(cat::int8_max - 2, 2_i8) == cat::int8_max);
   cat::verify(cat::sat_add(cat::int8_max - 1, 2) == cat::int8_max);
   cat::verify(cat::sat_add(cat::int8_max.raw, 2_i8) == cat::int8_max);
   cat::verify(cat::sat_add(cat::int8_max, 100) == cat::int8_max);
}

test(arithmetic_sat_sub_unsigned_fixed_width) {
   // Test unsigned saturating subtraction.
   static_assert(cat::sat_sub(cat::uint1_min + 3u, 1_u1) > cat::uint1_min);
   static_assert(cat::sat_sub(cat::uint1_min + 1u, 1_u1) == cat::uint1_min);
   static_assert(cat::sat_sub(cat::uint1_min + 1_u1, 1_u1) == cat::uint1_min);
   static_assert(cat::sat_sub(cat::uint1_min.raw, 1_u1) == cat::uint1_min);
   static_assert(cat::sat_sub(cat::uint1_min, 100_u1) == cat::uint1_min);

   cat::verify(cat::sat_sub(cat::uint1_min + 3u, 1_u1) > cat::uint1_min);
   cat::verify(cat::sat_sub(cat::uint1_min + 1u, 1_u1) == cat::uint1_min);
   cat::verify(cat::sat_sub(cat::uint1_min + 1u, 1u) == cat::uint1_min);
   cat::verify(cat::sat_sub(cat::uint1_min.raw, 1_u1) == cat::uint1_min);
   cat::verify(cat::sat_sub(cat::uint1_min, 100_u1) == cat::uint1_min);

   static_assert(cat::sat_sub(cat::uint2_min + 3u, 2_u2) > cat::uint2_min);
   static_assert(cat::sat_sub(cat::uint2_min + 2u, 2_u2) == cat::uint2_min);
   static_assert(cat::sat_sub(cat::uint2_min + 1_u2, 2_u2) == cat::uint2_min);
   static_assert(cat::sat_sub(cat::uint2_min.raw, 2_u2) == cat::uint2_min);
   static_assert(cat::sat_sub(cat::uint2_min, 100_u2) == cat::uint2_min);

   cat::verify(cat::sat_sub(cat::uint2_min + 3u, 2_u2) > cat::uint2_min);
   cat::verify(cat::sat_sub(cat::uint2_min + 2u, 2_u2) == cat::uint2_min);
   cat::verify(cat::sat_sub(cat::uint2_min + 1_u2, 2_u2) == cat::uint2_min);
   cat::verify(cat::sat_sub(cat::uint2_min.raw, 2_u2) == cat::uint2_min);
   cat::verify(cat::sat_sub(cat::uint2_min, 100_u2) == cat::uint2_min);

   static_assert(cat::sat_sub(cat::uint4_min + 3u, 2_u4) > cat::uint4_min);
   static_assert(cat::sat_sub(cat::uint4_min + 2u, 2_u4) == cat::uint4_min);
   static_assert(cat::sat_sub(cat::uint4_min + 1u, 2u) == cat::uint4_min);
   static_assert(cat::sat_sub(cat::uint4_min.raw, 2_u4) == cat::uint4_min);
   static_assert(cat::sat_sub(cat::uint4_min, 100u) == cat::uint4_min);

   cat::verify(cat::sat_sub(cat::uint4_min + 3u, 2_u4) > cat::uint4_min);
   cat::verify(cat::sat_sub(cat::uint4_min + 2u, 2_u4) == cat::uint4_min);
   cat::verify(cat::sat_sub(cat::uint4_min + 1u, 2u) == cat::uint4_min);
   cat::verify(cat::sat_sub(cat::uint4_min.raw, 2_u4) == cat::uint4_min);
   cat::verify(cat::sat_sub(cat::uint4_min, 100u) == cat::uint4_min);

   static_assert(cat::sat_sub(cat::uint8_min + 3u, 2_u8) > cat::uint8_min);
   static_assert(cat::sat_sub(cat::uint8_min + 2u, 2_u8) == cat::uint8_min);
   static_assert(cat::sat_sub(cat::uint8_min + 1u, 2u) == cat::uint8_min);
   static_assert(cat::sat_sub(cat::uint8_min.raw, 2_u8) == cat::uint8_min);
   static_assert(cat::sat_sub(cat::uint8_min, 100u) == cat::uint8_min);

   cat::verify(cat::sat_sub(cat::uint8_min + 3u, 2_u8) > cat::uint8_min);
   cat::verify(cat::sat_sub(cat::uint8_min + 2u, 2_u8) == cat::uint8_min);
   cat::verify(cat::sat_sub(cat::uint8_min + 1u, 2u) == cat::uint8_min);
   cat::verify(cat::sat_sub(cat::uint8_min.raw, 2_u8) == cat::uint8_min);
   cat::verify(cat::sat_sub(cat::uint8_min, 100u) == cat::uint8_min);
}

test(arithmetic_sat_sub_signed_fixed_width) {
   // Test signed saturating subtraction.
   static_assert(cat::sat_sub(cat::int1_min + 3_i1, 1_i1) > cat::int1_min);
   static_assert(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
   static_assert(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
   static_assert(cat::sat_sub(cat::int1_min.raw, 1_i1) == cat::int1_min);
   static_assert(cat::sat_sub(cat::int1_min, 100_i1) == cat::int1_min);

   cat::verify(cat::sat_sub(cat::int1_min + 3, 1_i1) > cat::int1_min);
   cat::verify(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
   cat::verify(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
   cat::verify(cat::sat_sub(cat::int1_min.raw, 1_i1) == cat::int1_min);
   cat::verify(cat::sat_sub(cat::int1_min, 100_i1) == cat::int1_min);

   static_assert(cat::sat_sub(cat::int2_min + 3_i2, 2_i2) > cat::int2_min);
   static_assert(cat::sat_sub(cat::int2_min + 2_i2, 2_i2) == cat::int2_min);
   static_assert(cat::sat_sub(cat::int2_min + 1_i2, 2_i2) == cat::int2_min);
   static_assert(cat::sat_sub(cat::int2_min.raw, 2_i2) == cat::int2_min);
   static_assert(cat::sat_sub(cat::int2_min, 100_i2) == cat::int2_min);

   cat::verify(cat::sat_sub(cat::int2_min + 3_i2, 2_i2) > cat::int2_min);
   cat::verify(cat::sat_sub(cat::int2_min + 2_i2, 2_i2) == cat::int2_min);
   cat::verify(cat::sat_sub(cat::int2_min + 1_i2, 2_i2) == cat::int2_min);
   cat::verify(cat::sat_sub(cat::int2_min.raw, 2_i2) == cat::int2_min);
   cat::verify(cat::sat_sub(cat::int2_min, 100_i2) == cat::int2_min);

   static_assert(cat::sat_sub(cat::int4_min + 3, 2_i4) > cat::int4_min);
   static_assert(cat::sat_sub(cat::int4_min + 2, 2_i4) == cat::int4_min);
   static_assert(cat::sat_sub(cat::int4_min + 1, 2) == cat::int4_min);
   static_assert(cat::sat_sub(cat::int4_min.raw, 2_i4) == cat::int4_min);
   static_assert(cat::sat_sub(cat::int4_min, 100) == cat::int4_min);

   cat::verify(cat::sat_sub(cat::int4_min + 3, 2_i4) > cat::int4_min);
   cat::verify(cat::sat_sub(cat::int4_min + 2, 2_i4) == cat::int4_min);
   cat::verify(cat::sat_sub(cat::int4_min + 1, 2) == cat::int4_min);
   cat::verify(cat::sat_sub(cat::int4_min.raw, 2_i4) == cat::int4_min);
   cat::verify(cat::sat_sub(cat::int4_min, 100) == cat::int4_min);

   static_assert(cat::sat_sub(cat::int8_min + 3, 2_i8) > cat::int8_min);
   static_assert(cat::sat_sub(cat::int8_min + 2, 2_i8) == cat::int8_min);
   static_assert(cat::sat_sub(cat::int8_min + 1, 2) == cat::int8_min);
   static_assert(cat::sat_sub(cat::int8_min.raw, 2_i8) == cat::int8_min);
   static_assert(cat::sat_sub(cat::int8_min, 100) == cat::int8_min);

   cat::verify(cat::sat_sub(cat::int8_min + 3, 2_i8) > cat::int8_min);
   cat::verify(cat::sat_sub(cat::int8_min + 2, 2_i8) == cat::int8_min);
   cat::verify(cat::sat_sub(cat::int8_min + 1, 2) == cat::int8_min);
   cat::verify(cat::sat_sub(cat::int8_min.raw, 2_i8) == cat::int8_min);
   cat::verify(cat::sat_sub(cat::int8_min, 100) == cat::int8_min);
}

// `sat_add` and `sat_sub` for signed integers must pick the saturation
// endpoint by the direction of overflow, not always pin to one side.
//   * `sat_add(min, -1)` overflows downward → `min`, NOT `max`.
//   * `sat_sub(max, -1)` overflows upward → `max`, NOT `min`.
// The same-direction cases (`sat_add(max, 1) == max`,
// `sat_sub(min, 1) == min`) are covered above. Operands must be the
// same width so the inner same-type `sat_add(T, T)` overload runs --
// mixing widths goes through the promoted overload, which would not
// hit the overflow branch at sub-`int` widths.
test(arithmetic_sat_add_sub_signed_overflow_direction) {
   static_assert(cat::sat_add(cat::int1_min.raw, (-1_i1).raw)
                 == cat::int1_min.raw);
   static_assert(cat::sat_add(cat::int2_min.raw, (-1_i2).raw)
                 == cat::int2_min.raw);
   static_assert(cat::sat_add(cat::int4_min.raw, -1) == cat::int4_min.raw);
   static_assert(cat::sat_add(cat::int8_min.raw, (-1_i8).raw)
                 == cat::int8_min.raw);
   static_assert(cat::sat_add(cat::int4_min.raw, cat::int4_min.raw)
                 == cat::int4_min.raw);

   cat::verify(cat::sat_add(cat::int1_min.raw, (-1_i1).raw)
               == cat::int1_min.raw);
   cat::verify(cat::sat_add(cat::int4_min.raw, -1) == cat::int4_min.raw);
   cat::verify(cat::sat_add(cat::int4_min.raw, cat::int4_min.raw)
               == cat::int4_min.raw);

   static_assert(cat::sat_sub(cat::int1_max.raw, (-1_i1).raw)
                 == cat::int1_max.raw);
   static_assert(cat::sat_sub(cat::int2_max.raw, (-1_i2).raw)
                 == cat::int2_max.raw);
   static_assert(cat::sat_sub(cat::int4_max.raw, -1) == cat::int4_max.raw);
   static_assert(cat::sat_sub(cat::int8_max.raw, (-1_i8).raw)
                 == cat::int8_max.raw);
   static_assert(cat::sat_sub(0, cat::int4_min.raw) == cat::int4_max.raw);

   cat::verify(cat::sat_sub(cat::int1_max.raw, (-1_i1).raw)
               == cat::int1_max.raw);
   cat::verify(cat::sat_sub(cat::int4_max.raw, -1) == cat::int4_max.raw);
   cat::verify(cat::sat_sub(0, cat::int4_min.raw) == cat::int4_max.raw);
}

// `cat::sat_mul` for unsigned integers always saturates upward to `max()`.
// Same-width and mixed-width operands both go through the
// `__builtin_mul_overflow` branch.
test(arithmetic_sat_mul_unsigned_fixed_width) {
   static_assert(cat::sat_mul((2_u1).raw, 3_u1) == 6u);
   static_assert(cat::sat_mul(cat::uint1_max.raw, 1_u1) == cat::uint1_max.raw);
   static_assert(cat::sat_mul(cat::uint1_max.raw, 2_u1) == cat::uint1_max.raw);
   static_assert(cat::sat_mul(cat::uint1_max, cat::uint1_max)
                 == cat::uint1_max);
   static_assert(cat::sat_mul((0_u1).raw, cat::uint1_max.raw) == 0u);

   cat::verify(cat::sat_mul((2_u1).raw, 3_u1) == 6u);
   cat::verify(cat::sat_mul(cat::uint1_max.raw, 2_u1) == cat::uint1_max.raw);

   static_assert(cat::sat_mul(cat::uint2_max.raw, 2_u2) == cat::uint2_max.raw);
   static_assert(cat::sat_mul(cat::uint4_max.raw, 2u) == cat::uint4_max.raw);
   static_assert(cat::sat_mul(cat::uint8_max.raw, 2u) == cat::uint8_max.raw);

   cat::verify(cat::sat_mul(cat::uint4_max.raw, 2u) == cat::uint4_max.raw);
   cat::verify(cat::sat_mul(cat::uint8_max.raw, 2u) == cat::uint8_max.raw);
}

// `cat::sat_mul` for signed integers picks the saturation endpoint by the sign
// of the true product (XOR of operand signs). The `min * min` case is positive
// overflow and saturates to `max()`.
test(arithmetic_sat_mul_signed_fixed_width) {
   static_assert(cat::sat_mul((2_i1).raw, 3_i1) == 6);
   static_assert(cat::sat_mul((-2_i1).raw, 3_i1) == -6);
   static_assert(cat::sat_mul(cat::int1_max.raw, 2_i1) == cat::int1_max.raw);
   static_assert(cat::sat_mul(cat::int1_min.raw, 2_i1) == cat::int1_min.raw);
   static_assert(cat::sat_mul(cat::int1_max.raw, -2_i1) == cat::int1_min.raw);
   static_assert(cat::sat_mul(cat::int1_min.raw, -2_i1) == cat::int1_max.raw);
   static_assert(cat::sat_mul(cat::int1_min.raw, cat::int1_min.raw)
                 == cat::int1_max.raw);
   static_assert(cat::sat_mul(cat::int1_min.raw, (-1_i1).raw)
                 == cat::int1_max.raw);

   cat::verify(cat::sat_mul(cat::int1_max.raw, -2_i1) == cat::int1_min.raw);
   cat::verify(cat::sat_mul(cat::int1_min.raw, -2_i1) == cat::int1_max.raw);
   cat::verify(cat::sat_mul(cat::int1_min.raw, cat::int1_min.raw)
               == cat::int1_max.raw);

   static_assert(cat::sat_mul(cat::int4_max.raw, 2) == cat::int4_max.raw);
   static_assert(cat::sat_mul(cat::int4_max.raw, -2) == cat::int4_min.raw);
   static_assert(cat::sat_mul(cat::int4_min.raw, 2) == cat::int4_min.raw);
   static_assert(cat::sat_mul(cat::int4_min.raw, -2) == cat::int4_max.raw);
   static_assert(cat::sat_mul(cat::int4_min.raw, -1) == cat::int4_max.raw);

   cat::verify(cat::sat_mul(cat::int4_max.raw, -2) == cat::int4_min.raw);
   cat::verify(cat::sat_mul(cat::int4_min.raw, 2) == cat::int4_min.raw);
   cat::verify(cat::sat_mul(cat::int4_min.raw, -1) == cat::int4_max.raw);

   static_assert(cat::sat_mul(cat::int8_max.raw, 2_i8) == cat::int8_max.raw);
   static_assert(cat::sat_mul(cat::int8_min.raw, 2_i8) == cat::int8_min.raw);
   static_assert(cat::sat_mul(cat::int8_min.raw, (-1_i8).raw)
                 == cat::int8_max.raw);

   cat::verify(cat::sat_mul(cat::int8_max.raw, 2_i8) == cat::int8_max.raw);
   cat::verify(cat::sat_mul(cat::int8_min.raw, (-1_i8).raw)
               == cat::int8_max.raw);
}

// `cat::sat_div` only overflows for signed `min() / -1`. Every other division
// stays in range. Unsigned division never overflows.
test(arithmetic_sat_div_unsigned_fixed_width) {
   static_assert(cat::sat_div((6_u1).raw, 2_u1) == 3u);
   static_assert(cat::sat_div(cat::uint1_max.raw, 1_u1) == cat::uint1_max.raw);
   static_assert(cat::sat_div(cat::uint4_max.raw, 1u) == cat::uint4_max.raw);
   static_assert(cat::sat_div((0_u4).raw, 5u) == 0u);
   static_assert(cat::sat_div(cat::uint8_max.raw, 2u)
                 == cat::uint8_max.raw / 2u);

   cat::verify(cat::sat_div(cat::uint1_max.raw, 1_u1) == cat::uint1_max.raw);
   cat::verify(cat::sat_div(cat::uint4_max.raw, 1u) == cat::uint4_max.raw);
}

test(arithmetic_sat_div_signed_fixed_width) {
   static_assert(cat::sat_div((6_i1).raw, 2_i1) == 3);
   static_assert(cat::sat_div((6_i1).raw, -2_i1) == -3);
   static_assert(cat::sat_div(cat::int1_max.raw, 1_i1) == cat::int1_max.raw);
   static_assert(cat::sat_div(cat::int1_max.raw, -1_i1) == -cat::int1_max.raw);

   // The defining test: `min() / -1` saturates to `max()`.
   static_assert(cat::sat_div(cat::int1_min.raw, -1_i1) == cat::int1_max.raw);
   static_assert(cat::sat_div(cat::int2_min.raw, -1_i2) == cat::int2_max.raw);
   static_assert(cat::sat_div(cat::int4_min.raw, -1) == cat::int4_max.raw);
   static_assert(cat::sat_div(cat::int8_min.raw, -1) == cat::int8_max.raw);

   // Adjacent values must NOT be touched by the saturation guard.
   static_assert(cat::sat_div(cat::int4_min.raw + 1, -1) == cat::int4_max.raw);
   static_assert(cat::sat_div(cat::int4_min.raw, 1) == cat::int4_min.raw);
   static_assert(cat::sat_div(cat::int4_min.raw, -2) == cat::int4_min.raw / -2);
   static_assert(cat::sat_div(cat::int4_max.raw, -1) == -cat::int4_max.raw);

   cat::verify(cat::sat_div(cat::int1_min.raw, -1_i1) == cat::int1_max.raw);
   cat::verify(cat::sat_div(cat::int4_min.raw, -1) == cat::int4_max.raw);
   cat::verify(cat::sat_div(cat::int8_min.raw, -1) == cat::int8_max.raw);
   cat::verify(cat::sat_div(cat::int4_min.raw + 1, -1) == cat::int4_max.raw);
   cat::verify(cat::sat_div(cat::int4_max.raw, -1) == -cat::int4_max.raw);
}

// `cat::wrap_sub` mirrors C++'s well-defined unsigned modular subtraction for
// unsigned `T` and uses two's-complement wrap for signed `T`.
test(arithmetic_wrap_sub_fixed_width) {
   static_assert(cat::wrap_sub(cat::uint1_min.raw, 1_u1) == cat::uint1_max.raw);
   static_assert(cat::wrap_sub(cat::uint1_min.raw, 2_u1)
                 == cat::uint1_max.raw - 1u);
   static_assert(cat::wrap_sub((5_u4).raw, 10u) == cat::uint4_max.raw - 4u);
   static_assert(cat::wrap_sub(cat::uint4_min.raw, 1u) == cat::uint4_max.raw);
   static_assert(cat::wrap_sub(cat::uint8_min.raw, 1u) == cat::uint8_max.raw);

   cat::verify(cat::wrap_sub(cat::uint1_min.raw, 1_u1) == cat::uint1_max.raw);
   cat::verify(cat::wrap_sub(cat::uint4_min.raw, 1u) == cat::uint4_max.raw);
   cat::verify(cat::wrap_sub(cat::uint8_min.raw, 1u) == cat::uint8_max.raw);

   static_assert(cat::wrap_sub(cat::int1_min.raw, 1_i1) == cat::int1_max.raw);
   static_assert(cat::wrap_sub(cat::int1_max.raw, -1_i1) == cat::int1_min.raw);
   static_assert(cat::wrap_sub(cat::int4_min.raw, 1) == cat::int4_max.raw);
   static_assert(cat::wrap_sub(cat::int4_max.raw, -1) == cat::int4_min.raw);
   static_assert(cat::wrap_sub(cat::int8_min.raw, 1) == cat::int8_max.raw);

   cat::verify(cat::wrap_sub(cat::int1_min.raw, 1_i1) == cat::int1_max.raw);
   cat::verify(cat::wrap_sub(cat::int4_min.raw, 1) == cat::int4_max.raw);
   cat::verify(cat::wrap_sub(cat::int4_max.raw, -1) == cat::int4_min.raw);
   cat::verify(cat::wrap_sub(cat::int8_min.raw, 1) == cat::int8_max.raw);
}

// `cat::wrap_div` performs ordinary signed/unsigned division, with the sole
// exception of `min() / -1`: the mathematical result is `max() + 1`, which
// two's-complement-wraps back to `min()` itself.
test(arithmetic_wrap_div_fixed_width) {
   static_assert(cat::wrap_div((6_u1).raw, 2_u1) == 3u);
   static_assert(cat::wrap_div(cat::uint1_max.raw, 1_u1) == cat::uint1_max.raw);
   static_assert(cat::wrap_div(cat::uint4_max.raw, 1u) == cat::uint4_max.raw);
   static_assert(cat::wrap_div(cat::uint8_max.raw, 2u)
                 == cat::uint8_max.raw / 2u);

   static_assert(cat::wrap_div((6_i1).raw, 2_i1) == 3);
   static_assert(cat::wrap_div((6_i1).raw, -2_i1) == -3);
   static_assert(cat::wrap_div((-6_i1).raw, 2_i1) == -3);
   static_assert(cat::wrap_div((-6_i1).raw, -2_i1) == 3);
   static_assert(cat::wrap_div(cat::int4_max.raw, 1) == cat::int4_max.raw);
   static_assert(cat::wrap_div(cat::int4_max.raw, -1) == -cat::int4_max.raw);

   // The `min() / -1` overflow case wraps to `min()`.
   static_assert(cat::wrap_div(cat::int1_min.raw, (-1_i1).raw)
                 == cat::int1_min.raw);
   static_assert(cat::wrap_div(cat::int4_min.raw, -1) == cat::int4_min.raw);
   static_assert(cat::wrap_div(cat::int8_min.raw, (-1_i8).raw)
                 == cat::int8_min.raw);

   cat::verify(cat::wrap_div(cat::uint4_max.raw, 1u) == cat::uint4_max.raw);
   cat::verify(cat::wrap_div((6_i1).raw, -2_i1) == -3);
   cat::verify(cat::wrap_div(cat::int4_min.raw, -1) == cat::int4_min.raw);
   cat::verify(cat::wrap_div(cat::int4_max.raw, 1) == cat::int4_max.raw);
}

test(arithmetic_sat_shl_unsigned_fixed_width) {
   // uint1: in-range shifts, boundary, count >= width, zero value, zero count.
   static_assert(cat::sat_shl((1_u1).raw, 1u) == 2u);
   static_assert(cat::sat_shl((1_u1).raw, 7u) == 0x80u);
   static_assert(cat::sat_shl((0x80_u1).raw, 1u) == cat::uint1_max.raw);
   static_assert(cat::sat_shl((0x40_u1).raw, 2u) == cat::uint1_max.raw);
   static_assert(cat::sat_shl((1_u1).raw, 8u) == cat::uint1_max.raw);
   static_assert(cat::sat_shl((1_u1).raw, 100u) == cat::uint1_max.raw);
   static_assert(cat::sat_shl((0_u1).raw, 100u) == 0u);
   static_assert(cat::sat_shl(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);

   cat::verify(cat::sat_shl((1_u1).raw, 1u) == 2u);
   cat::verify(cat::sat_shl((1_u1).raw, 7u) == 0x80u);
   cat::verify(cat::sat_shl((0x80_u1).raw, 1u) == cat::uint1_max.raw);
   cat::verify(cat::sat_shl((0x40_u1).raw, 2u) == cat::uint1_max.raw);
   cat::verify(cat::sat_shl((1_u1).raw, 8u) == cat::uint1_max.raw);
   cat::verify(cat::sat_shl((1_u1).raw, 100u) == cat::uint1_max.raw);
   cat::verify(cat::sat_shl((0_u1).raw, 100u) == 0u);
   cat::verify(cat::sat_shl(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);

   // Erasure overload picks up `cat::arithmetic` operands and preserves the LHS
   // shape.
   static_assert(cat::sat_shl(0x80_u1, 1_u1) == cat::uint1_max);
   static_assert(cat::sat_shl(1_u1, 7u) == 0x80_u1);
   cat::verify(cat::sat_shl(0x80_u1, 1_u1) == cat::uint1_max);
   cat::verify(cat::sat_shl(1_u1, 7u) == 0x80_u1);

   // uint2.
   static_assert(cat::sat_shl((1_u2).raw, 1u) == 2u);
   static_assert(cat::sat_shl((1_u2).raw, 15u) == 0x8000u);
   static_assert(cat::sat_shl((0x8000_u2).raw, 1u) == cat::uint2_max.raw);
   static_assert(cat::sat_shl((1_u2).raw, 16u) == cat::uint2_max.raw);
   static_assert(cat::sat_shl((0_u2).raw, 100u) == 0u);
   static_assert(cat::sat_shl(cat::uint2_max.raw, 0u) == cat::uint2_max.raw);

   cat::verify(cat::sat_shl((1_u2).raw, 1u) == 2u);
   cat::verify(cat::sat_shl((1_u2).raw, 15u) == 0x8000u);
   cat::verify(cat::sat_shl((0x8000_u2).raw, 1u) == cat::uint2_max.raw);
   cat::verify(cat::sat_shl((1_u2).raw, 16u) == cat::uint2_max.raw);
   cat::verify(cat::sat_shl((0_u2).raw, 100u) == 0u);
   cat::verify(cat::sat_shl(cat::uint2_max.raw, 0u) == cat::uint2_max.raw);

   // uint4.
   static_assert(cat::sat_shl((1_u4).raw, 1u) == 2u);
   static_assert(cat::sat_shl((1_u4).raw, 31u) == 0x80000000u);
   static_assert(cat::sat_shl((0x80000000_u4).raw, 1u) == cat::uint4_max.raw);
   static_assert(cat::sat_shl((1_u4).raw, 32u) == cat::uint4_max.raw);
   static_assert(cat::sat_shl((0_u4).raw, 100u) == 0u);
   static_assert(cat::sat_shl(cat::uint4_max.raw, 0u) == cat::uint4_max.raw);

   cat::verify(cat::sat_shl((1_u4).raw, 1u) == 2u);
   cat::verify(cat::sat_shl((1_u4).raw, 31u) == 0x80000000u);
   cat::verify(cat::sat_shl((0x80000000_u4).raw, 1u) == cat::uint4_max.raw);
   cat::verify(cat::sat_shl((1_u4).raw, 32u) == cat::uint4_max.raw);
   cat::verify(cat::sat_shl((0_u4).raw, 100u) == 0u);
   cat::verify(cat::sat_shl(cat::uint4_max.raw, 0u) == cat::uint4_max.raw);

   // uint8.
   static_assert(cat::sat_shl((1_u8).raw, 1u) == 2ull);
   static_assert(cat::sat_shl((1_u8).raw, 63u) == (1ull << 63u));
   static_assert(cat::sat_shl((1ull << 63u), 1u) == cat::uint8_max.raw);
   static_assert(cat::sat_shl((1_u8).raw, 64u) == cat::uint8_max.raw);
   static_assert(cat::sat_shl((0_u8).raw, 100u) == 0u);
   static_assert(cat::sat_shl(cat::uint8_max.raw, 0u) == cat::uint8_max.raw);

   cat::verify(cat::sat_shl((1_u8).raw, 1u) == 2ull);
   cat::verify(cat::sat_shl((1_u8).raw, 63u) == (1ull << 63u));
   cat::verify(cat::sat_shl((1ull << 63u), 1u) == cat::uint8_max.raw);
   cat::verify(cat::sat_shl((1_u8).raw, 64u) == cat::uint8_max.raw);
   cat::verify(cat::sat_shl((0_u8).raw, 100u) == 0u);
   cat::verify(cat::sat_shl(cat::uint8_max.raw, 0u) == cat::uint8_max.raw);
}

test(arithmetic_sat_shl_signed_fixed_width) {
   // int1: in-range shifts, positive overflow, negative overflow, edges.
   static_assert(cat::sat_shl((1_i1).raw, 1u) == 2);
   static_assert(cat::sat_shl((1_i1).raw, 6u) == 0x40);
   // Shifting a 1 into the sign bit is positive overflow -> clamp to max.
   static_assert(cat::sat_shl((1_i1).raw, 7u) == cat::int1_max.raw);
   static_assert(cat::sat_shl((1_i1).raw, 8u) == cat::int1_max.raw);
   // Negative shifts that fit don't saturate.
   static_assert(cat::sat_shl((-1_i1).raw, 6u) == -64);
   static_assert(cat::sat_shl((-1_i1).raw, 7u) == cat::int1_min.raw);
   // Negative overflow clamps to min.
   static_assert(cat::sat_shl((-2_i1).raw, 7u) == cat::int1_min.raw);
   static_assert(cat::sat_shl((-1_i1).raw, 8u) == cat::int1_min.raw);
   // `int1_min` shifted further is still negative overflow.
   static_assert(cat::sat_shl(cat::int1_min.raw, 1u) == cat::int1_min.raw);
   // Zero value, any count -> 0.
   static_assert(cat::sat_shl((0_i1).raw, 100u) == 0);
   // Zero count -> no-op.
   static_assert(cat::sat_shl(cat::int1_max.raw, 0u) == cat::int1_max.raw);
   static_assert(cat::sat_shl(cat::int1_min.raw, 0u) == cat::int1_min.raw);

   cat::verify(cat::sat_shl((1_i1).raw, 1u) == 2);
   cat::verify(cat::sat_shl((1_i1).raw, 6u) == 0x40);
   cat::verify(cat::sat_shl((1_i1).raw, 7u) == cat::int1_max.raw);
   cat::verify(cat::sat_shl((1_i1).raw, 8u) == cat::int1_max.raw);
   cat::verify(cat::sat_shl((-1_i1).raw, 6u) == -64);
   cat::verify(cat::sat_shl((-1_i1).raw, 7u) == cat::int1_min.raw);
   cat::verify(cat::sat_shl((-2_i1).raw, 7u) == cat::int1_min.raw);
   cat::verify(cat::sat_shl((-1_i1).raw, 8u) == cat::int1_min.raw);
   cat::verify(cat::sat_shl(cat::int1_min.raw, 1u) == cat::int1_min.raw);
   cat::verify(cat::sat_shl((0_i1).raw, 100u) == 0);
   cat::verify(cat::sat_shl(cat::int1_max.raw, 0u) == cat::int1_max.raw);
   cat::verify(cat::sat_shl(cat::int1_min.raw, 0u) == cat::int1_min.raw);

   // Erasure overload preserves the LHS shape.
   static_assert(cat::sat_shl(1_i1, 7u) == cat::int1_max);
   static_assert(cat::sat_shl(-2_i1, 7u) == cat::int1_min);
   cat::verify(cat::sat_shl(1_i1, 7u) == cat::int1_max);
   cat::verify(cat::sat_shl(-2_i1, 7u) == cat::int1_min);

   // int2.
   static_assert(cat::sat_shl((1_i2).raw, 14u) == 0x4000);
   static_assert(cat::sat_shl((1_i2).raw, 15u) == cat::int2_max.raw);
   static_assert(cat::sat_shl((1_i2).raw, 16u) == cat::int2_max.raw);
   static_assert(cat::sat_shl((-2_i2).raw, 15u) == cat::int2_min.raw);
   static_assert(cat::sat_shl((-1_i2).raw, 16u) == cat::int2_min.raw);
   static_assert(cat::sat_shl(cat::int2_min.raw, 1u) == cat::int2_min.raw);
   static_assert(cat::sat_shl((0_i2).raw, 100u) == 0);

   cat::verify(cat::sat_shl((1_i2).raw, 14u) == 0x4000);
   cat::verify(cat::sat_shl((1_i2).raw, 15u) == cat::int2_max.raw);
   cat::verify(cat::sat_shl((1_i2).raw, 16u) == cat::int2_max.raw);
   cat::verify(cat::sat_shl((-2_i2).raw, 15u) == cat::int2_min.raw);
   cat::verify(cat::sat_shl((-1_i2).raw, 16u) == cat::int2_min.raw);
   cat::verify(cat::sat_shl(cat::int2_min.raw, 1u) == cat::int2_min.raw);
   cat::verify(cat::sat_shl((0_i2).raw, 100u) == 0);

   // int4.
   static_assert(cat::sat_shl((1_i4).raw, 30u) == 0x40000000);
   static_assert(cat::sat_shl((1_i4).raw, 31u) == cat::int4_max.raw);
   static_assert(cat::sat_shl((1_i4).raw, 32u) == cat::int4_max.raw);
   static_assert(cat::sat_shl((-2_i4).raw, 31u) == cat::int4_min.raw);
   static_assert(cat::sat_shl((-1_i4).raw, 32u) == cat::int4_min.raw);
   static_assert(cat::sat_shl(cat::int4_min.raw, 1u) == cat::int4_min.raw);
   static_assert(cat::sat_shl((0_i4).raw, 100u) == 0);

   cat::verify(cat::sat_shl((1_i4).raw, 30u) == 0x40000000);
   cat::verify(cat::sat_shl((1_i4).raw, 31u) == cat::int4_max.raw);
   cat::verify(cat::sat_shl((1_i4).raw, 32u) == cat::int4_max.raw);
   cat::verify(cat::sat_shl((-2_i4).raw, 31u) == cat::int4_min.raw);
   cat::verify(cat::sat_shl((-1_i4).raw, 32u) == cat::int4_min.raw);
   cat::verify(cat::sat_shl(cat::int4_min.raw, 1u) == cat::int4_min.raw);
   cat::verify(cat::sat_shl((0_i4).raw, 100u) == 0);

   // int8.
   static_assert(cat::sat_shl((1_i8).raw, 62u) == (1ll << 62u));
   static_assert(cat::sat_shl((1_i8).raw, 63u) == cat::int8_max.raw);
   static_assert(cat::sat_shl((1_i8).raw, 64u) == cat::int8_max.raw);
   static_assert(cat::sat_shl((-2_i8).raw, 63u) == cat::int8_min.raw);
   static_assert(cat::sat_shl((-1_i8).raw, 64u) == cat::int8_min.raw);
   static_assert(cat::sat_shl(cat::int8_min.raw, 1u) == cat::int8_min.raw);
   static_assert(cat::sat_shl((0_i8).raw, 100u) == 0);

   cat::verify(cat::sat_shl((1_i8).raw, 62u) == (1ll << 62u));
   cat::verify(cat::sat_shl((1_i8).raw, 63u) == cat::int8_max.raw);
   cat::verify(cat::sat_shl((1_i8).raw, 64u) == cat::int8_max.raw);
   cat::verify(cat::sat_shl((-2_i8).raw, 63u) == cat::int8_min.raw);
   cat::verify(cat::sat_shl((-1_i8).raw, 64u) == cat::int8_min.raw);
   cat::verify(cat::sat_shl(cat::int8_min.raw, 1u) == cat::int8_min.raw);
   cat::verify(cat::sat_shl((0_i8).raw, 100u) == 0);
}

test(arithmetic_sat_shr_unsigned_fixed_width) {
   // Right shift never overflows. Saturation only guards the count >= width
   // case (and equivalently the count >= bit_width(value) case, where the
   // result is mathematically zero anyway).

   // uint1.
   static_assert(cat::sat_shr(cat::uint1_max.raw, 4u) == 0x0Fu);
   static_assert(cat::sat_shr((0x80_u1).raw, 7u) == 1u);
   static_assert(cat::sat_shr(cat::uint1_max.raw, 8u) == 0u);
   static_assert(cat::sat_shr(cat::uint1_max.raw, 100u) == 0u);
   static_assert(cat::sat_shr((0_u1).raw, 100u) == 0u);
   static_assert(cat::sat_shr(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);

   cat::verify(cat::sat_shr(cat::uint1_max.raw, 4u) == 0x0Fu);
   cat::verify(cat::sat_shr((0x80_u1).raw, 7u) == 1u);
   cat::verify(cat::sat_shr(cat::uint1_max.raw, 8u) == 0u);
   cat::verify(cat::sat_shr(cat::uint1_max.raw, 100u) == 0u);
   cat::verify(cat::sat_shr((0_u1).raw, 100u) == 0u);
   cat::verify(cat::sat_shr(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);

   // Erasure overload preserves the LHS shape.
   static_assert(cat::sat_shr(cat::uint1_max, 4u) == 0x0F_u1);
   static_assert(cat::sat_shr(cat::uint1_max, 8u) == 0_u1);
   cat::verify(cat::sat_shr(cat::uint1_max, 4u) == 0x0F_u1);
   cat::verify(cat::sat_shr(cat::uint1_max, 8u) == 0_u1);

   // uint2.
   static_assert(cat::sat_shr(cat::uint2_max.raw, 8u) == 0xFFu);
   static_assert(cat::sat_shr((0x8000_u2).raw, 15u) == 1u);
   static_assert(cat::sat_shr(cat::uint2_max.raw, 16u) == 0u);
   static_assert(cat::sat_shr(cat::uint2_max.raw, 100u) == 0u);
   static_assert(cat::sat_shr((0_u2).raw, 100u) == 0u);
   static_assert(cat::sat_shr(cat::uint2_max.raw, 0u) == cat::uint2_max.raw);

   cat::verify(cat::sat_shr(cat::uint2_max.raw, 8u) == 0xFFu);
   cat::verify(cat::sat_shr((0x8000_u2).raw, 15u) == 1u);
   cat::verify(cat::sat_shr(cat::uint2_max.raw, 16u) == 0u);
   cat::verify(cat::sat_shr(cat::uint2_max.raw, 100u) == 0u);
   cat::verify(cat::sat_shr((0_u2).raw, 100u) == 0u);
   cat::verify(cat::sat_shr(cat::uint2_max.raw, 0u) == cat::uint2_max.raw);

   // uint4.
   static_assert(cat::sat_shr(cat::uint4_max.raw, 16u) == 0xFFFFu);
   static_assert(cat::sat_shr((0x80000000_u4).raw, 31u) == 1u);
   static_assert(cat::sat_shr(cat::uint4_max.raw, 32u) == 0u);
   static_assert(cat::sat_shr(cat::uint4_max.raw, 100u) == 0u);
   static_assert(cat::sat_shr((0_u4).raw, 100u) == 0u);
   static_assert(cat::sat_shr(cat::uint4_max.raw, 0u) == cat::uint4_max.raw);

   cat::verify(cat::sat_shr(cat::uint4_max.raw, 16u) == 0xFFFFu);
   cat::verify(cat::sat_shr((0x80000000_u4).raw, 31u) == 1u);
   cat::verify(cat::sat_shr(cat::uint4_max.raw, 32u) == 0u);
   cat::verify(cat::sat_shr(cat::uint4_max.raw, 100u) == 0u);
   cat::verify(cat::sat_shr((0_u4).raw, 100u) == 0u);
   cat::verify(cat::sat_shr(cat::uint4_max.raw, 0u) == cat::uint4_max.raw);

   // uint8.
   static_assert(cat::sat_shr(cat::uint8_max.raw, 32u) == 0xFFFFFFFFull);
   static_assert(cat::sat_shr((1ull << 63u), 63u) == 1ull);
   static_assert(cat::sat_shr(cat::uint8_max.raw, 64u) == 0u);
   static_assert(cat::sat_shr(cat::uint8_max.raw, 100u) == 0u);
   static_assert(cat::sat_shr((0_u8).raw, 100u) == 0u);
   static_assert(cat::sat_shr(cat::uint8_max.raw, 0u) == cat::uint8_max.raw);

   cat::verify(cat::sat_shr(cat::uint8_max.raw, 32u) == 0xFFFFFFFFull);
   cat::verify(cat::sat_shr((1ull << 63u), 63u) == 1ull);
   cat::verify(cat::sat_shr(cat::uint8_max.raw, 64u) == 0u);
   cat::verify(cat::sat_shr(cat::uint8_max.raw, 100u) == 0u);
   cat::verify(cat::sat_shr((0_u8).raw, 100u) == 0u);
   cat::verify(cat::sat_shr(cat::uint8_max.raw, 0u) == cat::uint8_max.raw);
}

test(arithmetic_sat_shr_signed_fixed_width) {
   // Signed right shift is the C++ arithmetic shift for in-range counts:
   // positives lose bits off the bottom, negatives sign-extend. For counts at
   // or above the bit width (the C++-undefined regime), positives clamp to `0`
   // and negatives clamp to `-1` (sign bit fully replicated).

   // int1.
   static_assert(cat::sat_shr((0x40_i1).raw, 4u) == 4);
   static_assert(cat::sat_shr((-8_i1).raw, 2u) == -2);
   static_assert(cat::sat_shr(cat::int1_max.raw, 7u) == 0);
   static_assert(cat::sat_shr(cat::int1_min.raw, 7u) == -1);
   static_assert(cat::sat_shr((0x40_i1).raw, 8u) == 0);
   static_assert(cat::sat_shr((-1_i1).raw, 8u) == -1);
   static_assert(cat::sat_shr((0x40_i1).raw, 100u) == 0);
   static_assert(cat::sat_shr((-1_i1).raw, 100u) == -1);
   static_assert(cat::sat_shr((0_i1).raw, 100u) == 0);
   static_assert(cat::sat_shr(cat::int1_max.raw, 0u) == cat::int1_max.raw);
   static_assert(cat::sat_shr(cat::int1_min.raw, 0u) == cat::int1_min.raw);

   cat::verify(cat::sat_shr((0x40_i1).raw, 4u) == 4);
   cat::verify(cat::sat_shr((-8_i1).raw, 2u) == -2);
   cat::verify(cat::sat_shr(cat::int1_max.raw, 7u) == 0);
   cat::verify(cat::sat_shr(cat::int1_min.raw, 7u) == -1);
   cat::verify(cat::sat_shr((0x40_i1).raw, 8u) == 0);
   cat::verify(cat::sat_shr((-1_i1).raw, 8u) == -1);
   cat::verify(cat::sat_shr((0x40_i1).raw, 100u) == 0);
   cat::verify(cat::sat_shr((-1_i1).raw, 100u) == -1);
   cat::verify(cat::sat_shr((0_i1).raw, 100u) == 0);
   cat::verify(cat::sat_shr(cat::int1_max.raw, 0u) == cat::int1_max.raw);
   cat::verify(cat::sat_shr(cat::int1_min.raw, 0u) == cat::int1_min.raw);

   // Erasure overload preserves the LHS shape and signedness behavior.
   static_assert(cat::sat_shr(-8_i1, 2u) == -2_i1);
   static_assert(cat::sat_shr(-1_i1, 8u) == -1_i1);
   cat::verify(cat::sat_shr(-8_i1, 2u) == -2_i1);
   cat::verify(cat::sat_shr(-1_i1, 8u) == -1_i1);

   // int2.
   static_assert(cat::sat_shr((0x4000_i2).raw, 4u) == 0x400);
   static_assert(cat::sat_shr((-128_i2).raw, 2u) == -32);
   static_assert(cat::sat_shr(cat::int2_max.raw, 16u) == 0);
   static_assert(cat::sat_shr(cat::int2_min.raw, 16u) == -1);
   static_assert(cat::sat_shr((0_i2).raw, 100u) == 0);

   cat::verify(cat::sat_shr((0x4000_i2).raw, 4u) == 0x400);
   cat::verify(cat::sat_shr((-128_i2).raw, 2u) == -32);
   cat::verify(cat::sat_shr(cat::int2_max.raw, 16u) == 0);
   cat::verify(cat::sat_shr(cat::int2_min.raw, 16u) == -1);
   cat::verify(cat::sat_shr((0_i2).raw, 100u) == 0);

   // int4.
   static_assert(cat::sat_shr((0x40000000_i4).raw, 4u) == 0x4000000);
   static_assert(cat::sat_shr((-32'768_i4).raw, 2u) == -8'192);
   static_assert(cat::sat_shr(cat::int4_max.raw, 32u) == 0);
   static_assert(cat::sat_shr(cat::int4_min.raw, 32u) == -1);
   static_assert(cat::sat_shr((0_i4).raw, 100u) == 0);

   cat::verify(cat::sat_shr((0x40000000_i4).raw, 4u) == 0x4000000);
   cat::verify(cat::sat_shr((-32'768_i4).raw, 2u) == -8'192);
   cat::verify(cat::sat_shr(cat::int4_max.raw, 32u) == 0);
   cat::verify(cat::sat_shr(cat::int4_min.raw, 32u) == -1);
   cat::verify(cat::sat_shr((0_i4).raw, 100u) == 0);

   // int8.
   static_assert(cat::sat_shr((1ll << 62u), 4u) == (1ll << 58u));
   static_assert(cat::sat_shr((-1ll << 32u), 2u) == (-1ll << 30u));
   static_assert(cat::sat_shr(cat::int8_max.raw, 64u) == 0);
   static_assert(cat::sat_shr(cat::int8_min.raw, 64u) == -1);
   static_assert(cat::sat_shr((0_i8).raw, 100u) == 0);

   cat::verify(cat::sat_shr((1ll << 62u), 4u) == (1ll << 58u));
   cat::verify(cat::sat_shr((-1ll << 32u), 2u) == (-1ll << 30u));
   cat::verify(cat::sat_shr(cat::int8_max.raw, 64u) == 0);
   cat::verify(cat::sat_shr(cat::int8_min.raw, 64u) == -1);
   cat::verify(cat::sat_shr((0_i8).raw, 100u) == 0);
}

test(arithmetic_wrap_shl_unsigned_fixed_width) {
   // Wrap left shift rotates bits: bits shifted off the high end re-enter at
   // the low end. The count is taken modulo the bit width, so any non-negative
   // count is valid.

   // uint1: 8-bit rotation.
   static_assert(cat::wrap_shl((1_u1).raw, 1u) == 2u);
   static_assert(cat::wrap_shl((1_u1).raw, 7u) == 0x80u);
   // High bit rotates around to bit 0.
   static_assert(cat::wrap_shl((0x80_u1).raw, 1u) == 1u);
   // Two adjacent high bits: one stays, one wraps.
   static_assert(cat::wrap_shl((0xC0_u1).raw, 1u) == 0x81u);
   // Symmetric pattern is invariant under rotation by half the bit width.
   static_assert(cat::wrap_shl((0x55_u1).raw, 4u) == 0x55u);
   static_assert(cat::wrap_shl((0x12_u1).raw, 4u) == 0x21u);
   // Count == bit_width is a no-op.
   static_assert(cat::wrap_shl((1_u1).raw, 8u) == 1u);
   // Count > bit_width is taken modulo.
   static_assert(cat::wrap_shl((1_u1).raw, 9u) == 2u);
   static_assert(cat::wrap_shl((0x80_u1).raw, 17u) == 1u);
   // Zero count is a no-op.
   static_assert(cat::wrap_shl(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);
   // Zero value stays zero for any count.
   static_assert(cat::wrap_shl((0_u1).raw, 100u) == 0u);
   // All-ones rotates to itself.
   static_assert(cat::wrap_shl(cat::uint1_max.raw, 3u) == cat::uint1_max.raw);

   cat::verify(cat::wrap_shl((1_u1).raw, 1u) == 2u);
   cat::verify(cat::wrap_shl((1_u1).raw, 7u) == 0x80u);
   cat::verify(cat::wrap_shl((0x80_u1).raw, 1u) == 1u);
   cat::verify(cat::wrap_shl((0xC0_u1).raw, 1u) == 0x81u);
   cat::verify(cat::wrap_shl((0x55_u1).raw, 4u) == 0x55u);
   cat::verify(cat::wrap_shl((0x12_u1).raw, 4u) == 0x21u);
   cat::verify(cat::wrap_shl((1_u1).raw, 8u) == 1u);
   cat::verify(cat::wrap_shl((1_u1).raw, 9u) == 2u);
   cat::verify(cat::wrap_shl((0x80_u1).raw, 17u) == 1u);
   cat::verify(cat::wrap_shl(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);
   cat::verify(cat::wrap_shl((0_u1).raw, 100u) == 0u);
   cat::verify(cat::wrap_shl(cat::uint1_max.raw, 3u) == cat::uint1_max.raw);

   // Erasure overload preserves the LHS shape.
   static_assert(cat::wrap_shl(0x80_u1, 1u) == 1_u1);
   static_assert(cat::wrap_shl(0xC0_u1, 1u) == 0x81_u1);
   cat::verify(cat::wrap_shl(0x80_u1, 1u) == 1_u1);
   cat::verify(cat::wrap_shl(0xC0_u1, 1u) == 0x81_u1);

   // uint2: 16-bit rotation.
   static_assert(cat::wrap_shl((1_u2).raw, 15u) == 0x8000u);
   static_assert(cat::wrap_shl((0x8000_u2).raw, 1u) == 1u);
   static_assert(cat::wrap_shl((1_u2).raw, 16u) == 1u);
   static_assert(cat::wrap_shl((1_u2).raw, 17u) == 2u);
   static_assert(cat::wrap_shl((0x8000_u2).raw, 33u) == 1u);
   static_assert(cat::wrap_shl((0_u2).raw, 100u) == 0u);
   static_assert(cat::wrap_shl(cat::uint2_max.raw, 7u) == cat::uint2_max.raw);

   cat::verify(cat::wrap_shl((1_u2).raw, 15u) == 0x8000u);
   cat::verify(cat::wrap_shl((0x8000_u2).raw, 1u) == 1u);
   cat::verify(cat::wrap_shl((1_u2).raw, 16u) == 1u);
   cat::verify(cat::wrap_shl((1_u2).raw, 17u) == 2u);
   cat::verify(cat::wrap_shl((0x8000_u2).raw, 33u) == 1u);
   cat::verify(cat::wrap_shl((0_u2).raw, 100u) == 0u);
   cat::verify(cat::wrap_shl(cat::uint2_max.raw, 7u) == cat::uint2_max.raw);

   // uint4: 32-bit rotation.
   static_assert(cat::wrap_shl((1_u4).raw, 31u) == 0x80000000u);
   static_assert(cat::wrap_shl((0x80000000_u4).raw, 1u) == 1u);
   static_assert(cat::wrap_shl((0xDEADBEEF_u4).raw, 32u) == 0xDEADBEEFu);
   static_assert(cat::wrap_shl((0x12345678_u4).raw, 4u) == 0x23456781u);
   static_assert(cat::wrap_shl((0_u4).raw, 100u) == 0u);
   static_assert(cat::wrap_shl(cat::uint4_max.raw, 7u) == cat::uint4_max.raw);

   cat::verify(cat::wrap_shl((1_u4).raw, 31u) == 0x80000000u);
   cat::verify(cat::wrap_shl((0x80000000_u4).raw, 1u) == 1u);
   cat::verify(cat::wrap_shl((0xDEADBEEF_u4).raw, 32u) == 0xDEADBEEFu);
   cat::verify(cat::wrap_shl((0x12345678_u4).raw, 4u) == 0x23456781u);
   cat::verify(cat::wrap_shl((0_u4).raw, 100u) == 0u);
   cat::verify(cat::wrap_shl(cat::uint4_max.raw, 7u) == cat::uint4_max.raw);

   // uint8: 64-bit rotation.
   static_assert(cat::wrap_shl((1_u8).raw, 63u) == (1ull << 63u));
   static_assert(cat::wrap_shl((1ull << 63u), 1u) == 1ull);
   static_assert(cat::wrap_shl((1_u8).raw, 64u) == 1ull);
   static_assert(cat::wrap_shl((1_u8).raw, 65u) == 2ull);
   static_assert(cat::wrap_shl((1ull << 63u), 129u) == 1ull);
   static_assert(cat::wrap_shl((0_u8).raw, 100u) == 0u);
   static_assert(cat::wrap_shl(cat::uint8_max.raw, 7u) == cat::uint8_max.raw);

   cat::verify(cat::wrap_shl((1_u8).raw, 63u) == (1ull << 63u));
   cat::verify(cat::wrap_shl((1ull << 63u), 1u) == 1ull);
   cat::verify(cat::wrap_shl((1_u8).raw, 64u) == 1ull);
   cat::verify(cat::wrap_shl((1_u8).raw, 65u) == 2ull);
   cat::verify(cat::wrap_shl((1ull << 63u), 129u) == 1ull);
   cat::verify(cat::wrap_shl((0_u8).raw, 100u) == 0u);
   cat::verify(cat::wrap_shl(cat::uint8_max.raw, 7u) == cat::uint8_max.raw);
}

test(arithmetic_wrap_shl_signed_fixed_width) {
   // Signed wrap shift operates on the raw bit pattern: the sign bit is just
   // bit `N-1`, rotating in and out like any other bit. The result depends only
   // on the bit pattern, not on the sign.

   // int1: 8-bit rotation. All-ones pattern (-1) is invariant under rotation.
   static_assert(cat::wrap_shl((-1_i1).raw, 3u) == (-1_i1).raw);
   static_assert(cat::wrap_shl((-1_i1).raw, 8u) == (-1_i1).raw);
   // Rotating bit 6 into bit 7 flips the sign.
   static_assert(cat::wrap_shl((0x40_i1).raw, 1u) == cat::int1_min.raw);
   // Rotating the sign bit around to bit 0 leaves a positive value.
   static_assert(cat::wrap_shl(cat::int1_min.raw, 1u) == 1);
   // Zero value stays zero.
   static_assert(cat::wrap_shl((0_i1).raw, 100u) == 0);
   // Zero count is a no-op, including on negative values.
   static_assert(cat::wrap_shl((-42_i1).raw, 0u) == (-42_i1).raw);
   // Count taken modulo bit_width.
   static_assert(cat::wrap_shl(cat::int1_min.raw, 8u) == cat::int1_min.raw);
   static_assert(cat::wrap_shl(cat::int1_min.raw, 9u) == 1);

   cat::verify(cat::wrap_shl((-1_i1).raw, 3u) == (-1_i1).raw);
   cat::verify(cat::wrap_shl((-1_i1).raw, 8u) == (-1_i1).raw);
   cat::verify(cat::wrap_shl((0x40_i1).raw, 1u) == cat::int1_min.raw);
   cat::verify(cat::wrap_shl(cat::int1_min.raw, 1u) == 1);
   cat::verify(cat::wrap_shl((0_i1).raw, 100u) == 0);
   cat::verify(cat::wrap_shl((-42_i1).raw, 0u) == (-42_i1).raw);
   cat::verify(cat::wrap_shl(cat::int1_min.raw, 8u) == cat::int1_min.raw);
   cat::verify(cat::wrap_shl(cat::int1_min.raw, 9u) == 1);

   // Erasure overload preserves the LHS shape.
   static_assert(cat::wrap_shl(0x40_i1, 1u) == cat::int1_min);
   static_assert(cat::wrap_shl(cat::int1_min, 1u) == 1_i1);
   cat::verify(cat::wrap_shl(0x40_i1, 1u) == cat::int1_min);
   cat::verify(cat::wrap_shl(cat::int1_min, 1u) == 1_i1);

   // int2.
   static_assert(cat::wrap_shl((-1_i2).raw, 5u) == (-1_i2).raw);
   static_assert(cat::wrap_shl((0x4000_i2).raw, 1u) == cat::int2_min.raw);
   static_assert(cat::wrap_shl(cat::int2_min.raw, 1u) == 1);
   static_assert(cat::wrap_shl((0_i2).raw, 100u) == 0);
   static_assert(cat::wrap_shl(cat::int2_min.raw, 16u) == cat::int2_min.raw);

   cat::verify(cat::wrap_shl((-1_i2).raw, 5u) == (-1_i2).raw);
   cat::verify(cat::wrap_shl((0x4000_i2).raw, 1u) == cat::int2_min.raw);
   cat::verify(cat::wrap_shl(cat::int2_min.raw, 1u) == 1);
   cat::verify(cat::wrap_shl((0_i2).raw, 100u) == 0);
   cat::verify(cat::wrap_shl(cat::int2_min.raw, 16u) == cat::int2_min.raw);

   // int4.
   static_assert(cat::wrap_shl((-1_i4).raw, 7u) == (-1_i4).raw);
   static_assert(cat::wrap_shl((0x40000000_i4).raw, 1u) == cat::int4_min.raw);
   static_assert(cat::wrap_shl(cat::int4_min.raw, 1u) == 1);
   static_assert(cat::wrap_shl((0_i4).raw, 100u) == 0);
   static_assert(cat::wrap_shl(cat::int4_min.raw, 32u) == cat::int4_min.raw);

   cat::verify(cat::wrap_shl((-1_i4).raw, 7u) == (-1_i4).raw);
   cat::verify(cat::wrap_shl((0x40000000_i4).raw, 1u) == cat::int4_min.raw);
   cat::verify(cat::wrap_shl(cat::int4_min.raw, 1u) == 1);
   cat::verify(cat::wrap_shl((0_i4).raw, 100u) == 0);
   cat::verify(cat::wrap_shl(cat::int4_min.raw, 32u) == cat::int4_min.raw);

   // int8.
   static_assert(cat::wrap_shl((-1_i8).raw, 11u) == (-1_i8).raw);
   static_assert(cat::wrap_shl((1ll << 62u), 1u) == cat::int8_min.raw);
   static_assert(cat::wrap_shl(cat::int8_min.raw, 1u) == 1);
   static_assert(cat::wrap_shl((0_i8).raw, 100u) == 0);
   static_assert(cat::wrap_shl(cat::int8_min.raw, 64u) == cat::int8_min.raw);

   cat::verify(cat::wrap_shl((-1_i8).raw, 11u) == (-1_i8).raw);
   cat::verify(cat::wrap_shl((1ll << 62u), 1u) == cat::int8_min.raw);
   cat::verify(cat::wrap_shl(cat::int8_min.raw, 1u) == 1);
   cat::verify(cat::wrap_shl((0_i8).raw, 100u) == 0);
   cat::verify(cat::wrap_shl(cat::int8_min.raw, 64u) == cat::int8_min.raw);
}

test(arithmetic_wrap_shr_unsigned_fixed_width) {
   // Wrap right shift is the mirror of `wrap_shl`: bits shifted off the low end
   // re-enter at the high end. The count is taken modulo the bit width.

   // uint1: 8-bit rotation.
   static_assert(cat::wrap_shr((0x80_u1).raw, 1u) == 0x40u);
   // Low bit rotates around to the high bit.
   static_assert(cat::wrap_shr((1_u1).raw, 1u) == 0x80u);
   // Two low bits: one stays, one wraps around.
   static_assert(cat::wrap_shr((0x03_u1).raw, 1u) == 0x81u);
   // Symmetric pattern is invariant under rotation by 4.
   static_assert(cat::wrap_shr((0x55_u1).raw, 4u) == 0x55u);
   static_assert(cat::wrap_shr((0x12_u1).raw, 4u) == 0x21u);
   // Count taken modulo bit_width.
   static_assert(cat::wrap_shr((1_u1).raw, 8u) == 1u);
   static_assert(cat::wrap_shr((1_u1).raw, 9u) == 0x80u);
   static_assert(cat::wrap_shr((1_u1).raw, 17u) == 0x80u);
   static_assert(cat::wrap_shr(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);
   static_assert(cat::wrap_shr((0_u1).raw, 100u) == 0u);
   static_assert(cat::wrap_shr(cat::uint1_max.raw, 3u) == cat::uint1_max.raw);

   cat::verify(cat::wrap_shr((0x80_u1).raw, 1u) == 0x40u);
   cat::verify(cat::wrap_shr((1_u1).raw, 1u) == 0x80u);
   cat::verify(cat::wrap_shr((0x03_u1).raw, 1u) == 0x81u);
   cat::verify(cat::wrap_shr((0x55_u1).raw, 4u) == 0x55u);
   cat::verify(cat::wrap_shr((0x12_u1).raw, 4u) == 0x21u);
   cat::verify(cat::wrap_shr((1_u1).raw, 8u) == 1u);
   cat::verify(cat::wrap_shr((1_u1).raw, 9u) == 0x80u);
   cat::verify(cat::wrap_shr((1_u1).raw, 17u) == 0x80u);
   cat::verify(cat::wrap_shr(cat::uint1_max.raw, 0u) == cat::uint1_max.raw);
   cat::verify(cat::wrap_shr((0_u1).raw, 100u) == 0u);
   cat::verify(cat::wrap_shr(cat::uint1_max.raw, 3u) == cat::uint1_max.raw);

   // Erasure overload preserves the LHS shape.
   static_assert(cat::wrap_shr(1_u1, 1u) == 0x80_u1);
   static_assert(cat::wrap_shr(0x03_u1, 1u) == 0x81_u1);
   cat::verify(cat::wrap_shr(1_u1, 1u) == 0x80_u1);
   cat::verify(cat::wrap_shr(0x03_u1, 1u) == 0x81_u1);

   // `wrap_shl` and `wrap_shr` are inverses under the same count.
   static_assert(cat::wrap_shr(cat::wrap_shl((0xA5_u1).raw, 3u), 3u)
                 == (0xA5_u1).raw);
   static_assert(cat::wrap_shl(cat::wrap_shr((0xA5_u1).raw, 3u), 3u)
                 == (0xA5_u1).raw);
   cat::verify(cat::wrap_shr(cat::wrap_shl((0xA5_u1).raw, 3u), 3u)
               == (0xA5_u1).raw);
   cat::verify(cat::wrap_shl(cat::wrap_shr((0xA5_u1).raw, 3u), 3u)
               == (0xA5_u1).raw);

   // uint2.
   static_assert(cat::wrap_shr((0x8000_u2).raw, 15u) == 1u);
   static_assert(cat::wrap_shr((1_u2).raw, 1u) == 0x8000u);
   static_assert(cat::wrap_shr((0x0003_u2).raw, 1u) == 0x8001u);
   static_assert(cat::wrap_shr((1_u2).raw, 16u) == 1u);
   static_assert(cat::wrap_shr((1_u2).raw, 17u) == 0x8000u);
   static_assert(cat::wrap_shr((0_u2).raw, 100u) == 0u);

   cat::verify(cat::wrap_shr((0x8000_u2).raw, 15u) == 1u);
   cat::verify(cat::wrap_shr((1_u2).raw, 1u) == 0x8000u);
   cat::verify(cat::wrap_shr((0x0003_u2).raw, 1u) == 0x8001u);
   cat::verify(cat::wrap_shr((1_u2).raw, 16u) == 1u);
   cat::verify(cat::wrap_shr((1_u2).raw, 17u) == 0x8000u);
   cat::verify(cat::wrap_shr((0_u2).raw, 100u) == 0u);

   // uint4.
   static_assert(cat::wrap_shr((0x80000000_u4).raw, 31u) == 1u);
   static_assert(cat::wrap_shr((1_u4).raw, 1u) == 0x80000000u);
   static_assert(cat::wrap_shr((0xDEADBEEF_u4).raw, 32u) == 0xDEADBEEFu);
   static_assert(cat::wrap_shr((0x12345678_u4).raw, 4u) == 0x81234567u);
   static_assert(cat::wrap_shr((0_u4).raw, 100u) == 0u);

   cat::verify(cat::wrap_shr((0x80000000_u4).raw, 31u) == 1u);
   cat::verify(cat::wrap_shr((1_u4).raw, 1u) == 0x80000000u);
   cat::verify(cat::wrap_shr((0xDEADBEEF_u4).raw, 32u) == 0xDEADBEEFu);
   cat::verify(cat::wrap_shr((0x12345678_u4).raw, 4u) == 0x81234567u);
   cat::verify(cat::wrap_shr((0_u4).raw, 100u) == 0u);

   // uint8.
   static_assert(cat::wrap_shr((1ull << 63u), 63u) == 1ull);
   static_assert(cat::wrap_shr((1_u8).raw, 1u) == (1ull << 63u));
   static_assert(cat::wrap_shr((1_u8).raw, 64u) == 1ull);
   static_assert(cat::wrap_shr((1_u8).raw, 65u) == (1ull << 63u));
   static_assert(cat::wrap_shr((0_u8).raw, 100u) == 0u);

   cat::verify(cat::wrap_shr((1ull << 63u), 63u) == 1ull);
   cat::verify(cat::wrap_shr((1_u8).raw, 1u) == (1ull << 63u));
   cat::verify(cat::wrap_shr((1_u8).raw, 64u) == 1ull);
   cat::verify(cat::wrap_shr((1_u8).raw, 65u) == (1ull << 63u));
   cat::verify(cat::wrap_shr((0_u8).raw, 100u) == 0u);
}

test(arithmetic_wrap_shr_signed_fixed_width) {
   // Signed wrap shift operates on the raw bit pattern. Right-rotation moves
   // the low bit into the sign position, flipping sign when bit 0 is set.

   // int1.
   static_assert(cat::wrap_shr((-1_i1).raw, 3u) == (-1_i1).raw);
   // Sign bit rotates into bit 6, leaving a positive.
   static_assert(cat::wrap_shr(cat::int1_min.raw, 1u) == (0x40_i1).raw);
   // Bit 0 rotates into the sign position, flipping to negative.
   static_assert(cat::wrap_shr((1_i1).raw, 1u) == cat::int1_min.raw);
   static_assert(cat::wrap_shr((0_i1).raw, 100u) == 0);
   static_assert(cat::wrap_shr((-42_i1).raw, 0u) == (-42_i1).raw);
   static_assert(cat::wrap_shr((1_i1).raw, 8u) == (1_i1).raw);
   static_assert(cat::wrap_shr((1_i1).raw, 9u) == cat::int1_min.raw);

   cat::verify(cat::wrap_shr((-1_i1).raw, 3u) == (-1_i1).raw);
   cat::verify(cat::wrap_shr(cat::int1_min.raw, 1u) == (0x40_i1).raw);
   cat::verify(cat::wrap_shr((1_i1).raw, 1u) == cat::int1_min.raw);
   cat::verify(cat::wrap_shr((0_i1).raw, 100u) == 0);
   cat::verify(cat::wrap_shr((-42_i1).raw, 0u) == (-42_i1).raw);
   cat::verify(cat::wrap_shr((1_i1).raw, 8u) == (1_i1).raw);
   cat::verify(cat::wrap_shr((1_i1).raw, 9u) == cat::int1_min.raw);

   // Erasure overload preserves the LHS shape.
   static_assert(cat::wrap_shr(1_i1, 1u) == cat::int1_min);
   static_assert(cat::wrap_shr(cat::int1_min, 1u) == 0x40_i1);
   cat::verify(cat::wrap_shr(1_i1, 1u) == cat::int1_min);
   cat::verify(cat::wrap_shr(cat::int1_min, 1u) == 0x40_i1);

   // int2.
   static_assert(cat::wrap_shr((-1_i2).raw, 5u) == (-1_i2).raw);
   static_assert(cat::wrap_shr(cat::int2_min.raw, 1u) == (0x4000_i2).raw);
   static_assert(cat::wrap_shr((1_i2).raw, 1u) == cat::int2_min.raw);
   static_assert(cat::wrap_shr((0_i2).raw, 100u) == 0);

   cat::verify(cat::wrap_shr((-1_i2).raw, 5u) == (-1_i2).raw);
   cat::verify(cat::wrap_shr(cat::int2_min.raw, 1u) == (0x4000_i2).raw);
   cat::verify(cat::wrap_shr((1_i2).raw, 1u) == cat::int2_min.raw);
   cat::verify(cat::wrap_shr((0_i2).raw, 100u) == 0);

   // int4.
   static_assert(cat::wrap_shr((-1_i4).raw, 7u) == (-1_i4).raw);
   static_assert(cat::wrap_shr(cat::int4_min.raw, 1u) == (0x40000000_i4).raw);
   static_assert(cat::wrap_shr((1_i4).raw, 1u) == cat::int4_min.raw);
   static_assert(cat::wrap_shr((0_i4).raw, 100u) == 0);

   cat::verify(cat::wrap_shr((-1_i4).raw, 7u) == (-1_i4).raw);
   cat::verify(cat::wrap_shr(cat::int4_min.raw, 1u) == (0x40000000_i4).raw);
   cat::verify(cat::wrap_shr((1_i4).raw, 1u) == cat::int4_min.raw);
   cat::verify(cat::wrap_shr((0_i4).raw, 100u) == 0);

   // int8.
   static_assert(cat::wrap_shr((-1_i8).raw, 11u) == (-1_i8).raw);
   static_assert(cat::wrap_shr(cat::int8_min.raw, 1u) == (1ll << 62u));
   static_assert(cat::wrap_shr((1_i8).raw, 1u) == cat::int8_min.raw);
   static_assert(cat::wrap_shr((0_i8).raw, 100u) == 0);

   cat::verify(cat::wrap_shr((-1_i8).raw, 11u) == (-1_i8).raw);
   cat::verify(cat::wrap_shr(cat::int8_min.raw, 1u) == (1ll << 62u));
   cat::verify(cat::wrap_shr((1_i8).raw, 1u) == cat::int8_min.raw);
   cat::verify(cat::wrap_shr((0_i8).raw, 100u) == 0);
}

test(arithmetic_sat_shl_index) {
   // Saturating left shift of a `cat::index` clamps at `index::max()` (`2^63 -
   // 1`). The high bit (bit 63) must always stay zero, so any shift that would
   // set it saturates instead. `index` is effectively a 63-bit unsigned type:
   // bit 62 is the highest usable bit.
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;
   using sat_idx = cat::index<cat::overflow_policies::saturate>;
   constexpr auto high_bit = 1ull << 63u;
   constexpr auto bit_62 = 1ull << 62u;

   // In-range shifts behave like the C++ shift. Bit 62 is reachable.
   static_assert(cat::sat_shl(sat_idx{1}, 0u) == sat_idx{1});
   static_assert(cat::sat_shl(sat_idx{1}, 1u) == sat_idx{2});
   static_assert(cat::sat_shl(sat_idx{1}, 61u) == sat_idx{1ull << 61u});
   static_assert(cat::sat_shl(sat_idx{1}, 62u) == sat_idx{bit_62});
   static_assert(cat::sat_shl(wrap_idx{1}, 1u) == wrap_idx{2});

   // Every shift that would set bit 63 saturates to `idx_max`.
   static_assert(cat::sat_shl(sat_idx{1}, 63u) == cat::idx_max);
   static_assert(cat::sat_shl(sat_idx{2}, 62u) == cat::idx_max);
   static_assert(cat::sat_shl(sat_idx{3}, 62u) == cat::idx_max);
   static_assert(cat::sat_shl(sat_idx{bit_62}, 1u) == cat::idx_max);

   // Shifting `bit_62 - 1` (all 62 low bits set) by 1 just fills in bit 62
   // without overflowing into bit 63, so it does not saturate.
   static_assert(cat::sat_shl(sat_idx{bit_62 - 1u}, 1u)
                 == sat_idx{cat::idx_max.raw - 1u});

   // Shift counts >= bit_width fully saturate (any non-zero value lands at
   // `index::max()`).
   static_assert(cat::sat_shl(sat_idx{1}, 64u) == cat::idx_max);
   static_assert(cat::sat_shl(sat_idx{1}, 100u) == cat::idx_max);
   static_assert(cat::sat_shl(sat_idx{cat::idx_max}, 1u) == cat::idx_max);
   static_assert(cat::sat_shl(sat_idx{cat::idx_max}, 63u) == cat::idx_max);

   // `idx_max` unshifted is still `idx_max`.
   static_assert(cat::sat_shl(sat_idx{cat::idx_max}, 0u) == cat::idx_max);

   // Zero saturates to zero for any shift count.
   static_assert(cat::sat_shl(sat_idx{0}, 0u) == sat_idx{0});
   static_assert(cat::sat_shl(sat_idx{0}, 63u) == sat_idx{0});
   static_assert(cat::sat_shl(sat_idx{0}, 100u) == sat_idx{0});

   // The result NEVER has the high bit set, regardless of input or shift.
   static_assert((cat::sat_shl(sat_idx{1}, 63u).raw & high_bit) == 0u);
   static_assert((cat::sat_shl(sat_idx{1}, 64u).raw & high_bit) == 0u);
   static_assert((cat::sat_shl(sat_idx{1}, 100u).raw & high_bit) == 0u);
   static_assert((cat::sat_shl(sat_idx{bit_62}, 1u).raw & high_bit) == 0u);
   static_assert((cat::sat_shl(sat_idx{cat::idx_max}, 63u).raw & high_bit)
                 == 0u);

   // The saturation ceiling is exactly `idx_max`, never anything larger (would
   // violate the invariant) and never anything smaller (would waste range).
   static_assert(cat::sat_shl(sat_idx{1}, 63u).raw == cat::idx_max.raw);

   // Member `operator<<` goes through `sat_shl` and matches.
   static_assert((sat_idx{1} << 62u) == sat_idx{bit_62});
   static_assert((sat_idx{1} << 63u) == cat::idx_max);
   static_assert((sat_idx{2} << 62u) == cat::idx_max);
   static_assert((sat_idx{bit_62} << 1u) == cat::idx_max);

   cat::verify(cat::sat_shl(sat_idx{1}, 62u) == sat_idx{bit_62});
   cat::verify(cat::sat_shl(sat_idx{1}, 63u) == cat::idx_max);
   cat::verify(cat::sat_shl(sat_idx{2}, 62u) == cat::idx_max);
   cat::verify(cat::sat_shl(sat_idx{bit_62}, 1u) == cat::idx_max);
   cat::verify(cat::sat_shl(sat_idx{1}, 64u) == cat::idx_max);
   cat::verify(cat::sat_shl(sat_idx{cat::idx_max}, 1u) == cat::idx_max);
   cat::verify(cat::sat_shl(sat_idx{0}, 100u) == sat_idx{0});
   cat::verify((cat::sat_shl(sat_idx{1}, 100u).raw & high_bit) == 0u);
   cat::verify((cat::sat_shl(sat_idx{bit_62}, 1u).raw & high_bit) == 0u);
   cat::verify((sat_idx{bit_62} << 1u) == cat::idx_max);
}

test(arithmetic_sat_shr_index) {
   // Saturating right shift of a `cat::index`. The value is always non-
   // negative, so the signed arithmetic right shift cannot flip into the sign
   // and the result stays in range.
   using sat_idx = cat::index<cat::overflow_policies::saturate>;

   // In-range shifts behave like the C++ right shift.
   static_assert(cat::sat_shr(sat_idx{4}, 1u) == sat_idx{2});
   static_assert(cat::sat_shr(sat_idx{1ull << 62u}, 62u) == sat_idx{1});
   static_assert(cat::sat_shr(sat_idx{cat::idx_max}, 0u)
                 == sat_idx{cat::idx_max});

   // Shift counts >= bit_width clamp to zero (positive values).
   static_assert(cat::sat_shr(sat_idx{1}, 64u) == sat_idx{0});
   static_assert(cat::sat_shr(sat_idx{cat::idx_max}, 100u) == sat_idx{0});

   // Zero stays zero.
   static_assert(cat::sat_shr(sat_idx{0}, 5u) == sat_idx{0});

   cat::verify(cat::sat_shr(sat_idx{4}, 1u) == sat_idx{2});
   cat::verify(cat::sat_shr(sat_idx{1ull << 62u}, 62u) == sat_idx{1});
   cat::verify(cat::sat_shr(sat_idx{cat::idx_max}, 0u)
               == sat_idx{cat::idx_max});
   cat::verify(cat::sat_shr(sat_idx{1}, 64u) == sat_idx{0});
   cat::verify(cat::sat_shr(sat_idx{cat::idx_max}, 100u) == sat_idx{0});
   cat::verify(cat::sat_shr(sat_idx{0}, 5u) == sat_idx{0});
}

test(arithmetic_wrap_shl_index) {
   // Wrap left shift of a `cat::index` rotates over the lower `bit_width - 1`
   // bits (the usable index range). The high bit always stays zero, preserving
   // the `index` invariant. The shift count is taken modulo the ring width, so
   // any non-negative count is valid.
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;
   constexpr auto ring_bits = __SIZE_WIDTH__ - 1u;

   // Basic rotation within the ring.
   static_assert(cat::wrap_shl(wrap_idx{1}, 1u) == wrap_idx{2});
   static_assert(cat::wrap_shl(wrap_idx{1}, ring_bits - 1u)
                 == wrap_idx{1ull << (ring_bits - 1u)});

   // Bit `ring_bits - 1` rotates around to bit 0 -- NOT to bit 63.
   static_assert(cat::wrap_shl(wrap_idx{1ull << (ring_bits - 1u)}, 1u)
                 == wrap_idx{1});

   // Count taken modulo `ring_bits`, so rotating by `ring_bits` is a no-op (and
   // `ring_bits + 1` is the same as 1).
   static_assert(cat::wrap_shl(wrap_idx{1}, ring_bits) == wrap_idx{1});
   static_assert(cat::wrap_shl(wrap_idx{1}, ring_bits + 1u) == wrap_idx{2});

   // Large counts are handled via the modulo.
   static_assert(cat::wrap_shl(wrap_idx{1}, ring_bits * 2u) == wrap_idx{1});
   static_assert(cat::wrap_shl(wrap_idx{1}, 1'000u)
                 == wrap_idx{1ull << (1'000u % ring_bits)});

   // Zero count is a no-op.
   static_assert(cat::wrap_shl(wrap_idx{cat::idx_max}, 0u)
                 == wrap_idx{cat::idx_max});

   // Zero value stays zero for any count.
   static_assert(cat::wrap_shl(wrap_idx{0}, 100u) == wrap_idx{0});

   // All-ones-in-ring (`idx_max`) rotates to itself.
   static_assert(cat::wrap_shl(wrap_idx{cat::idx_max}, 5u)
                 == wrap_idx{cat::idx_max});
   static_assert(cat::wrap_shl(wrap_idx{cat::idx_max}, 1'000u)
                 == wrap_idx{cat::idx_max});

   // The result always has bit 63 cleared (index invariant).
   static_assert((cat::wrap_shl(wrap_idx{cat::idx_max}, 1u).raw & (1ull << 63u))
                 == 0u);
   static_assert((cat::wrap_shl(wrap_idx{1ull << 62u}, 3u).raw & (1ull << 63u))
                 == 0u);

   cat::verify(cat::wrap_shl(wrap_idx{1}, 1u) == wrap_idx{2});
   cat::verify(cat::wrap_shl(wrap_idx{1ull << (ring_bits - 1u)}, 1u)
               == wrap_idx{1});
   cat::verify(cat::wrap_shl(wrap_idx{1}, ring_bits) == wrap_idx{1});
   cat::verify(cat::wrap_shl(wrap_idx{1}, ring_bits + 1u) == wrap_idx{2});
   cat::verify(cat::wrap_shl(wrap_idx{1}, ring_bits * 2u) == wrap_idx{1});
   cat::verify(cat::wrap_shl(wrap_idx{cat::idx_max}, 0u)
               == wrap_idx{cat::idx_max});
   cat::verify(cat::wrap_shl(wrap_idx{0}, 100u) == wrap_idx{0});
   cat::verify(cat::wrap_shl(wrap_idx{cat::idx_max}, 5u)
               == wrap_idx{cat::idx_max});
   cat::verify((cat::wrap_shl(wrap_idx{cat::idx_max}, 1u).raw & (1ull << 63u))
               == 0u);
   cat::verify((cat::wrap_shl(wrap_idx{1ull << 62u}, 3u).raw & (1ull << 63u))
               == 0u);
}

test(arithmetic_wrap_shr_index) {
   // Wrap right shift of a `cat::index` mirrors `wrap_shl`: bits shifted off
   // the bottom re-enter at the top of the 63-bit ring (bit 62 being the top of
   // the usable range), never at bit 63.
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;
   constexpr auto ring_bits = __SIZE_WIDTH__ - 1u;

   // Basic rotation within the ring.
   static_assert(cat::wrap_shr(wrap_idx{4}, 1u) == wrap_idx{2});
   static_assert(
      cat::wrap_shr(wrap_idx{1ull << (ring_bits - 1u)}, ring_bits - 1u)
      == wrap_idx{1});

   // Bit 0 rotates around to bit `ring_bits - 1` -- NOT to bit 63.
   static_assert(cat::wrap_shr(wrap_idx{1}, 1u)
                 == wrap_idx{1ull << (ring_bits - 1u)});

   // Count taken modulo `ring_bits`.
   static_assert(cat::wrap_shr(wrap_idx{1}, ring_bits) == wrap_idx{1});
   static_assert(cat::wrap_shr(wrap_idx{2}, ring_bits + 1u) == wrap_idx{1});

   // Inverse property: rotating left then right restores the value.
   static_assert(cat::wrap_shr(cat::wrap_shl(wrap_idx{0x1234}, 7u), 7u)
                 == wrap_idx{0x1234});

   // Zero count is a no-op.
   static_assert(cat::wrap_shr(wrap_idx{cat::idx_max}, 0u)
                 == wrap_idx{cat::idx_max});

   // Zero value stays zero.
   static_assert(cat::wrap_shr(wrap_idx{0}, 100u) == wrap_idx{0});

   // All-ones-in-ring rotates to itself.
   static_assert(cat::wrap_shr(wrap_idx{cat::idx_max}, 5u)
                 == wrap_idx{cat::idx_max});

   // Result always has bit 63 cleared.
   static_assert((cat::wrap_shr(wrap_idx{1}, 1u).raw & (1ull << 63u)) == 0u);
   static_assert((cat::wrap_shr(wrap_idx{cat::idx_max}, 1u).raw & (1ull << 63u))
                 == 0u);

   cat::verify(cat::wrap_shr(wrap_idx{4}, 1u) == wrap_idx{2});
   cat::verify(cat::wrap_shr(wrap_idx{1}, 1u)
               == wrap_idx{1ull << (ring_bits - 1u)});
   cat::verify(cat::wrap_shr(wrap_idx{1}, ring_bits) == wrap_idx{1});
   cat::verify(cat::wrap_shr(cat::wrap_shl(wrap_idx{0x1234}, 7u), 7u)
               == wrap_idx{0x1234});
   cat::verify(cat::wrap_shr(wrap_idx{0}, 100u) == wrap_idx{0});
   cat::verify(cat::wrap_shr(wrap_idx{cat::idx_max}, 5u)
               == wrap_idx{cat::idx_max});
   cat::verify((cat::wrap_shr(wrap_idx{1}, 1u).raw & (1ull << 63u)) == 0u);
   cat::verify((cat::wrap_shr(wrap_idx{cat::idx_max}, 1u).raw & (1ull << 63u))
               == 0u);
}

test(arithmetic_wrap_add_mul_and_overflow_reference_constexpr) {
   // Test wrapping arithmetic operations.
   static_assert(cat::wrap_add(0, 1) == 1);
   static_assert(cat::wrap_add(cat::int4_max - 1, 1) == cat::int4_max);
   static_assert(cat::wrap_add(cat::int4_max, 1) == cat::int4_min);
   static_assert(cat::wrap_add(cat::int4_max, 2) == cat::int4_min + 1);

   static_assert(cat::wrap_add(0u, 1u) == 1u);
   static_assert(cat::wrap_add(cat::uint4_max - 1u, 1u) == cat::uint4_max);
   static_assert(cat::wrap_add(cat::uint4_max, 1u) == cat::uint4_min);
   static_assert(cat::wrap_add(cat::uint4_max, 2u) == cat::uint4_min + 1u);

   static_assert(cat::wrap_mul(0, 1) == 0);
   static_assert(cat::wrap_mul(cat::int4_max, 1) == cat::int4_max);
   static_assert(cat::wrap_mul(cat::int4_max, 2) == -2);

   static_assert(cat::wrap_mul(0u, 1u) == 0u);
   static_assert(cat::wrap_mul(cat::uint4_max, 1u) == cat::uint4_max);
   static_assert(cat::wrap_mul(cat::uint4_max, 2u) == cat::uint4_max - 1u);

   // Overflow reference views (`wrap()`, `sat()`, ...) use `wrap_add`,
   // `sat_add`, and related helpers in `constexpr` the same way as at runtime.
   static_assert((cat::int4_max.wrap() + 100) == cat::int4_min + 99);
   static_assert((cat::int4_max.sat() + 100) == cat::int4_max);
   static_assert((cat::int4_min.wrap() - 1) == cat::int4_max);
   static_assert((cat::uint4_max.wrap() + 1u) == cat::uint4_min);
   static_assert((cat::int4_max.sat() + 1) == cat::int4_max);
   static_assert((cat::int4_min.sat() - 1) == cat::int4_min);
}

test(
   arithmetic_promotion_hierarchy_overflow_semantics_and_strong_overflow_types) {
   // Test overflow semantics.
   int4 safe_int = int4::max();
   cat::verify((safe_int.wrap() + 100) == cat::int4_min + 99);

   safe_int.wrap() += 1;
   cat::verify(safe_int == cat::int4_min);
   safe_int.wrap() += 100;
   cat::verify(safe_int.wrap() == cat::int4_min + 100);
   cat::verify(safe_int.sat() == cat::int4_min + 100);
   cat::verify(safe_int.raw == cat::int4_min + 100);

   // Test saturating overflow with member access syntax.
   safe_int = int4::max();
   cat::verify((safe_int.sat() + 100) == cat::int4_max);

   safe_int.sat() += 1;
   cat::verify(safe_int == cat::int4_max);
   safe_int.sat() += 100;
   cat::verify(safe_int.wrap() == cat::int4_max);
   cat::verify(safe_int.sat() == cat::int4_max);
   cat::verify(safe_int.raw == cat::int4_max);

   // Test overflow strong types.
   cat::wrap_int4 wrap_int4 = cat::int4_max;
   wrap_int4 += 100;
   cat::verify(wrap_int4 == cat::int4_min + 99);
   wrap_int4 = cat::int4_max;
   wrap_int4.sat() += 100;
   cat::verify(wrap_int4 == cat::int4_max);

   cat::sat_int4 saturate_int4 = cat::int4_max;
   saturate_int4 += 100;
   cat::verify(saturate_int4 == cat::int4_max);
   saturate_int4 = cat::int4_max;
   saturate_int4.wrap() += 100;
   cat::verify(saturate_int4 == cat::int4_min + 99);
}

// Binary arithmetic operators (`+`, `-`, `*`, `/`, `&`, ...) always take the
// left operand's overflow policy. The right operand's policy is irrelevant
// because the operation is performed under the left operand's overflow
// semantics, and compound assignment operators rely on this rule to assign the
// result back into the left operand.
test(arithmetic_binary_ops_take_lhs_overflow_policy) {
   using cat::is_same;
   using wrap_uword =
      cat::arithmetic<__UINT64_TYPE__, cat::overflow_policies::wrap>;
   using sat_iword =
      cat::arithmetic<__INT64_TYPE__, cat::overflow_policies::saturate>;
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;

   // Same-width pairs preserve LHS's policy regardless of RHS's policy.
   static_assert(
      is_same<decltype(cat::wrap_int4() + cat::int4()), cat::wrap_int4>);
   static_assert(
      is_same<decltype(cat::wrap_int4() + cat::sat_int4()), cat::wrap_int4>);
   static_assert(
      is_same<decltype(cat::sat_int4() + cat::wrap_int4()), cat::sat_int4>);
   static_assert(
      is_same<decltype(cat::wrap_int4() - cat::sat_int4()), cat::wrap_int4>);
   static_assert(
      is_same<decltype(cat::sat_int4() - cat::wrap_int4()), cat::sat_int4>);

   // `wrap` / `saturate` LHS NEVER widens, even on `+` / `*`: the user opted
   // into a fixed-width well-defined overflow domain, and silently growing into
   // a wider type would defeat that intent.
   static_assert(is_same<decltype(cat::wrap_int4() + iword()), cat::wrap_int4>);
   static_assert(is_same<decltype(cat::sat_int4() + iword()), cat::sat_int4>);
   static_assert(is_same<decltype(cat::wrap_int4() * iword()), cat::wrap_int4>);

   // `-`, `/`, `%`, `<<`, and `>>` likewise keep the wrap/sat LHS's storage.
   static_assert(is_same<decltype(cat::wrap_int4() - iword()), cat::wrap_int4>);
   static_assert(is_same<decltype(cat::sat_int4() / iword()), cat::sat_int4>);
   static_assert(
      is_same<decltype(cat::wrap_uint4() % cat::uword()), cat::wrap_uint4>);

   // When the LHS is wider, the result is the LHS's exact type.
   static_assert(is_same<decltype(wrap_uword() + cat::uint4()), wrap_uword>);
   static_assert(is_same<decltype(wrap_uword() - cat::uint4()), wrap_uword>);
   static_assert(is_same<decltype(sat_iword() + cat::int4()), sat_iword>);

   // `index` keeps its kind on either side of the width comparison: the result
   // is always an `index<P>`, where `P` is the LHS's policy.
   static_assert(is_same<decltype(wrap_idx() + cat::uint4()), wrap_idx>);
   // RHS's wrap policy is dropped because LHS (`uint4`) has undefined policy.
   // The wider index-shape is preserved.
   static_assert(is_same<decltype(cat::uint4() + wrap_idx()), idx>);
   // `wrap_idx` never promotes either: stay at the index-shape of the LHS.
   static_assert(is_same<decltype(wrap_idx() + 0ul), wrap_idx>);

   // Bitwise `&` / `|` keep the LHS's type (and therefore the LHS's policy). A
   // narrower RHS zero-extends into the LHS width, but a wider RHS would have
   // to silently truncate so it is rejected.
   static_assert(
      is_same<decltype(cat::wrap_uint4() & cat::uint4()), cat::wrap_uint4>);
   static_assert(
      is_same<decltype(cat::sat_uint4() | cat::wrap_uint4()), cat::sat_uint4>);

   // The runtime semantics still come from the LHS.
   {
      cat::wrap_int4 lhs = cat::int4_max;
      cat::sat_int4 rhs = 1;
      cat::wrap_int4 result = lhs + rhs;
      cat::verify(result == cat::int4_min);  // wrap, not saturate.
   }
   {
      cat::sat_int4 lhs = cat::int4_max;
      cat::wrap_int4 rhs = 1;
      cat::sat_int4 result = lhs + rhs;
      cat::verify(result == cat::int4_max);  // saturate, not wrap.
   }
   {
      cat::wrap_int4 lhs = cat::int4_min;
      cat::sat_int4 rhs = 1;
      cat::wrap_int4 result = lhs - rhs;
      cat::verify(result == cat::int4_max);  // wrap.
   }
   {
      cat::sat_int4 lhs = cat::int4_min;
      cat::wrap_int4 rhs = 1;
      cat::sat_int4 result = lhs - rhs;
      cat::verify(result == cat::int4_min);  // saturate.
   }
}

// `+` and `*` promote width to the wider operand (so `narrow + wide` returns
// the wider type), but `-`, `/`, `%`, `<<`, `>>`, `&`, and `|` keep the LHS's
// storage width because their results are bounded by the LHS by construction.
// `&` and `|` additionally reject a wider RHS because it would have to silently
// truncate to fit the LHS-shape result.
test(arithmetic_binary_ops_width_rules) {
   using cat::is_same;

   // Width is promoted on `+` and `*`.
   static_assert(is_same<decltype(cat::int4() + cat::iword()), cat::iword>);
   static_assert(is_same<decltype(cat::iword() + cat::int4()), cat::iword>);
   static_assert(is_same<decltype(cat::int4() * cat::iword()), cat::iword>);
   static_assert(is_same<decltype(cat::iword() * cat::int4()), cat::iword>);

   // Signed undefined `-` widens to a wider RHS so an underflow stays
   // representable. Unsigned undefined `-` and same-or-narrower RHS keep the
   // LHS shape.
   static_assert(is_same<decltype(cat::int4() - cat::iword()), cat::iword>);
   static_assert(is_same<decltype(cat::iword() - cat::int4()), cat::iword>);
   static_assert(is_same<decltype(cat::int4() / cat::iword()), cat::int4>);
   static_assert(is_same<decltype(cat::iword() / cat::int4()), cat::iword>);
   static_assert(is_same<decltype(cat::uint4() % cat::uword()), cat::uint4>);
   static_assert(is_same<decltype(cat::uword() % cat::uint4()), cat::uword>);
   static_assert(is_same<decltype(cat::uint4() << cat::uword()), cat::uint4>);
   static_assert(is_same<decltype(cat::uword() >> cat::uint4()), cat::uword>);

   // `&` / `|` accept any RHS that is at most as wide as the LHS. The RHS
   // zero-extends into the LHS width and the result keeps the LHS's type.
   static_assert(is_same<decltype(cat::uint4() & cat::uint4()), cat::uint4>);
   static_assert(is_same<decltype(cat::uword() | cat::uword()), cat::uword>);
   static_assert(is_same<decltype(cat::uword() & cat::uint4()), cat::uword>);
   static_assert(is_same<decltype(cat::uword() | cat::uint4()), cat::uword>);

   // Narrower raw integers also widen on the friend reverse operator, so a raw
   // `unsigned char` (or any narrower raw unsigned) `&` `cat::uword` works
   // without an explicit cast.
   static_assert(is_same<decltype(static_cast<unsigned char>(0) & cat::uword()),
                         cat::uword>);
   static_assert(is_same<decltype(cat::uword() & static_cast<unsigned char>(0)),
                         cat::uword>);

   // The narrow-LHS direction would have to silently truncate the wider RHS to
   // fit the LHS-shape result, so it stays rejected at the `bit_and` / `bit_or`
   // member level.
   static_assert(!cat::detail::has_binary_bit_and<cat::uint4, cat::uword>);
   static_assert(!cat::detail::has_binary_bit_or<cat::uint4, cat::uword>);

   // `+=` / `-=` / `/=` / `%=` work when the result fits the LHS, which is
   // automatic given the LHS-shape rule above.
   {
      cat::int4 a = 100_i4;
      a -= cat::iword(50);
      cat::verify(a == 50_i4);
   }
   {
      cat::iword a = 100_i8;
      a /= cat::int4(4);
      cat::verify(a == 25_i8);
   }
   {
      cat::uint4 a = 100_u4;
      a %= cat::uword(7u);
      cat::verify(a == 2_u4);
   }
}

// Compound assignment operators must yield the same unqualified type as the
// corresponding non-compound operator.
test(arithmetic_compound_ops_match_non_compound_return_type) {
   using cat::is_same;
   using cat::remove_cvref;
   using wrap_iword =
      cat::arithmetic<__INT64_TYPE__, cat::overflow_policies::wrap>;
   using sat_iword =
      cat::arithmetic<__INT64_TYPE__, cat::overflow_policies::saturate>;
   using wrap_uword =
      cat::arithmetic<__UINT64_TYPE__, cat::overflow_policies::wrap>;
   using sat_uword =
      cat::arithmetic<__UINT64_TYPE__, cat::overflow_policies::saturate>;
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;
   using sat_idx = cat::index<cat::overflow_policies::saturate>;

   // Same-width pairs with mixed policies. `+=`, `-=`, `*=`, `/=`, `%=` all
   // keep the LHS's policy because that's exactly what the binary form returns.
   {
      cat::wrap_int4 a;
      cat::sat_int4 b;
      static_assert(is_same<remove_cvref<decltype(a += b)>, decltype(a + b)>);
      static_assert(is_same<remove_cvref<decltype(a -= b)>, decltype(a - b)>);
      static_assert(is_same<remove_cvref<decltype(a *= b)>, decltype(a * b)>);
      static_assert(is_same<remove_cvref<decltype(a /= b)>, decltype(a / b)>);
   }
   {
      cat::sat_uint4 a;
      cat::wrap_uint4 b;
      static_assert(is_same<remove_cvref<decltype(a += b)>, decltype(a + b)>);
      static_assert(is_same<remove_cvref<decltype(a -= b)>, decltype(a - b)>);
      static_assert(is_same<remove_cvref<decltype(a *= b)>, decltype(a * b)>);
      static_assert(is_same<remove_cvref<decltype(a /= b)>, decltype(a / b)>);
      static_assert(is_same<remove_cvref<decltype(a %= b)>, decltype(a % b)>);
   }

   // LHS wider than RHS. `+=` and `*=` are still defined here because
   // `promoted_type` widens to the LHS, so the binary result is also LHS-shape.
   {
      wrap_iword a;
      cat::int4 b;
      static_assert(is_same<remove_cvref<decltype(a += b)>, decltype(a + b)>);
      static_assert(is_same<remove_cvref<decltype(a -= b)>, decltype(a - b)>);
      static_assert(is_same<remove_cvref<decltype(a *= b)>, decltype(a * b)>);
      static_assert(is_same<remove_cvref<decltype(a /= b)>, decltype(a / b)>);
   }
   {
      sat_iword a;
      cat::wrap_int4 b;
      static_assert(is_same<remove_cvref<decltype(a += b)>, decltype(a + b)>);
      static_assert(is_same<remove_cvref<decltype(a *= b)>, decltype(a * b)>);
   }

   // RHS wider than LHS. `-=`, `/=`, `%=` stay LHS-shape so they are still
   // defined. `+=` / `*=` would silently truncate the wider binary result and
   // are correctly rejected entirely (so they are not asserted here).
   {
      cat::wrap_int4 a;
      cat::iword b;
      static_assert(is_same<remove_cvref<decltype(a -= b)>, decltype(a - b)>);
      static_assert(is_same<remove_cvref<decltype(a /= b)>, decltype(a / b)>);
   }
   {
      cat::sat_uint4 a;
      cat::uword b;
      static_assert(is_same<remove_cvref<decltype(a -= b)>, decltype(a - b)>);
      static_assert(is_same<remove_cvref<decltype(a /= b)>, decltype(a / b)>);
      static_assert(is_same<remove_cvref<decltype(a %= b)>, decltype(a % b)>);
   }

   // Bitwise `&=` / `|=` keep the LHS's policy whether the RHS is the same
   // width or narrower (the narrower RHS zero-extends).
   {
      cat::wrap_uint4 a;
      cat::sat_uint4 b;
      static_assert(is_same<remove_cvref<decltype(a &= b)>, decltype(a & b)>);
      static_assert(is_same<remove_cvref<decltype(a |= b)>, decltype(a | b)>);
   }
   {
      sat_uword a;
      cat::wrap_uint4 b;
      static_assert(is_same<remove_cvref<decltype(a &= b)>, decltype(a & b)>);
      static_assert(is_same<remove_cvref<decltype(a |= b)>, decltype(a | b)>);
   }

   // Shifts always return the LHS's shape regardless of the bit-count operand's
   // type or policy. Cover unsigned, signed, and `index` LHSes.
   {
      cat::sat_uint4 a;
      wrap_uword b;
      static_assert(is_same<remove_cvref<decltype(a <<= b)>, decltype(a << b)>);
      static_assert(is_same<remove_cvref<decltype(a >>= b)>, decltype(a >> b)>);
   }
   {
      cat::wrap_int4 a;
      cat::sat_uint4 b;
      static_assert(is_same<remove_cvref<decltype(a <<= b)>, decltype(a << b)>);
      static_assert(is_same<remove_cvref<decltype(a >>= b)>, decltype(a >> b)>);
   }
   {
      sat_idx a;
      cat::uword b;
      static_assert(is_same<remove_cvref<decltype(a <<= b)>, decltype(a << b)>);
      static_assert(is_same<remove_cvref<decltype(a >>= b)>, decltype(a >> b)>);
   }
   {
      wrap_idx a;
      cat::uint4 b;
      static_assert(is_same<remove_cvref<decltype(a <<= b)>, decltype(a << b)>);
      static_assert(is_same<remove_cvref<decltype(a >>= b)>, decltype(a >> b)>);
   }
}

// Signed `cat::arithmetic` and `cat::idx` both expose `<<` / `>>`. Signed
// shifts use the underlying C++ arithmetic shift (sign-extending `>>`), and
// `idx` shifts go through a signed-bitcast trick because `idx` always keeps its
// high bit zero.
test(arithmetic_signed_and_idx_arithmetic_shift) {
   using cat::is_same;

   // Signed `>>` is the arithmetic right shift: it sign-extends a negative
   // value rather than shifting in zeros like the unsigned version would.
   {
      int4 negative = -8_i4;
      cat::verify((negative >> 2_u4) == -2_i4);
      cat::verify((negative >> uword(1u)) == -4_i4);
      cat::verify((iword(-1'024) >> 4_u4) == -64_i8);
      cat::verify((int1(-16) >> uint4(2u)) == -4_i1);
   }

   // Signed `<<` for non-negative operands matches the unsigned shift.
   {
      int4 a = 3_i4;
      cat::verify((a << 2_u4) == 12_i4);
      iword b = 1_i8;
      cat::verify((b << uword(10u)) == 1'024_i8);
      // Compile-time integer constants on the RHS of a signed shift.
      cat::verify((a << 1) == 6_i4);
      cat::verify((a >> 1) == 1_i4);
   }

   // `<<=` / `>>=` work on signed `arithmetic`. The result preserves the LHS
   // type (and therefore its overflow policy).
   {
      int4 a = 5_i4;
      a <<= 2_u4;
      cat::verify(a == 20_i4);
      a >>= 1_u4;
      cat::verify(a == 10_i4);
   }

   // `idx` shifts. `idx >> n` is logically the same as the unsigned shift (idx
   // has a zero high bit), but it is also exposed as the *arithmetic* shift
   // through the signed bitcast trick.
   {
      idx i = 0b1100_idx;
      cat::verify((i >> 2_u4) == 0b11_idx);
      cat::verify((i << 2_u4) == 0b11'0000_idx);
      cat::verify((i >> 1) == 0b110_idx);
      cat::verify((i << uword(3u)) == 0b110'0000_idx);
   }

   {
      idx i = 1_idx;
      i <<= 4_u4;
      cat::verify(i == 16_idx);
      i >>= 1_u4;
      cat::verify(i == 8_idx);
   }

   // Width keeps the LHS type for both signed and `idx` shifts. Shift operators
   // on signed `arithmetic` pick the LHS storage just like the unsigned ones.
   // `idx` shifts always come back as `idx`.
   static_assert(is_same<decltype(int4() << uint4()), int4>);
   static_assert(is_same<decltype(int4() >> uint4()), int4>);
   static_assert(is_same<decltype(int4() << uword()), int4>);
   static_assert(is_same<decltype(iword() << uint4()), iword>);
   static_assert(is_same<decltype(iword() >> uint4()), iword>);
   static_assert(is_same<decltype(idx() << uint4()), idx>);
   static_assert(is_same<decltype(idx() >> uint4()), idx>);
   static_assert(is_same<decltype(idx() << uword()), idx>);
   static_assert(is_same<decltype(idx() >> uword()), idx>);

   // Cross-policy still propagates the LHS policy on signed / `idx` shifts.
   static_assert(
      is_same<decltype(cat::wrap_int4() << uint4()), cat::wrap_int4>);
   static_assert(
      is_same<decltype(cat::wrap_int4() >> uint4()), cat::wrap_int4>);

   // Same-type signed-by-signed compiles now: the LHS dispatches into
   // `shift_left_by(int4)` with no signedness constraint on `U`. The RHS is
   // expected to be a non-negative count even though the type allows a sign bit
   // (mirroring the C++ standard rule for `<<`).
   {
      int4 a = 1_i4;
      cat::verify((a << int4(3)) == 8_i4);
      cat::verify((int4(8) >> int4(2)) == 2_i4);
   }

   // `_by` and `_into` are the two dispatch sides of the same `<<` / `>>`
   // operator (LHS and RHS respectively), so signed `arithmetic` exposes both.
   // That lets a raw integer or any non-`arithmetic` value sit on the LHS with
   // a signed count on the RHS, just like the unsigned case.
   static_assert(cat::detail::has_reverse_shift_left<int4, unsigned>);
   static_assert(cat::detail::has_reverse_shift_right<int4, unsigned>);
   static_assert(cat::detail::has_reverse_shift_left<idx, unsigned>);
   static_assert(cat::detail::has_reverse_shift_right<idx, unsigned>);
   {
      unsigned int value = 1u;
      cat::verify((value << int4(3)) == 8u);
      cat::verify((value << idx(5)) == 32u);
      uword wide = 1_uz;
      cat::verify((wide << idx(5)) == 32_uz);
      cat::verify((wide << iword(3)) == 8_uz);
      cat::verify((idx(0b1100) << uword(2u)) == 0b11'0000_idx);
   }
}

// Saturating shifts. With `saturate` policy, `<<` clamps to the type's `max()`
// / `min()` instead of pushing bits past the sign / high bit, and `>>` only
// adds a guard against shift counts at or above the bit width (the C++ shift
// would otherwise be undefined).
test(arithmetic_saturating_shift) {
   using cat::is_same;
   using sat_idx = cat::index<cat::overflow_policies::saturate>;

   // Unsigned `<<` saturating: shifting a high bit out of the type clamps to
   // `max()` rather than dropping bits.
   {
      sat_uint4 a = 0xff000000u;
      cat::verify((a << 1_u4) == sat_uint4(cat::uint4_max));
      cat::verify((a << 8_u4) == sat_uint4(cat::uint4_max));
      sat_uint1 small = 0x40u;
      cat::verify((small << 1_u4) == sat_uint1(0x80u));
      cat::verify((small << 2_u4) == sat_uint1(0xffu));
      cat::verify((small << 8_u4) == sat_uint1(0xffu));
      cat::verify((sat_uint4(0u) << 31_u4) == sat_uint4(0u));
   }

   // Unsigned `>>` saturating: shifts past the value's leading 1 clamp to `0`.
   // This is the same value the unsigned arithmetic shift would converge to,
   // but explicit so the count >= width case stays defined.
   {
      sat_uint4 a = sat_uint4(0xff000000u);
      cat::verify((a >> 24_u4) == sat_uint4(0xffu));
      cat::verify((a >> 32_u4) == sat_uint4(0u));
      cat::verify((sat_uint4(1u) >> 1_u4) == sat_uint4(0u));
   }

   // Signed `<<` saturating: positive overflow clamps to `max()`, negative
   // overflow clamps to `min()`. Compare against the wrapping policy as a
   // sanity check on the same input.
   {
      sat_int4 a = sat_int4(0x40000000);
      cat::verify((a << 1_u4) == sat_int4(cat::int4_max));
      cat::verify((a << 8_u4) == sat_int4(cat::int4_max));
      cat::verify((sat_int4(3) << 2_u4) == sat_int4(12));
      cat::verify((sat_int4(0) << 31_u4) == sat_int4(0));
      cat::verify((sat_int4(1) << 31_u4) == sat_int4(cat::int4_max));
      cat::verify((sat_int4(1) << 32_u4) == sat_int4(cat::int4_max));
   }
   {
      sat_int4 n = sat_int4(-0x40000000);
      cat::verify((n << 1_u4) == sat_int4(cat::int4_min));
      cat::verify((n << 2_u4) == sat_int4(cat::int4_min));
      cat::verify((sat_int4(-1) << 31_u4) == sat_int4(cat::int4_min));
      cat::verify((sat_int4(-2) << 30_u4) == sat_int4(cat::int4_min));
      cat::verify((sat_int4(-2) << 31_u4) == sat_int4(cat::int4_min));
      cat::verify((sat_int4(-3) << 1_u4) == sat_int4(-6));
      cat::verify((sat_int4(cat::int4_min) << 1_u4) == sat_int4(cat::int4_min));
   }

   // Signed `>>` saturating: arithmetic right shift never overflows, so
   // saturate matches the regular arithmetic shift for in-range counts. For
   // counts at or above the bit width (the C++-undefined regime), positives
   // clamp to `0` and negatives clamp to `-1`.
   {
      cat::verify((sat_int4(-8) >> 2_u4) == sat_int4(-2));
      cat::verify((sat_int4(0x40) >> 4_u4) == sat_int4(4));
      cat::verify((sat_int4(0x40) >> 32_u4) == sat_int4(0));
      cat::verify((sat_int4(-0x40) >> 32_u4) == sat_int4(-1));
      cat::verify((sat_int4(-1) >> 32_u4) == sat_int4(-1));
   }

   // `idx` saturating `<<` clamps to `idx::max()` rather than pushing a bit
   // into the sign position (which would break the high-bit-zero invariant
   // under the default `undefined` policy).
   {
      sat_idx i = sat_idx(1_idx);
      // `sat_idx::max()` is the largest representable index, with the high bit
      // still zero: `(1 << (bits - 1)) - 1`.
      auto const idx_max = sat_idx(cat::limits<idx>::max());
      cat::verify((i << 4_u4) == sat_idx(16_idx));
      // Shifting a 1 all the way to bit `bits - 1` would set the high bit. The
      // `sat_idx` clamps to its `max()` instead.
      cat::verify((i << cat::limits<idx>::bits - 1) == idx_max);
      cat::verify((i << cat::limits<idx>::bits) == idx_max);
      cat::verify((sat_idx(0_idx) << 1_u4) == sat_idx(0_idx));
   }
   // `idx` saturating `>>` matches the regular arithmetic shift for in-range
   // counts and clamps to `0` for counts at or above the width.
   {
      sat_idx i = sat_idx(0b1100_idx);
      cat::verify((i >> 2_u4) == sat_idx(0b11_idx));
      cat::verify((i >> cat::limits<idx>::bits) == sat_idx(0_idx));
   }

   // Width and policy propagation: a saturating shift returns the LHS's exact
   // policy and width, just like the other binary operators.
   static_assert(is_same<decltype(sat_uint4() << cat::uint4()), sat_uint4>);
   static_assert(is_same<decltype(sat_uint4() >> cat::uint4()), sat_uint4>);
   static_assert(is_same<decltype(sat_int4() << cat::uint4()), sat_int4>);
   static_assert(is_same<decltype(sat_int4() >> cat::uint4()), sat_int4>);
   static_assert(is_same<decltype(sat_idx() << cat::uint4()), sat_idx>);
   static_assert(is_same<decltype(sat_idx() >> cat::uint4()), sat_idx>);

   // Compound assignment runs through the same dispatch.
   {
      sat_int4 a = sat_int4(0x40000000);
      a <<= 2_u4;
      cat::verify(a == sat_int4(cat::int4_max));
      sat_int4 b = sat_int4(-0x40000000);
      b <<= 2_u4;
      cat::verify(b == sat_int4(cat::int4_min));
      sat_uint4 c = sat_uint4(0x80000000u);
      c <<= 1_u4;
      cat::verify(c == sat_uint4(cat::uint4_max));
   }
}

// Signed `arithmetic` `%` is the C++ truncated remainder: the sign of the
// result follows the dividend, and a mixed-sign RHS does not silently promote
// the LHS to unsigned. Without the explicit RHS narrowing in
// `arithmetic::modulo_by`, `int4(-7) % uint4(3)` would go through the usual
// arithmetic conversions, turn `-7` into a huge unsigned, and yield `0` instead
// of `-1`.
test(arithmetic_signed_modulo_is_remainder) {
   // Same-signed remainders. C++'s built-in `%` already gives the right answer
   // here. This just pins the contract.
   {
      int4 a = -7_i4;
      cat::verify((a % int4(3)) == -1_i4);
      cat::verify((int4(7) % int4(-3)) == 1_i4);
      cat::verify((int4(-7) % int4(-3)) == -1_i4);
      cat::verify((int1(-16) % int1(5)) == -1_i1);
      cat::verify((iword(-1'024) % iword(7)) == -2_i8);
   }

   // Cross-sign constant remainders. The RHS is unsigned but the dividend is
   // negative, so the result still follows the dividend's sign.
   {
      cat::verify((int4(-7) % uint4(3)) == -1_i4);
      cat::verify((int1(-16) % uint4(5)) == -1_i1);
      cat::verify((iword(-1'024) % uword(7u)) == -2_i8);
   }

   // Cross-sign remainders against compile-time constants (the common case -- a
   // literal `2` is `int`, but the user usually thinks of it as just "two",
   // regardless of LHS storage signedness).
   {
      constexpr int signed_two = 2;
      int const signed_const_two = 2;
      constexpr unsigned int unsigned_two = 2u;
      unsigned int const unsigned_const_two = 2u;

      cat::verify((int4(-7) % signed_two) == -1_i4);
      cat::verify((int4(-7) % signed_const_two) == -1_i4);
      cat::verify((int4(-7) % unsigned_two) == -1_i4);
      cat::verify((int4(-7) % unsigned_const_two) == -1_i4);
   }

   // `%=` keeps the LHS's signedness and still produces remainder.
   {
      int4 a = -7_i4;
      a %= uint4(3);
      cat::verify(a == -1_i4);

      int4 b = -7_i4;
      b %= 3u;
      cat::verify(b == -1_i4);
   }

   // The result type still matches the LHS storage / policy.
   static_assert(cat::is_same<decltype(int4() % uint4()), int4>);
   static_assert(cat::is_same<decltype(int4() % iword()), int4>);
   static_assert(cat::is_same<decltype(int1() % uint4()), int1>);
   static_assert(cat::is_same<decltype(iword() % uword()), iword>);
   static_assert(cat::is_same<decltype(int8() % uint4()), int8>);
   // Wrapping policy on the LHS propagates through the LHS-shaped result.
   static_assert(
      cat::is_same<decltype(cat::wrap_int4() % uint4()), cat::wrap_int4>);
}

// Narrowing into a saturating destination clamps to the destination's range,
// and narrowing into a wrapping destination uses two's-complement modular
// reduction. These conversions are well-defined at any value, so they are
// implicit -- choosing a `sat_*` / `wrap_*` type is itself an opt-in to these
// semantics. Compile-time `enable_if` for fitting values still dispatches to
// the safe constructor, and the `consteval` hard error for unfitting
// compile-time values stays in place.
test(arithmetic_saturating_and_wrapping_narrowing_casts) {
   using cat::is_same;

   // `cat::sat_cast` clamps raw integers into the destination's range,
   // including across signedness boundaries.
   static_assert(cat::sat_cast<int>(1L) == 1);
   static_assert(cat::sat_cast<__INT8_TYPE__>(1'000'000L) == 127);
   static_assert(cat::sat_cast<__INT8_TYPE__>(-1'000'000L) == -128);
   // Signed -> unsigned: negative clamps to 0, large clamps to max.
   static_assert(cat::sat_cast<__UINT8_TYPE__>(-50) == 0u);
   static_assert(cat::sat_cast<__UINT8_TYPE__>(1'000'000) == 255u);
   // Unsigned -> signed: any value above signed max clamps to signed max.
   static_assert(cat::sat_cast<__INT8_TYPE__>(1'000'000u) == 127);
   static_assert(cat::sat_cast<__INT32_TYPE__>(static_cast<unsigned int>(-1))
                 == cat::int4_max.raw);

   // Saturating implicit construction at runtime.
   {
      cat::iword runtime_wide = cat::int8_max;
      cat::sat_int4 saturated = runtime_wide;
      cat::verify(saturated == cat::int4_max);
   }
   {
      cat::iword runtime_wide = cat::int8_min;
      cat::sat_int4 saturated = runtime_wide;
      cat::verify(saturated == cat::int4_min);
   }
   // Cross-signedness saturation at runtime.
   {
      cat::iword runtime_negative = -50;
      cat::sat_uint1 saturated = runtime_negative;
      cat::verify(saturated == 0_u1);
   }
   {
      cat::uword runtime_huge = cat::uint8_max;
      cat::sat_int4 saturated = runtime_huge;
      cat::verify(saturated == cat::int4_max);
   }
   // In-range narrowings round-trip.
   {
      cat::iword runtime_small = 42;
      cat::sat_int4 saturated = runtime_small;
      cat::verify(saturated == 42_i4);
   }

   // Wrapping implicit construction at runtime: two's-complement modular
   // reduction. `4'294'967'296` is `2 ** 32`.
   {
      cat::iword runtime_wide = 4'294'967'301;
      cat::wrap_int4 wrapped = runtime_wide;
      cat::verify(wrapped == 5_i4);
   }
   {
      cat::iword runtime_wide = 8'589'934'592;  // `2 ** 33`.
      cat::wrap_uint4 wrapped = runtime_wide;
      cat::verify(wrapped == 0_u4);
   }

   // The compile-time `fits` path is unaffected: small literals construct into
   // a `sat_*` / `wrap_*` type via the implicit `enable_if` overload, which
   // uses plain `static_cast` because the value already fits.
   static_assert(cat::sat_int4(5) == 5);
   static_assert(cat::wrap_int4(5) == 5);

   // The runtime saturating constructor really is implicit (it would not
   // compile in a copy-init context if it were `explicit`).
   static_assert(is_same<decltype([] {
                            cat::iword w = 1;
                            cat::sat_int4 s = w;
                            return s;
                         }()),
                         cat::sat_int4>);
}

test(arithmetic_uword_mixed_word_operations) {
   // Test integers.
   uword test_uword;
   test_uword = 100u + 0_uz;
   test_uword = 100ull + 0_uz;
   test_uword = 100_u4 + 0_uz;
   test_uword = 100_u8 + 0_uz;

   test_uword = 0_uz + 100u;
   test_uword = 0_uz + 100ull;
   test_uword = 0_uz + 100_u4;
   test_uword = 0_uz + 100_u8;

   test_uword = 100u - 1_uz;
   test_uword = 100ull - 1_uz;
   test_uword = 100_u4 - 1_uz;
   test_uword = 100_u8 - 1_uz;

   test_uword = 200_uz - 100u;
   test_uword = 200_uz - 100ull;
   test_uword = 200_uz - 100_u4;
   test_uword = 200_uz - 100_u8;

   test_uword = 0u;
   test_uword |= 1u;
   cat::verify(test_uword == 1u);

   test_uword = 12_uz * 5u;
   cat::verify(test_uword == 60_uz);
   test_uword = 12_uz * 5ull;
   cat::verify(test_uword == 60_uz);
   test_uword = 12_uz * 5_u4;
   cat::verify(test_uword == 60_uz);
   test_uword = 12_uz * 5_u8;
   cat::verify(test_uword == 60_uz);

   test_uword = 5u * 12_uz;
   cat::verify(test_uword == 60_uz);
   test_uword = 5ull * 12_uz;
   cat::verify(test_uword == 60_uz);
   test_uword = 5_u4 * 12_uz;
   cat::verify(test_uword == 60_uz);
   test_uword = 5_u8 * 12_uz;
   cat::verify(test_uword == 60_uz);

   test_uword = 120_uz / 4u;
   cat::verify(test_uword == 30_uz);
   test_uword = 120_uz / 4ull;
   cat::verify(test_uword == 30_uz);
   test_uword = 120_uz / 4_u4;
   cat::verify(test_uword == 30_uz);
   test_uword = 120_uz / 4_u8;
   cat::verify(test_uword == 30_uz);

   test_uword = 120u / 4_uz;
   cat::verify(test_uword == 30_uz);
   test_uword = 120ull / 4_uz;
   cat::verify(test_uword == 30_uz);
   test_uword = 120_u4 / 4_uz;
   cat::verify(test_uword == 30_uz);
   test_uword = 120_u8 / 4_uz;
   cat::verify(test_uword == 30_uz);

   test_uword = 100_uz;
   test_uword *= 2u;
   cat::verify(test_uword == 200_uz);
   test_uword /= 8_u4;
   cat::verify(test_uword == 25_uz);

   test_uword = 0xFu;
   test_uword &= 3u;
   cat::verify(test_uword == 3u);
   test_uword |= 8_u4;
   cat::verify(test_uword == 11u);

   test_uword = 7_uz;
   ++test_uword;
   cat::verify(test_uword == 8_uz);
   test_uword++;
   cat::verify(test_uword == 9_uz);

   idx const word_idx = 42_idx;
   test_uword = word_idx;
   cat::verify(test_uword == 42_uz);
   test_uword = 10_uz + word_idx;
   cat::verify(test_uword == 52_uz);
   test_uword = word_idx + 10_uz;
   cat::verify(test_uword == 52_uz);

   test_uword = uword{100} + uintptr<void>{20};
   cat::verify(test_uword == 120_uz);
   test_uword = uintptr<void>{80} + 40_uz;
   cat::verify(test_uword == 120_uz);
}

test(arithmetic_float_construction_raw_assignment_and_bit_cast) {
   float4 safe_float = 0.f;
   safe_float = 2.f;
   cat::verify(safe_float == 2.f);
   safe_float.raw = 1.f;
   cat::verify(safe_float == 1.f);

   int const int_into_float_1 = -2'024;
   float4 from_int{int_into_float_1};
   cat::verify(from_int == static_cast<float>(int_into_float_1));
   cat::verify(float4(int_into_float_1).raw
               == static_cast<float>(int_into_float_1));
   int const int_into_float_2 = 16'777'216;
   from_int = float4{int_into_float_2};
   cat::verify(from_int == static_cast<float>(int_into_float_2));
   cat::verify(float4(int_into_float_2).raw
               == static_cast<float>(int_into_float_2));

   // Test bit-casts.
   cat::verify(__builtin_bit_cast(unsigned, 2_i4) == 2u);
}

test(arithmetic_float8_binary_operations_ordering_limits_and_float4_promotion) {
   float8 a = 3.5_f8;
   float8 b = 2_f8;
   cat::verify(a + b == 5.5_f8);
   cat::verify(a - b == 1.5_f8);
   cat::verify(a * b == 7._f8);
   cat::verify(a / b == 1.75_f8);

   cat::verify(a > b);
   cat::verify(a >= a);
   cat::verify((a <=> b) > 0);

   float8 widened = 1_f4;
   cat::verify(widened == 1._f8);

   float8 mixed = 1._f8 + 2._f4;
   cat::verify(mixed == 3._f8);
}

test(
   arithmetic_float4_binary_ops_raw_float_ordering_limits_make_signed_and_compound) {
   float4 x = 10_f4;
   float4 y = 4_f4;
   cat::verify(x + y == 14_f4);
   cat::verify(x - y == 6_f4);
   cat::verify(x * y == 40_f4);
   cat::verify(x / y == 2.5_f4);

   float4 const two = 2_f4;
   auto left = two + 3.f;
   cat::verify(left == 5.f);

   auto right = 3.f + two;
   cat::verify(right == 5.f);

   cat::verify((1_f4 <=> 2_f4) < 0);
   cat::verify(0.5_f4 < 1_f4);

   cat::verify(cat::make_signed(1._f4) == 1._f4);

   float4 acc = 1._f4;
   acc += 2._f4;
   cat::verify(acc == 3._f4);
   acc -= 0.5_f4;
   cat::verify(acc == 2.5_f4);
   acc *= 2._f4;
   cat::verify(acc == 5._f4);
   acc /= 4._f4;
   cat::verify(acc == 1.25_f4);
}

test(arithmetic_idx_operations_traits_and_word_conversions) {
   // Test `idx`.
   idx idx1;
   idx idx2 = 1;
   idx2 = 1;
   idx idx3 = 1u;
   idx3 = 1u;
   idx1 = idx2;

   idx1 + idx2;
   idx1 + 1;
   1 + idx1;
   1u + idx1;
   1ull + idx1;
   idx1 + 1;
   idx1 + 1u;
   idx1 + 1ull;
   1_uz + idx1;
   1_sz + idx1;
   idx1 + 1_uz;
   idx1 + 1_sz;
   idx1 += 1u;
   idx1++;
   ++idx1;
   idx1 += 1;

   idx1 - idx2;
   idx1 - 1;
   10 - idx1;
   10u - idx1;
   10ull - idx1;
   1_uz - idx1;
   1_sz - idx1;
   idx1 - 1_uz;
   idx1 - 1_sz;

   idx1* idx2;
   idx1 * 1;
   idx1 * 1ull;
   1 * idx1;
   1u * idx1;
   1ull * idx1;
   1_uz * idx1;
   1_sz * idx1;
   idx1 * 1_uz;
   idx1 * 1_sz;
   idx1 *= 1;

   idx1 / idx2;
   idx1 / 1;
   idx1 / 1ull;
   1 / idx1;
   1u / idx1;
   1ull / idx1;
   1_uz / idx1;
   1_sz / idx1;
   idx _ = idx1 / 1_uz;
   iword _ = idx1 / 1_sz;
   idx1 /= 1_uz;

   idx lesser_idx = 1;
   idx greater_idx = 2;

   cat::verify(lesser_idx == lesser_idx);
   cat::verify(lesser_idx < greater_idx);
   cat::verify(lesser_idx <= greater_idx);
   cat::verify(lesser_idx <= lesser_idx);

   cat::verify(greater_idx == greater_idx);
   cat::verify(greater_idx > lesser_idx);
   cat::verify(greater_idx >= lesser_idx);
   cat::verify(greater_idx >= greater_idx);

   iword index_iword = idx2;
   index_iword = idx2;
   uword index_uword = idx2;
   index_uword = idx2;

   uword add_uword_idx = 1_uz + idx2;
   add_uword_idx += idx2;
}

// Helpers for the semantics tests below. `requires { T{V}; }` inside a
// `static_assert` is a hard error if `T`'s constructor is disabled via
// `enable_if`, so the SFINAE check must be hidden behind a concept.
namespace semantics_helpers {
template <typename T, auto value>
concept can_brace_init = requires { T{value}; };

template <typename L, typename R>
concept can_plus_assign = requires(L lhs, R rhs) { lhs += rhs; };

template <typename L, typename R>
concept can_minus_assign = requires(L lhs, R rhs) { lhs -= rhs; };

template <typename L, typename R>
concept can_times_assign = requires(L lhs, R rhs) { lhs *= rhs; };
}  // namespace semantics_helpers

// Constructors implicitly accept sound (in-range) integer constants and reject
// unsound (out-of-range) constants. Non-constant values that may be unsound
// require an explicit cast.
test(arithmetic_semantics_constructor_rejects_unsound_constants) {
   using semantics_helpers::can_brace_init;

   constexpr int1 sound_small = 100;
   cat::verify(sound_small == 100);
   constexpr uint4 sound_unsigned = 42u;
   cat::verify(sound_unsigned == 42u);
   static_assert(can_brace_init<int1, 100>);
   static_assert(can_brace_init<uint4, 42u>);

   // Out-of-range integer constants must be REJECTED (no silent truncation).
   static_assert(!can_brace_init<int1, 1'000>);
   static_assert(!can_brace_init<uint1, -1>);
   static_assert(!can_brace_init<uint4, -1>);

   // Out-of-range constant via a `consteval` literal: hard error too.
   static_assert(!can_brace_init<int1, 200_i4>);

   // Runtime values that may be unsound require an explicit cast: implicit
   // conversion of any int to int1 is rejected, but explicit construction
   // works.
   static_assert(!cat::is_convertible<int4, int1>);
   static_assert(cat::is_constructible<int1, int4>);
}

// Implicit construction into wrap/sat types is well-defined regardless of the
// source value -- choosing wrap/sat is itself the opt-in to that semantics.
// Casting to an `undefined` type is unsafe and stays explicit.
test(arithmetic_semantics_implicit_construction_into_wrap_sat) {
   // Saturating: clamps to destination range.
   static_assert(sat_int1{cat::int4{500}} == sat_int1::max());
   static_assert(sat_int1{cat::int4{-500}} == sat_int1::min());
   static_assert(sat_int8{cat::uint8::max()} == sat_int8::max());
   static_assert(sat_uint1{cat::int4{-1}} == sat_uint1{0});
   static_assert(sat_uint4{cat::int4{-7}} == sat_uint4{0});

   // Wrapping: modular cast.
   static_assert(wrap_int1{cat::int4{128}} == wrap_int1::min());
   static_assert(wrap_uint1{cat::int4{256}} == wrap_uint1{0});
   static_assert(wrap_uint1{cat::int4{257}} == wrap_uint1{1});

   // Implicit construction for wrap / sat is allowed even from values that
   // would not be sound for `undefined`: choosing wrap/sat is itself opt-in.
   static_assert(cat::is_convertible<cat::int4, sat_int1>);
   static_assert(cat::is_convertible<cat::int4, wrap_int1>);
   static_assert(cat::is_convertible<cat::uint8, sat_uint1>);
   static_assert(cat::is_convertible<cat::uint8, wrap_uint1>);

   // `undefined` narrowing is still explicit.
   static_assert(!cat::is_convertible<cat::int4, int1>);
}

// Arithmetic propagates the LHS overflow policy through the result. `wrap` LHS
// plus `undefined` RHS yields a `wrap` result, etc.
test(arithmetic_semantics_overflow_policy_propagates_via_lhs) {
   // LHS policy wins.
   static_assert(
      cat::is_same<decltype(cat::wrap_int4{} + cat::int4{}), cat::wrap_int4>);
   static_assert(
      cat::is_same<decltype(cat::sat_int4{} + cat::int4{}), cat::sat_int4>);
   static_assert(
      cat::is_same<decltype(cat::int4{} + cat::wrap_int4{}), cat::int4>);
   static_assert(
      cat::is_same<decltype(cat::int4{} + cat::sat_int4{}), cat::int4>);

   // `-`, `*`, `&`, `|` likewise.
   static_assert(
      cat::is_same<decltype(cat::wrap_int4{} - cat::int4{}), cat::wrap_int4>);
   static_assert(
      cat::is_same<decltype(cat::sat_int4{} - cat::int4{}), cat::sat_int4>);
   static_assert(
      cat::is_same<decltype(cat::wrap_int4{} * cat::int4{}), cat::wrap_int4>);
   static_assert(
      cat::is_same<decltype(cat::sat_int4{} * cat::int4{}), cat::sat_int4>);
   static_assert(cat::is_same<decltype(cat::wrap_uint4{} & cat::uint4{}),
                              cat::wrap_uint4>);
   static_assert(
      cat::is_same<decltype(cat::sat_uint4{} | cat::uint4{}), cat::sat_uint4>);
}

// Promotion rules.
//   * `wrap` / `saturate` LHS NEVER promote -- the result keeps LHS shape,
//     regardless of RHS width.
//   * `undefined` LHS may promote to a wider RHS for `+` and `*`.
//   * `undefined` UNSIGNED `-` does NOT promote (LHS shape preserved).
//   * `undefined` SIGNED `-` may promote to a wider RHS so an underflowed
//     difference stays representable.
//   * `-`, `/`, `%`, shifts, `&`, `|` otherwise keep LHS shape.
test(arithmetic_semantics_promotion_rules) {
   // `undefined` LHS promotes to wider RHS for `+` and `*`.
   static_assert(cat::is_same<decltype(int1{} + int4{}), int4>);
   static_assert(cat::is_same<decltype(int1{} * int4{}), int4>);
   static_assert(cat::is_same<decltype(uint1{} + uint4{}), uint4>);
   static_assert(cat::is_same<decltype(uint1{} * uint4{}), uint4>);

   // `wrap` / `saturate` LHS NEVER promote, even when RHS is wider.
   static_assert(
      cat::is_same<decltype(cat::wrap_int1{} + int4{}), cat::wrap_int1>);
   static_assert(
      cat::is_same<decltype(cat::wrap_int1{} * int4{}), cat::wrap_int1>);
   static_assert(
      cat::is_same<decltype(cat::sat_int1{} + int4{}), cat::sat_int1>);
   static_assert(
      cat::is_same<decltype(cat::sat_int1{} * int4{}), cat::sat_int1>);
   static_assert(
      cat::is_same<decltype(cat::wrap_uint1{} + uint4{}), cat::wrap_uint1>);
   static_assert(
      cat::is_same<decltype(cat::sat_uint1{} + uint4{}), cat::sat_uint1>);

   // `wrap` / `saturate` `-` keeps LHS shape too.
   static_assert(
      cat::is_same<decltype(cat::wrap_int1{} - int4{}), cat::wrap_int1>);
   static_assert(
      cat::is_same<decltype(cat::sat_int1{} - int4{}), cat::sat_int1>);
   static_assert(
      cat::is_same<decltype(cat::wrap_uint1{} - uint4{}), cat::wrap_uint1>);
   static_assert(
      cat::is_same<decltype(cat::sat_uint1{} - uint4{}), cat::sat_uint1>);

   // `undefined` UNSIGNED `-` does NOT promote.
   static_assert(cat::is_same<decltype(uint1{} - uint4{}), uint1>);
   static_assert(cat::is_same<decltype(uint4{} - uint8{}), uint4>);

   // `undefined` SIGNED `-` DOES promote when the RHS is wider.
   static_assert(cat::is_same<decltype(int1{} - int4{}), int4>);
   static_assert(cat::is_same<decltype(int4{} - int8{}), int8>);
   static_assert(cat::is_same<decltype(int4{} - iword{}), iword>);

   // `-`, `/`, `%`, `&`, `|`, `<<`, `>>` still keep LHS shape when RHS is
   // narrower or same-width.
   static_assert(cat::is_same<decltype(int4{} - int1{}), int4>);
   static_assert(cat::is_same<decltype(int4{} / int1{}), int4>);
   static_assert(cat::is_same<decltype(int4{} % int1{}), int4>);
   static_assert(cat::is_same<decltype(uint4{} & uint1{}), uint4>);
   static_assert(cat::is_same<decltype(uint4{} | uint1{}), uint4>);
   static_assert(cat::is_same<decltype(uint4{} << uint1{}), uint4>);
   static_assert(cat::is_same<decltype(uint4{} >> uint1{}), uint4>);
}

// Compound assignment never silently narrows. The compound op is only available
// when the corresponding binary op preserves the LHS shape, so a promoted
// result on a narrow LHS is rejected. Sound implicit constructors from constant
// RHS are still allowed.
test(arithmetic_semantics_compound_assignment_rejects_narrowing) {
   using semantics_helpers::can_minus_assign;
   using semantics_helpers::can_plus_assign;
   using semantics_helpers::can_times_assign;

   // Same-shape compound: always allowed.
   static_assert(can_plus_assign<int4, int4>);
   static_assert(can_minus_assign<uint4, uint4>);
   static_assert(can_times_assign<cat::wrap_int4, cat::wrap_int4>);
   static_assert(can_plus_assign<cat::sat_int4, cat::sat_int4>);

   // Sound constants: implicit construction means `int4 += 5` is fine.
   static_assert(can_plus_assign<int4, int>);

   // Promoted forms (undefined `+` or `*`): rejected on narrow LHS.
   static_assert(!can_plus_assign<int1, int4>);
   static_assert(!can_times_assign<int1, int4>);
   static_assert(!can_plus_assign<uint1, uint4>);

   // `wrap` / `saturate` never promote, so wide RHS is allowed for them (the
   // binary op preserves LHS shape).
   static_assert(can_plus_assign<cat::wrap_int1, int4>);
   static_assert(can_plus_assign<cat::sat_int1, int4>);
   static_assert(can_times_assign<cat::wrap_int1, int4>);
   static_assert(can_times_assign<cat::sat_int1, int4>);

   // Undefined unsigned `-=`: keeps LHS shape, so wide RHS is fine.
   static_assert(can_minus_assign<uint1, uint4>);

   // Undefined signed `-=`: PROMOTES on wide RHS, so it must be REJECTED.
   static_assert(!can_minus_assign<int1, int4>);
   static_assert(!can_minus_assign<int4, iword>);
}

// `overflow_reference` (`.undef()`, `.wrap()`, `.sat()`) observes the same
// rules as the materialized type at its policy.
test(arithmetic_semantics_overflow_reference_view_types) {
   using semantics_helpers::can_minus_assign;

   int1 a{};
   int4 b{};

   // `.wrap()` / `.sat()` views compute in their policy and do not promote.
   static_assert(cat::is_same<decltype(a.wrap() + b), cat::wrap_int1>);
   static_assert(cat::is_same<decltype(a.sat() + b), cat::sat_int1>);
   static_assert(cat::is_same<decltype(a.undef() + b), int4>);
   static_assert(cat::is_same<decltype(a.wrap() - b), cat::wrap_int1>);
   static_assert(cat::is_same<decltype(a.sat() - b), cat::sat_int1>);
   static_assert(cat::is_same<decltype(a.undef() - b), int4>);

   // Compound assignment through an overflow reference rejects the promotion
   // case the same way as the materialized type.
   static_assert(!can_minus_assign<decltype(a.undef()), int4>);
   static_assert(can_minus_assign<decltype(a.wrap()), int4>);
   static_assert(can_minus_assign<decltype(a.sat()), int4>);

   (void)b;
}

// Signed `%` is C-style remainder (sign of LHS is preserved). Unsigned `%` is
// mathematical modulo (result in `[0, |rhs|)`). The discriminating case is
// unsigned LHS with negative signed RHS, which must take `|rhs|` first so the
// result stays non-negative.
test(arithmetic_semantics_signed_remainder_unsigned_modulo) {
   // C-style remainder for signed LHS: sign of LHS is preserved.
   static_assert((int4{-7} % int4{3}) == int4{-1});
   static_assert((int4{-7} % int4{-3}) == int4{-1});
   static_assert((int4{7} % int4{-3}) == int4{1});
   static_assert((int4{7} % int4{3}) == int4{1});

   // Mathematical modulo for unsigned LHS: result is in `[0, |rhs|)`.
   static_assert((uint4{7} % uint4{3}) == uint4{1});
   static_assert((uint4{0} % uint4{3}) == uint4{0});

   // Unsigned LHS with signed RHS still gives a non-negative result in `[0,
   // |rhs|)`. This is the discriminating property between modulo and remainder.
   static_assert((uint4{7} % int4{-3}) == uint4{1});
   static_assert((uint4{8} % int4{-3}) == uint4{2});
}

namespace {
// Non-constexpr identity. Wrapping a literal in this strips its
// constant-expression-ness without changing its type, which hides it from the
// `consteval` `idx::operator-` overload's `enable_if` predicate. Lets us keep
// type-trait checks on the runtime / iword-widening path while still spelling
// literals at the call site.
auto
deconst_number(auto i) {
   return i;
}
}  // namespace

// `index` (idx) follows the same rules as the comparable `arithmetic`, but `idx
// - idx` (and `idx - signed`) returns an `iword` (signed distance) so an
// underflowed difference is representable. Wrap / sat `idx` never promote. The
// compile-time non-underflow fast path that keeps `idx` is covered by
// `arithmetic_semantics_idx_subtract_constexpr_preserves_idx`.
test(arithmetic_semantics_idx_subtract_returns_signed_distance) {
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;
   using sat_idx = cat::index<cat::overflow_policies::saturate>;

   // `idx + idx -> idx`. `idx - idx -> iword` (signed distance).
   static_assert(cat::is_same<decltype(idx{} + idx{}), idx>);
   static_assert(cat::is_same<decltype(idx{} - idx{}), iword>);

   // `idx + signed -> iword` (signed_distance). For `idx - signed` we launder
   // the RHS through `deconst_number` so it is no longer a constant expression.
   // That hides it from the constexpr non-underflow overload's `enable_if` and
   // exercises the genuine iword widening.
   static_assert(cat::is_same<decltype(idx{} + int4{}), iword>);
   static_assert(cat::is_same<decltype(idx{} - deconst_number(int4{})), iword>);
   static_assert(cat::is_same<decltype(idx{} - deconst_number(1)), iword>);
   static_assert(cat::is_same<decltype(idx{} - deconst_number(1u)), iword>);

   // wrap_idx / sat_idx never promote.
   static_assert(cat::is_same<decltype(wrap_idx{} + idx{}), wrap_idx>);
   static_assert(cat::is_same<decltype(sat_idx{} + idx{}), sat_idx>);
   static_assert(cat::is_same<decltype(wrap_idx{} - idx{}), wrap_idx>);
   static_assert(cat::is_same<decltype(sat_idx{} - idx{}), sat_idx>);
}

// `idx - integer` keeps the `idx` shape (no widening to `iword`) when the
// subtraction is a constant expression that is provably non-underflowing.
// Runtime operands and would-underflow constants still fall through to the
// iword-returning overload (validated in
// `arithmetic_semantics_idx_subtract_returns_signed_distance` above).
test(arithmetic_semantics_idx_subtract_constexpr_preserves_idx) {
   // Non-underflowing constant operands stay as `idx`. Both signed and unsigned
   // RHS, and the boundary case `lhs == rhs` (result is zero).
   static_assert(cat::is_same<decltype(idx{5} - 3u), idx>);
   static_assert(cat::is_same<decltype(idx{5} - 5u), idx>);
   static_assert(cat::is_same<decltype(idx{5} - int4{3}), idx>);
   static_assert(cat::is_same<decltype(idx{1} - 1), idx>);

   // Values are correct and the result feeds back through the implicit `idx ->
   // __SIZE_TYPE__` conversion (the original motivation for this overload, to
   // make `pointer[idx_expr]` work without `.raw`).
   static_assert((idx{5} - 3u) == 2_idx);
   static_assert((idx{5} - 5u) == 0_idx);
   static_assert((idx{5} - int4{3}) == 2_idx);
   static_assert((idx{4_uki} - 1u) == idx{4'095});

   // The constexpr-preserved `idx` implicitly converts to `__SIZE_TYPE__` for
   // raw-pointer subscripting, which is the whole point of keeping the `idx`
   // shape.
   constexpr __SIZE_TYPE__ subscript = idx{5} - 3u;
   static_assert(subscript == 2u);

   // Same-type `idx - idx` is unaffected: the non-template member from
   // `minus_interface` wins overload resolution against the templated friend,
   // so the result type is still `iword`.
   static_assert(cat::is_same<decltype(idx{5} - idx{3}), iword>);
}

// With genuine constant operands, an underflowing subtraction (`lhs < rhs`) is
// detected by the `enable_if(self.raw >= other)` predicate on the new
// `idx`-preserving `operator-`. The predicate evaluates to `false`, the
// overload drops out of the candidate set, and resolution falls through to the
// `subtract_by` member that widens to `iword` so the negative result is
// representable. This is the same path runtime operands take, but reached
// through compile-time evaluation rather than through non-constant operands
// disabling `enable_if` entirely.
test(arithmetic_semantics_idx_subtract_constexpr_underflow_widens_to_iword) {
   // Smallest underflow (`0 - 1`), three-element underflow, and signed RHS
   // underflow. All three are constant expressions. Only the would-underflow
   // predicate forces the iword fall-through.
   static_assert(cat::is_same<decltype(idx{} - 1u), iword>);
   static_assert(cat::is_same<decltype(idx{3} - 5u), iword>);
   static_assert(cat::is_same<decltype(idx{0} - int4{1}), iword>);
   static_assert(cat::is_same<decltype(idx{0} - 1), iword>);

   // The negative result round-trips correctly through `iword`, which is the
   // whole point of widening on a would-underflow.
   static_assert((idx{} - 1u) == iword{-1});
   static_assert((idx{3} - 5u) == iword{-2});
   static_assert((idx{0} - int4{1}) == iword{-1});
   static_assert((idx{0} - 1) == iword{-1});

   // Boundary: `lhs == rhs` is exactly the predicate's `>=` edge and stays on
   // the `idx`-preserving path (covered in
   // `arithmetic_semantics_idx_subtract_constexpr_preserves_idx`). The first
   // underflow value past it (`lhs == rhs - 1`) widens here.
   static_assert(cat::is_same<decltype(idx{5} - 6u), iword>);
   static_assert((idx{5} - 6u) == iword{-1});
}

// `wrap`-policy `<<` and `>>` ROTATE the underlying bit pattern instead of
// shifting bits off the end. For `arithmetic<T>` the rotation modulus is
// `sizeof(T) * 8`. For `wrap_idx` the rotation modulus is
// `sizeof(__SIZE_TYPE__) * 8 - 1` so the high bit stays zero (preserving the
// `index` invariant). Any non-negative shift count is valid (taken modulo the
// bit width).
test(arithmetic_semantics_wrap_shift_rotates_bits) {
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;

   // Result keeps LHS shape (no promotion).
   static_assert(cat::is_same<decltype(wrap_uint1{} << 1), wrap_uint1>);
   static_assert(cat::is_same<decltype(wrap_uint1{} >> 1), wrap_uint1>);
   static_assert(cat::is_same<decltype(cat::wrap_int4{} << 1), cat::wrap_int4>);
   static_assert(cat::is_same<decltype(cat::wrap_int4{} >> 1), cat::wrap_int4>);

   // Unsigned wrap left-rotation: a high-bit set rotates around to bit 0.
   static_assert((wrap_uint1{0x80u} << 1u) == wrap_uint1{0x01u});
   static_assert((wrap_uint1{0xC0u} << 1u) == wrap_uint1{0x81u});
   static_assert((wrap_uint1{0x55u} << 4u) == wrap_uint1{0x55u});
   static_assert((wrap_uint1{0x12u} << 4u) == wrap_uint1{0x21u});

   // Unsigned wrap right-rotation: a low-bit set rotates around to the high
   // bit.
   static_assert((wrap_uint1{0x01u} >> 1u) == wrap_uint1{0x80u});
   static_assert((wrap_uint1{0x03u} >> 1u) == wrap_uint1{0x81u});
   static_assert((wrap_uint1{0x12u} >> 4u) == wrap_uint1{0x21u});

   // Any count is valid: count is taken modulo the bit width.
   static_assert((wrap_uint1{0x01u} << 0u) == wrap_uint1{0x01u});
   static_assert((wrap_uint1{0x01u} << 8u) == wrap_uint1{0x01u});
   static_assert((wrap_uint1{0x01u} << 9u) == wrap_uint1{0x02u});
   static_assert((wrap_uint1{0x80u} << 17u) == wrap_uint1{0x01u});

   // Larger widths: 32-bit rotation.
   static_assert((wrap_uint4{0x80000000u} << 1u) == wrap_uint4{0x00000001u});
   static_assert((wrap_uint4{0xDEADBEEFu} << 32u) == wrap_uint4{0xDEADBEEFu});
   static_assert((wrap_uint4{0x12345678u} << 4u) == wrap_uint4{0x23456781u});

   // Signed wrap rotation operates on the raw bit pattern (the sign bit is just
   // bit `N-1`). `int1{-1}` is `0xFF`, so any rotation is a no-op.
   static_assert((wrap_int1{-1} << 3) == wrap_int1{-1});
   // `int1{0x40}` (= 64) shifted left by 1 normally becomes `128 == int8_min`,
   // which is well-defined under wrap. Rotation gives the same answer here
   // because the bit moves from position 6 to position 7, not off the top.
   static_assert((wrap_int1{0x40} << 1) == wrap_int1{wrap_int1::min()});
   // `int1{wrap_int1::min()}` (= -128, bit pattern 0x80) << 1: the sign bit
   // rotates around to bit 0. Result is `wrap_int1{1}`.
   static_assert((wrap_int1{wrap_int1::min()} << 1) == wrap_int1{1});

   // `wrap_idx`: rotation is over the lower 63 bits. The high bit stays zero so
   // the `index` high-bit-zero invariant is preserved. Bit 62 (the highest
   // usable bit) rotates around to bit 0 under `<< 1`.
   constexpr __SIZE_TYPE__ idx_high_bit = static_cast<__SIZE_TYPE__>(1) << 62u;
   static_assert((wrap_idx{idx_high_bit} << 1u) == wrap_idx{1u});
   static_assert((wrap_idx{1u} >> 1u) == wrap_idx{idx_high_bit});

   // Rotation over a count >= 63 wraps modulo 63.
   static_assert((wrap_idx{1u} << 63u) == wrap_idx{1u});
   static_assert((wrap_idx{1u} << 64u) == wrap_idx{2u});

   // `wrap_idx` rotation never sets the high bit, regardless of input. The
   // constructor would normally reject a value with the high bit set, so the
   // runtime check below catches that. Here we prove the static type does not
   // lose the LHS shape.
   static_assert(cat::is_same<decltype(wrap_idx{} << 1u), wrap_idx>);
   static_assert(cat::is_same<decltype(wrap_idx{} >> 1u), wrap_idx>);
}

// Runtime side of the semantics tests above. The `static_assert`s above catch
// type-level regressions. The assertions here catch value-level regressions for
// the cases that involve actual computation (saturation clamps, wrap-around
// values, mathematical modulo, signed-undefined widening sub, wrap-shift
// rotation through the optimizer pipeline).
test(arithmetic_semantics_runtime) {
   using wrap_idx = cat::index<cat::overflow_policies::wrap>;
   using sat_idx = cat::index<cat::overflow_policies::saturate>;

   // Saturating construction clamps. Wrapping construction applies modular
   // reduction. These bypass the compile-time `enable_if` path by going through
   // runtime values, so they exercise the `policy_cast` catch-all constructor.
   {
      cat::int4 runtime_huge = 500;
      cat::int4 runtime_negative = -500;
      cat::sat_int1 sat_clamped_high = runtime_huge;
      cat::sat_int1 sat_clamped_low = runtime_negative;
      cat::verify(sat_clamped_high == cat::sat_int1::max());
      cat::verify(sat_clamped_low == cat::sat_int1::min());

      cat::sat_uint1 sat_clamped_zero = runtime_negative;
      cat::verify(sat_clamped_zero == cat::sat_uint1{0});
   }

   // Signed undefined `-` widens to a wider RHS so an underflow stays
   // representable, and the widened value is the mathematically-correct
   // difference.
   {
      cat::int1 narrow = -100;
      cat::int4 wide = 100;
      auto diff = narrow - wide;
      static_assert(cat::is_same<decltype(diff), cat::int4>);
      cat::verify(diff == -200);
   }

   // wrap/sat LHS keeps shape, even with a wider RHS, and the policy is applied
   // to the full operation.
   {
      cat::wrap_int1 lhs = cat::wrap_int1::max();
      cat::int4 wide = 1;
      auto sum = lhs + wide;
      static_assert(cat::is_same<decltype(sum), cat::wrap_int1>);
      cat::verify(sum == cat::wrap_int1::min());
   }
   {
      cat::sat_int1 lhs = cat::sat_int1::max();
      cat::int4 wide = 1'000;
      auto sum = lhs + wide;
      static_assert(cat::is_same<decltype(sum), cat::sat_int1>);
      cat::verify(sum == cat::sat_int1::max());
   }

   // Unsigned LHS modulo with a signed RHS takes the absolute value of the RHS
   // first, so the result is in `[0, |rhs|)`.
   {
      cat::uint4 lhs = 7;
      cat::int4 negative_rhs = -3;
      auto result = lhs % negative_rhs;
      static_assert(cat::is_same<decltype(result), cat::uint4>);
      cat::verify(result == cat::uint4{1});
   }

   // `wrap_idx` / `sat_idx` subtract preserves shape and applies the policy.
   {
      wrap_idx lhs{0u};
      cat::idx rhs{1u};
      auto diff = lhs - rhs;
      static_assert(cat::is_same<decltype(diff), wrap_idx>);
      cat::verify(diff == wrap_idx{cat::limits<__SIZE_TYPE__>::max()});
   }
   {
      sat_idx lhs{0u};
      cat::idx rhs{1u};
      auto diff = lhs - rhs;
      static_assert(cat::is_same<decltype(diff), sat_idx>);
      cat::verify(diff == sat_idx{0u});
   }

   // Wrap-policy `<<`/`>>` rotate. Runtime values exercise the
   // non-constant-evaluated path (so `wrap_shl`/`wrap_shr` run through the
   // regular optimizer pipeline, not the constexpr evaluator).
   {
      wrap_uint1 v{0x80u};
      cat::verify((v << 1u) == wrap_uint1{0x01u});
      cat::verify((v >> 7u) == wrap_uint1{0x01u});

      // Rotating left by `N` then right by `N` returns the original.
      wrap_uint4 w{0xDEADBEEFu};
      cat::verify(((w << 13u) >> 13u) == w);

      // Rotating left `bit_width` times by 1 returns the original.
      wrap_uint1 r = wrap_uint1{0xA5u};
      wrap_uint1 const original = r;
      for (int i = 0; i < 8; ++i) {
         r = r << 1u;
      }
      cat::verify(r == original);
   }

   // `wrap_idx` rotation cycles within the lower 63 bits and never sets the
   // high bit.
   {
      wrap_idx v{1u};
      // After rotating right by 1, bit 0 moves to bit 62.
      auto rotated = v >> 1u;
      cat::verify(rotated == wrap_idx{static_cast<__SIZE_TYPE__>(1) << 62u});
      // The high bit must remain zero after rotation, regardless of input.
      cat::verify((rotated.raw & (static_cast<__SIZE_TYPE__>(1) << 63u)) == 0u);

      // Rotating by `63` is a no-op (count taken mod 63).
      cat::verify((v << 63u) == v);
      cat::verify((v >> 63u) == v);
   }
}

// Every member operator on a `wrap` / `saturate` LHS
// produces the same `.raw` storage value as the corresponding free
// function called on the raw operands. Concretely:
//   `(wrap_T{a} OP wrap_T{b}).raw == wrap_X(a, b)`
//   `(sat_T{a}  OP sat_T{b} ).raw == sat_X(a, b)`
// for every supported `(OP, X)` pair. This is the contract that lets
// users mix free and member calls freely. If a future refactor wires a
// member to a different impl than its free function, this test catches
// the divergence at both compile time and run time.
test(arithmetic_semantics_member_matches_free_function) {
   // Pick operands that exercise overflow for each operator so the equivalence
   // test is meaningful (not just identity).
   {
      // wrap_uint4: addition wraps past max.
      constexpr unsigned a = 0xFFFFFFFFu;
      constexpr unsigned b = 5u;
      static_assert((wrap_uint4{a} + wrap_uint4{b}).raw == cat::wrap_add(a, b));
      static_assert((wrap_uint4{a} * wrap_uint4{b}).raw == cat::wrap_mul(a, b));
      static_assert((wrap_uint4{0u} - wrap_uint4{1u}).raw
                    == cat::wrap_sub(0u, 1u));
      static_assert((wrap_uint4{a} / wrap_uint4{2u}).raw
                    == cat::wrap_div(a, 2u));
      static_assert((wrap_uint4{a} << wrap_uint4{1u}).raw
                    == cat::wrap_shl(a, 1u));
      static_assert((wrap_uint4{1u} >> wrap_uint4{1u}).raw
                    == cat::wrap_shr(1u, 1u));

      cat::verify((wrap_uint4{a} + wrap_uint4{b}).raw == cat::wrap_add(a, b));
      cat::verify((wrap_uint4{a} * wrap_uint4{b}).raw == cat::wrap_mul(a, b));
      cat::verify((wrap_uint4{0u} - wrap_uint4{1u}).raw
                  == cat::wrap_sub(0u, 1u));
      cat::verify((wrap_uint4{a} / wrap_uint4{2u}).raw == cat::wrap_div(a, 2u));
      cat::verify((wrap_uint4{a} << wrap_uint4{1u}).raw
                  == cat::wrap_shl(a, 1u));
      cat::verify((wrap_uint4{1u} >> wrap_uint4{1u}).raw
                  == cat::wrap_shr(1u, 1u));
   }
   {
      // sat_uint4: saturating clamp at max.
      constexpr unsigned a = 0xFFFFFFFFu;
      constexpr unsigned b = 5u;
      static_assert((sat_uint4{a} + sat_uint4{b}).raw == cat::sat_add(a, b));
      static_assert((sat_uint4{a} * sat_uint4{b}).raw == cat::sat_mul(a, b));
      static_assert((sat_uint4{0u} - sat_uint4{1u}).raw
                    == cat::sat_sub(0u, 1u));
      static_assert((sat_uint4{a} / sat_uint4{2u}).raw == cat::sat_div(a, 2u));
      static_assert((sat_uint4{a} << sat_uint4{1u}).raw == cat::sat_shl(a, 1u));
      static_assert((sat_uint4{1u} >> sat_uint4{1u}).raw
                    == cat::sat_shr(1u, 1u));

      cat::verify((sat_uint4{a} + sat_uint4{b}).raw == cat::sat_add(a, b));
      cat::verify((sat_uint4{a} * sat_uint4{b}).raw == cat::sat_mul(a, b));
      cat::verify((sat_uint4{0u} - sat_uint4{1u}).raw == cat::sat_sub(0u, 1u));
      cat::verify((sat_uint4{a} / sat_uint4{2u}).raw == cat::sat_div(a, 2u));
      cat::verify((sat_uint4{a} << sat_uint4{1u}).raw == cat::sat_shl(a, 1u));
      cat::verify((sat_uint4{1u} >> sat_uint4{1u}).raw == cat::sat_shr(1u, 1u));
   }
   {
      // wrap_int4: signed wrap covers negative-direction overflow and the
      // `min() / -1` edge case.
      constexpr int a = cat::int4_max.raw;
      constexpr int b = cat::int4_min.raw;
      static_assert((wrap_int4{a} + wrap_int4{1}).raw == cat::wrap_add(a, 1));
      static_assert((wrap_int4{b} - wrap_int4{1}).raw == cat::wrap_sub(b, 1));
      static_assert((wrap_int4{a} * wrap_int4{2}).raw == cat::wrap_mul(a, 2));
      static_assert((wrap_int4{b} / wrap_int4{-1}).raw == cat::wrap_div(b, -1));
      static_assert((wrap_int4{a} << wrap_int4{1}).raw == cat::wrap_shl(a, 1));
      static_assert((wrap_int4{a} >> wrap_int4{1}).raw == cat::wrap_shr(a, 1));

      cat::verify((wrap_int4{a} + wrap_int4{1}).raw == cat::wrap_add(a, 1));
      cat::verify((wrap_int4{b} - wrap_int4{1}).raw == cat::wrap_sub(b, 1));
      cat::verify((wrap_int4{a} * wrap_int4{2}).raw == cat::wrap_mul(a, 2));
      cat::verify((wrap_int4{b} / wrap_int4{-1}).raw == cat::wrap_div(b, -1));
      cat::verify((wrap_int4{a} << wrap_int4{1}).raw == cat::wrap_shl(a, 1));
      cat::verify((wrap_int4{a} >> wrap_int4{1}).raw == cat::wrap_shr(a, 1));
   }
   {
      // sat_int4: signed saturation covers all four overflow direction combos
      // plus the `min() / -1` edge case.
      constexpr int a = cat::int4_max.raw;
      constexpr int b = cat::int4_min.raw;
      static_assert((sat_int4{a} + sat_int4{1}).raw == cat::sat_add(a, 1));
      static_assert((sat_int4{b} + sat_int4{-1}).raw == cat::sat_add(b, -1));
      static_assert((sat_int4{b} - sat_int4{1}).raw == cat::sat_sub(b, 1));
      static_assert((sat_int4{a} - sat_int4{-1}).raw == cat::sat_sub(a, -1));
      static_assert((sat_int4{a} * sat_int4{2}).raw == cat::sat_mul(a, 2));
      static_assert((sat_int4{b} * sat_int4{2}).raw == cat::sat_mul(b, 2));
      static_assert((sat_int4{b} / sat_int4{-1}).raw == cat::sat_div(b, -1));
      static_assert((sat_int4{a} << sat_int4{1}).raw == cat::sat_shl(a, 1));
      static_assert((sat_int4{a} >> sat_int4{1}).raw == cat::sat_shr(a, 1));

      cat::verify((sat_int4{a} + sat_int4{1}).raw == cat::sat_add(a, 1));
      cat::verify((sat_int4{b} + sat_int4{-1}).raw == cat::sat_add(b, -1));
      cat::verify((sat_int4{b} - sat_int4{1}).raw == cat::sat_sub(b, 1));
      cat::verify((sat_int4{a} - sat_int4{-1}).raw == cat::sat_sub(a, -1));
      cat::verify((sat_int4{a} * sat_int4{2}).raw == cat::sat_mul(a, 2));
      cat::verify((sat_int4{b} * sat_int4{2}).raw == cat::sat_mul(b, 2));
      cat::verify((sat_int4{b} / sat_int4{-1}).raw == cat::sat_div(b, -1));
      cat::verify((sat_int4{a} << sat_int4{1}).raw == cat::sat_shl(a, 1));
      cat::verify((sat_int4{a} >> sat_int4{1}).raw == cat::sat_shr(a, 1));
   }
   {
      // wrap_idx / sat_idx shifts go through the index-specific free overloads.
      // The equivalence must still hold.
      using wrap_idx = cat::index<cat::overflow_policies::wrap>;
      using sat_idx = cat::index<cat::overflow_policies::saturate>;
      constexpr __SIZE_TYPE__ bit_62 = static_cast<__SIZE_TYPE__>(1) << 62u;
      static_assert((wrap_idx{bit_62} << 1u).raw
                    == cat::wrap_shl(wrap_idx{bit_62}, 1u).raw);
      static_assert((wrap_idx{1u} >> 1u).raw
                    == cat::wrap_shr(wrap_idx{1u}, 1u).raw);
      static_assert((sat_idx{bit_62} << 1u).raw
                    == cat::sat_shl(sat_idx{bit_62}, 1u).raw);
      static_assert((sat_idx{1u} >> 1u).raw
                    == cat::sat_shr(sat_idx{1u}, 1u).raw);

      cat::verify((wrap_idx{bit_62} << 1u).raw
                  == cat::wrap_shl(wrap_idx{bit_62}, 1u).raw);
      cat::verify((sat_idx{bit_62} << 1u).raw
                  == cat::sat_shl(sat_idx{bit_62}, 1u).raw);
   }
}

namespace {

// Mirrors `detail::promoted_arithmetic` and `detail::promoted_type` constraints
// in `<cat/arithmetic>` for every pair in `promotion_arithmetic_types`.
// `promoted_type` picks the wider operand's storage shape and tags it with the
// LHS's overflow policy.

template <typename Left, typename Right>
consteval auto
expected_promoted_arithmetic() {
   using L = cat::remove_constref<Left>;
   using R = cat::remove_constref<Right>;
   constexpr cat::overflow_policies policy = cat::detail::overflow_policy_of<L>;

   if constexpr (policy != cat::overflow_policies::undefined) {
      // wrap / sat / trap: never promote, always return LHS shape.
      return typename cat::detail::rebind_overflow_policy<L, policy>::type{};
   } else if constexpr (cat::detail::integer_promotion_hierarchy<L>::order
                        >= cat::detail::integer_promotion_hierarchy<R>::order) {
      return typename cat::detail::rebind_overflow_policy<L, policy>::type{};
   } else {
      return typename cat::detail::rebind_overflow_policy<R, policy>::type{};
   }
}

template <typename Left, typename Right>
using expected_promoted_type =
   decltype(expected_promoted_arithmetic<Left, Right>());

template <typename L, typename R>
concept has_promoted_type =
   requires { typename cat::detail::promoted_type<L, R>; };

template <typename L, typename R>
consteval auto
promotion_pair_matches_fn() -> bool {
   if constexpr (!has_promoted_type<L, R>) {
      return true;
   } else {
      return cat::is_same<cat::detail::promoted_type<L, R>,
                          expected_promoted_type<L, R>>;
   }
}

template <typename T, typename... Us>
consteval auto
promotion_row(cat::type_list<T>, cat::type_list<Us...>) -> bool {
   return (... && promotion_pair_matches_fn<T, Us>());
}

template <typename... Ts>
consteval auto
promotion_grid(cat::type_list<Ts...> all) -> bool {
   return (... && promotion_row(cat::type_list<Ts>{}, all));
}

// Every `cat::detail::promoted_type<L, R>` that exists must match
// `expected_promoted_arithmetic` above. Pairs where `promoted_type` is absent
// (mixed float vs integer) are skipped via checking `has_promoted_type`.
using promotion_arithmetic_types = cat::type_list<
   // Raw scalars (unsafe arithmetic, platform-sized where relevant).
   int, unsigned int, float, double,
   // Fixed-width wrappers.
   int1, uint1, int2, uint2, int4, uint4, int8, uint8, iword, uword,
   // Distinct overflow policies at the same width.
   wrap_int4, sat_uint4,
   // Indices (special promotion order 7).
   idx, cat::index<cat::overflow_policies::wrap>,
   // Pointer integers (order 9).
   intptr<void>, uintptr<void>, float4, float8>;

static_assert(promotion_grid(promotion_arithmetic_types{}));

}  // namespace

#pragma clang diagnostic pop
