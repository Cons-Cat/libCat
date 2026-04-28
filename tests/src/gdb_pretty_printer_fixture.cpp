#include <cat/arithmetic>
#include <cat/array>
#include <cat/bit>
#include <cat/bitset>
#include <cat/span>
#include <cat/string>

// This file compiles libCat data types with DWARF debug symbols for
// GDB to consume. This is loaded by `gdb_pretty_printers.gdb.in` through
// the `check-gdb-pretty-printers` CMake target to catch regressions
// in pretty printers.

// This test does not address semantics, so we shouldn't see warnings.
// NOLINTBEGIN
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

using wrap_int4 =
   cat::arithmetic<__INT32_TYPE__, cat::overflow_policies::wrap>;
using wrap_iword =
   cat::arithmetic<cat::iword::raw_type, cat::overflow_policies::wrap>;
using sat_iword =
   cat::arithmetic<cat::iword::raw_type, cat::overflow_policies::saturate>;
using wrap_uword =
   cat::arithmetic<cat::uword::raw_type, cat::overflow_policies::wrap>;
using sat_uword =
   cat::arithmetic<cat::uword::raw_type, cat::overflow_policies::saturate>;
using wrap_idx = cat::index<cat::overflow_policies::wrap>;
using sat_idx = cat::index<cat::overflow_policies::saturate>;

namespace arithmetic_matrix {

template <typename... Types>
struct type_pack {};

template <typename T>
inline constexpr bool is_index = false;

template <cat::overflow_policies policy>
inline constexpr bool is_index<cat::index<policy>> = true;

template <typename T>
constexpr auto
raw(T value) {
   return cat::make_raw_arithmetic(value);
}

template <typename Value, typename Expected>
constexpr auto
raw_equals(Value value, Expected expected) -> bool {
   using raw_value = decltype(raw(value));
   return raw(value) == static_cast<raw_value>(expected);
}

template <typename T>
constexpr auto
min_value() -> T {
   return T{cat::limits<T>::min()};
}

template <typename T>
constexpr auto
max_value() -> T {
   return T{cat::limits<T>::max()};
}

template <typename T>
constexpr auto
in_range_checks() -> bool {
   bool ok = true;
   ok = ok && raw_equals(T{6u}, 6u);
   ok = ok && raw_equals(T{6u} + T{2u}, 8u);
   ok = ok && raw_equals(T{6u} - T{2u}, 4u);
   ok = ok && raw_equals(T{6u} * T{2u}, 12u);
   ok = ok && raw_equals(T{6u} / T{2u}, 3u);
   ok = ok && raw_equals(T{7u} % T{3u}, 1u);
   ok = ok && raw_equals(T{1u} << T{1u}, 2u);
   ok = ok && raw_equals(T{4u} >> T{1u}, 2u);
   return ok;
}

template <typename T>
constexpr auto
undefined_checks() -> bool {
   bool ok = in_range_checks<T>();
   if constexpr (is_index<T>) {
      ok = ok && cat::is_same<decltype(T{0u} - T{1u}), cat::iword>;
   } else {
      ok = ok && cat::is_same<decltype(T{6u} + T{2u}), T>;
      ok = ok && cat::is_same<decltype(T{6u} * T{2u}), T>;
   }
   return ok;
}

template <typename T>
constexpr auto
wrap_checks() -> bool {
   bool ok = in_range_checks<T>();
   using raw_type = typename T::raw_type;

   if constexpr (is_index<T>) {
      constexpr raw_type ring_bits = cat::limits<T>::digits;
      constexpr raw_type ring_top = raw_type{1u} << (ring_bits - 1u);
      constexpr raw_type high_bit = raw_type{1u} << (cat::limits<T>::bits - 1u);
      ok = ok && raw_equals(T{1u} << T{ring_bits}, 1u);
      ok = ok && raw_equals(T{1u} << T{ring_bits + 1u}, 2u);
      ok = ok && raw_equals(T{ring_top} << T{1u}, 1u);
      ok = ok && raw_equals(T{1u} >> T{1u}, ring_top);
      ok = ok && raw_equals(T{2u} >> T{ring_bits + 1u}, 1u);
      ok = ok && ((raw(T{ring_top} << T{3u}) & high_bit) == 0u);
      ok = ok && cat::is_same<decltype(T{6u} + T{2u}), T>;
      ok = ok && cat::is_same<decltype(T{6u} - T{2u}), T>;
   } else {
      constexpr raw_type min = cat::limits<T>::min();
      constexpr raw_type max = cat::limits<T>::max();
      constexpr raw_type bits = cat::limits<T>::bits;
      ok = ok && raw_equals(max_value<T>() + T{1u},
                            cat::wrap_add(max, raw_type{1u}));
      ok = ok && raw_equals(min_value<T>() - T{1u},
                            cat::wrap_sub(min, raw_type{1u}));
      ok = ok && raw_equals(max_value<T>() * T{2u},
                            cat::wrap_mul(max, raw_type{2u}));
      ok = ok && raw_equals(T{1u} << T{bits}, 1u);
      ok = ok && raw_equals(T{1u} >> T{bits}, 1u);
      ok = ok && raw_equals(T{1u} << T{bits + 1u}, 2u);
      if constexpr (cat::is_signed_integral<raw_type>) {
         ok = ok && raw_equals(min_value<T>() / T{-1},
                               cat::wrap_div(min, raw_type{-1}));
      }
      ok = ok && cat::is_same<decltype(T{6u} + T{2u}), T>;
      ok = ok && cat::is_same<decltype(T{6u} * T{2u}), T>;
   }
   return ok;
}

template <typename T>
constexpr auto
saturate_checks() -> bool {
   bool ok = in_range_checks<T>();
   using raw_type = typename T::raw_type;

   if constexpr (is_index<T>) {
      constexpr raw_type high_bit = raw_type{1u} << (cat::limits<T>::bits - 1u);
      ok = ok && raw_equals(T{0u} - T{1u}, 0u);
      ok = ok && raw_equals(T{1u} << T{cat::limits<T>::digits},
                            cat::limits<T>::max());
      ok = ok && raw_equals(max_value<T>() << T{1u}, cat::limits<T>::max());
      ok = ok && raw_equals(max_value<T>() >> T{cat::limits<T>::bits}, 0u);
      ok = ok && ((raw(max_value<T>() << T{1u}) & high_bit) == 0u);
      ok = ok && cat::is_same<decltype(T{6u} - T{2u}), T>;
   } else {
      constexpr raw_type min = cat::limits<T>::min();
      constexpr raw_type max = cat::limits<T>::max();
      constexpr raw_type bits = cat::limits<T>::bits;
      ok = ok && raw_equals(max_value<T>() + T{1u}, max);
      ok = ok && raw_equals(min_value<T>() - T{1u}, min);
      ok = ok && raw_equals(max_value<T>() * T{2u}, max);
      ok = ok && raw_equals(max_value<T>() << T{1u}, max);
      ok = ok && raw_equals(max_value<T>() >> T{bits}, 0u);
      if constexpr (cat::is_signed_integral<raw_type>) {
         ok = ok && raw_equals(min_value<T>() * T{2u}, min);
         ok = ok && raw_equals(min_value<T>() / T{-1}, max);
         ok = ok && raw_equals(min_value<T>() >> T{bits}, -1);
      }
      ok = ok && cat::is_same<decltype(T{6u} + T{2u}), T>;
      ok = ok && cat::is_same<decltype(T{6u} * T{2u}), T>;
   }
   return ok;
}

template <typename T>
constexpr auto
case_checks() -> bool {
   if constexpr (T::policy == cat::overflow_policies::wrap) {
      return wrap_checks<T>();
   } else if constexpr (T::policy == cat::overflow_policies::saturate) {
      return saturate_checks<T>();
   } else {
      return undefined_checks<T>();
   }
}

using cases = type_pack<
   cat::int1, cat::wrap_int1, cat::sat_int1, cat::uint1, cat::wrap_uint1,
   cat::sat_uint1, cat::int2, cat::wrap_int2, cat::sat_int2, cat::uint2,
   cat::wrap_uint2, cat::sat_uint2, cat::int4, cat::wrap_int4, cat::sat_int4,
   cat::uint4, cat::wrap_uint4, cat::sat_uint4, cat::int8, cat::wrap_int8,
   cat::sat_int8, cat::uint8, cat::wrap_uint8, cat::sat_uint8, cat::iword,
   cat::arithmetic<cat::iword::raw_type, cat::overflow_policies::wrap>,
   cat::arithmetic<cat::iword::raw_type, cat::overflow_policies::saturate>,
   cat::uword,
   cat::arithmetic<cat::uword::raw_type, cat::overflow_policies::wrap>,
   cat::arithmetic<cat::uword::raw_type, cat::overflow_policies::saturate>,
   cat::idx, cat::index<cat::overflow_policies::wrap>,
   cat::index<cat::overflow_policies::saturate>>;

template <typename... Types>
constexpr auto
all_static(type_pack<Types...>) -> bool {
   return (case_checks<Types>() && ...);
}

template <typename... Types>
auto
run(type_pack<Types...>) -> int {
   int failures = 0;
   ((failures += case_checks<Types>() ? 0 : 1), ...);
   return failures;
}

}  // namespace arithmetic_matrix

static_assert(arithmetic_matrix::all_static(arithmetic_matrix::cases{}));

[[gnu::used, gnu::retain]]
cat::monostate_type cat_gdb_monostate_value = cat::monostate;

[[gnu::used, gnu::retain]]
cat::int4 cat_gdb_arithmetic_value = 42;

[[gnu::used, gnu::retain]]
wrap_int4 cat_gdb_arithmetic_wrap_value = 250;

[[gnu::used, gnu::retain]]
cat::idx cat_gdb_index_value = 7u;

[[gnu::used, gnu::retain]]
cat::byte cat_gdb_byte_value;

[[gnu::used, gnu::retain]]
cat::int4 cat_gdb_span_data[] = {1, 2, 3};

[[gnu::used, gnu::retain]]
cat::span<cat::int4> cat_gdb_span_value{cat_gdb_span_data, 3u};

[[gnu::used, gnu::retain]]
cat::array<cat::int4, 3u> cat_gdb_array_value{4, 5, 6};

[[gnu::used, gnu::retain]]
char cat_gdb_str_span_data[] = {'h', 'i', '\0', '!'};

[[gnu::used, gnu::retain]]
cat::str_span cat_gdb_str_span_value{cat_gdb_str_span_data, 4u};

[[gnu::used, gnu::retain]]
cat::str_inplace<5u> cat_gdb_str_inplace_value{"hello"};

[[gnu::used, gnu::retain]]
char cat_gdb_zstr_span_data[] = {'z', 'i', 'p', '\0'};

[[gnu::used, gnu::retain]]
cat::zstr_span cat_gdb_zstr_span_value{cat_gdb_zstr_span_data, 4u};

[[gnu::used, gnu::retain]]
cat::zstr_view cat_gdb_zstr_view_value{"view"};

[[gnu::used, gnu::retain]]
cat::zstr_inplace<6u> cat_gdb_zstr_inplace_value{"hello"};

[[gnu::used, gnu::retain]]
cat::zstr_inplace<6u> cat_gdb_zstr_padded_inplace_value =
   cat::make_zstr_inplace<6u>("hi");

[[gnu::used, gnu::retain]]
cat::bit_value cat_gdb_bit_value = true;

[[gnu::used, gnu::retain]]
cat::bitset<10u> cat_gdb_bitset_value{"1010101010"};

[[gnu::used, gnu::retain]]
cat::bitset<6u> cat_gdb_bitset6_value =
   cat::make_bitset_filled<6u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<8u> cat_gdb_bitset8_value =
   cat::make_bitset_filled<8u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<16u> cat_gdb_bitset16_value =
   cat::make_bitset_filled<16u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<17u> cat_gdb_bitset17_value =
   cat::make_bitset_filled<17u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<24u> cat_gdb_bitset24_value =
   cat::make_bitset_filled<24u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<25u> cat_gdb_bitset25_value =
   cat::make_bitset_filled<25u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<32u> cat_gdb_bitset32_value =
   cat::make_bitset_filled<32u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<33u> cat_gdb_bitset33_value =
   cat::make_bitset_filled<33u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<40u> cat_gdb_bitset40_value =
   cat::make_bitset_filled<40u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<64u> cat_gdb_bitset64_value =
   cat::make_bitset_filled<64u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<65u> cat_gdb_bitset65_value =
   cat::make_bitset_filled<65u>(cat::one_bit);
[[gnu::used, gnu::retain]]
cat::bitset<128u> cat_gdb_bitset128_value =
   cat::make_bitset_filled<128u>(cat::one_bit);

[[gnu::used, gnu::retain]]
int cat_gdb_arithmetic_matrix_failures = 0;

[[gnu::used, gnu::retain]]
cat::int1 cat_gdb_int1_value = 65;
[[gnu::used, gnu::retain]]
cat::wrap_int1 cat_gdb_wrap_int1_value = 65;
[[gnu::used, gnu::retain]]
cat::sat_int1 cat_gdb_sat_int1_value = 65;

[[gnu::used, gnu::retain]]
cat::uint1 cat_gdb_uint1_value = 65u;
[[gnu::used, gnu::retain]]
cat::wrap_uint1 cat_gdb_wrap_uint1_value = 65u;
[[gnu::used, gnu::retain]]
cat::sat_uint1 cat_gdb_sat_uint1_value = 65u;

[[gnu::used, gnu::retain]]
cat::int2 cat_gdb_int2_value = 1200;
[[gnu::used, gnu::retain]]
cat::wrap_int2 cat_gdb_wrap_int2_value = 1200;
[[gnu::used, gnu::retain]]
cat::sat_int2 cat_gdb_sat_int2_value = 1200;

[[gnu::used, gnu::retain]]
cat::uint2 cat_gdb_uint2_value = 1200u;
[[gnu::used, gnu::retain]]
cat::wrap_uint2 cat_gdb_wrap_uint2_value = 1200u;
[[gnu::used, gnu::retain]]
cat::sat_uint2 cat_gdb_sat_uint2_value = 1200u;

[[gnu::used, gnu::retain]]
cat::int4 cat_gdb_int4_value = 120000;
[[gnu::used, gnu::retain]]
cat::wrap_int4 cat_gdb_wrap_int4_value = 120000;
[[gnu::used, gnu::retain]]
cat::sat_int4 cat_gdb_sat_int4_value = 120000;

[[gnu::used, gnu::retain]]
cat::uint4 cat_gdb_uint4_value = 120000u;
[[gnu::used, gnu::retain]]
cat::wrap_uint4 cat_gdb_wrap_uint4_value = 120000u;
[[gnu::used, gnu::retain]]
cat::sat_uint4 cat_gdb_sat_uint4_value = 120000u;

[[gnu::used, gnu::retain]]
cat::int8 cat_gdb_int8_value = 12000000000;
[[gnu::used, gnu::retain]]
cat::wrap_int8 cat_gdb_wrap_int8_value = 12000000000;
[[gnu::used, gnu::retain]]
cat::sat_int8 cat_gdb_sat_int8_value = 12000000000;

[[gnu::used, gnu::retain]]
cat::uint8 cat_gdb_uint8_value = 12000000000u;
[[gnu::used, gnu::retain]]
cat::wrap_uint8 cat_gdb_wrap_uint8_value = 12000000000u;
[[gnu::used, gnu::retain]]
cat::sat_uint8 cat_gdb_sat_uint8_value = 12000000000u;

[[gnu::used, gnu::retain]]
cat::iword cat_gdb_iword_value = 12000000000;
[[gnu::used, gnu::retain]]
wrap_iword cat_gdb_wrap_iword_value = 12000000000;
[[gnu::used, gnu::retain]]
sat_iword cat_gdb_sat_iword_value = 12000000000;

[[gnu::used, gnu::retain]]
cat::uword cat_gdb_uword_value = 12000000000u;
[[gnu::used, gnu::retain]]
wrap_uword cat_gdb_wrap_uword_value = 12000000000u;
[[gnu::used, gnu::retain]]
sat_uword cat_gdb_sat_uword_value = 12000000000u;

[[gnu::used, gnu::retain]]
cat::idx cat_gdb_idx_value = 12000000000u;
[[gnu::used, gnu::retain]]
wrap_idx cat_gdb_wrap_idx_value = 12000000000u;
[[gnu::used, gnu::retain]]
sat_idx cat_gdb_sat_idx_value = 12000000000u;

extern "C" [[gnu::noinline, clang::optnone]]
void
cat_gdb_pretty_printer_breakpoint() {
   asm volatile("" ::"g"(&cat_gdb_monostate_value),
                "g"(&cat_gdb_arithmetic_value),
                "g"(&cat_gdb_arithmetic_wrap_value),
                "g"(&cat_gdb_index_value),
                "g"(&cat_gdb_byte_value),
                "g"(&cat_gdb_span_value),
                "g"(&cat_gdb_array_value),
                "g"(&cat_gdb_str_span_value),
                "g"(&cat_gdb_str_inplace_value),
                "g"(&cat_gdb_zstr_span_value),
                "g"(&cat_gdb_zstr_view_value),
                "g"(&cat_gdb_zstr_inplace_value),
                "g"(&cat_gdb_zstr_padded_inplace_value),
                "g"(&cat_gdb_bit_value),
                "g"(&cat_gdb_bitset_value),
                "g"(&cat_gdb_bitset6_value),
                "g"(&cat_gdb_bitset8_value),
                "g"(&cat_gdb_bitset16_value),
                "g"(&cat_gdb_bitset17_value),
                "g"(&cat_gdb_bitset24_value),
                "g"(&cat_gdb_bitset25_value),
                "g"(&cat_gdb_bitset32_value),
                "g"(&cat_gdb_bitset33_value),
                "g"(&cat_gdb_bitset40_value),
                "g"(&cat_gdb_bitset64_value),
                "g"(&cat_gdb_bitset65_value),
                "g"(&cat_gdb_bitset128_value),
                "g"(&cat_gdb_arithmetic_matrix_failures)
                : "memory");
}

auto
main() -> int {
   cat_gdb_byte_value.value = 0xabu;
   cat_gdb_arithmetic_matrix_failures =
      arithmetic_matrix::run(arithmetic_matrix::cases{});
   cat_gdb_pretty_printer_breakpoint();
   return cat_gdb_arithmetic_matrix_failures == 0 ? 0 : 1;
}

// NOLINTEND
#pragma clang diagnostic pop
