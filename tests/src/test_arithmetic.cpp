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
   static_assert(
      !cat::detail::has_raw_implicit_storage_shape<int,
                                                   cat::uint4::raw_type>());
   static_assert(
      !cat::detail::has_raw_implicit_storage_shape<unsigned int,
                                                   cat::int4::raw_type>());
   static_assert(cat::detail::has_raw_implicit_storage_shape<int, float>());
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
      cat::detail::has_raw_implicit_storage_shape<cat::uint4::raw_type,
                                                  cat::idx::raw_type>());
   static_assert(
      cat::detail::raw_source_fits_implicit_storage<cat::idx::raw_type>(200));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::idx::raw_type>(-1));
   static_assert(
      !cat::detail::raw_source_fits_implicit_storage<cat::idx::raw_type>(-1ll));

   static_assert(cat::detail::has_raw_implicit_storage_shape<
                 cat::uint4::raw_type, cat::uintptr<void>::raw_type>());
   static_assert(cat::detail::raw_source_fits_implicit_storage<
                 cat::uintptr<void>::raw_type>(200));
   static_assert(!cat::detail::raw_source_fits_implicit_storage<
                 cat::uintptr<void>::raw_type>(-1));
   static_assert(!cat::detail::raw_source_fits_implicit_storage<
                 cat::uintptr<void>::raw_type>(-1ll));

   static_assert(cat::detail::has_raw_implicit_storage_shape<
                 cat::uint4::raw_type, cat::intptr<void>::raw_type>());
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

   static_assert(cat::is_same<decltype(uword::max() - uintptr<void>{8u}),
                              uword>);
   static_assert(cat::is_same<decltype(iword::max() - intptr<void>{8}),
                              iword>);

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

   static_assert(uword(-1) == cat::limits<uword>::max());
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
   uintptr<void> uincptr{0xdeadbeef};
   cat::verify(++uincptr == 0xdeadbef0);
   cat::verify(uincptr++ == 0xdeadbef1);
   cat::verify(--uincptr == 0xdeadbef0);
   cat::verify(uincptr-- == 0xdeadbeef);
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
   // Test signed saturating addition.
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

   // Overflow reference views (`wrap()`, `sat()`, ...) use `wrap_add`, `sat_add`,
   // and related helpers in `constexpr` the same way as at runtime.
   static_assert((cat::int4_max.wrap() + 100) == cat::int4_min + 99);
   static_assert((cat::int4_max.sat() + 100) == cat::int4_max);
   static_assert((cat::int4_min.wrap() - 1) == cat::int4_max);
   static_assert((cat::uint4_max.wrap() + 1u) == cat::uint4_min);
   static_assert((cat::int4_max.sat() + 1) == cat::int4_max);
   static_assert((cat::int4_min.sat() - 1) == cat::int4_min);
}

test(arithmetic_promotion_hierarchy_overflow_semantics_and_strong_overflow_types) {
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

test(arithmetic_float4_binary_ops_raw_float_ordering_limits_make_signed_and_compound) {
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

namespace {

// Mirrors `detail::promoted_arithmetic` and `detail::promoted_type` constraints
// in `<cat/arithmetic>` for every pair in `promotion_arithmetic_types`.

template <typename left, typename right>
consteval auto
expected_promoted_arithmetic() {
   using L = cat::remove_constref<left>;
   using R = cat::remove_constref<right>;
   constexpr bool is_left_unsafe = cat::is_unsafe_arithmetic<L>;
   constexpr bool is_right_unsafe = cat::is_unsafe_arithmetic<R>;

   if constexpr (cat::detail::integer_promotion_hierarchy<L>::order
                 >= cat::detail::integer_promotion_hierarchy<R>::order) {
      if constexpr (is_left_unsafe) {
         return cat::arithmetic<L, cat::overflow_policies::undefined>();
      } else {
         return L{};
      }
   } else {
      if constexpr (is_right_unsafe) {
         return cat::arithmetic<R, cat::overflow_policies::undefined>();
      } else {
         return R{};
      }
   }
}

template <typename left, typename right>
using expected_promoted_type =
   decltype(expected_promoted_arithmetic<left, right>());

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
// (mixed signedness at the same promotion order, or mixed float vs integer) are
// skipped via checking `has_promoted_type`.
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

