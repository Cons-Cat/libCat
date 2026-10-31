#include <cat/bitset_enum>

#include "../unit_tests.hpp"

namespace {
enum class test_flags : unsigned short {
   read = 1u << 0,
   write = 1u << 1,
   execute = 1u << 2,
   sticky = 1u << 15,
};
}  // namespace

template <>
struct cat::enum_flag_trait<test_flags> : cat::true_trait {};

$test(bitset_enum) {
   using flags_type = cat::bitset_enum<test_flags>;
   static_assert(flags_type::size() == 16u);
   static_assert(sizeof(flags_type) == sizeof(test_flags));

   flags_type flags =
      cat::make_bitset_enum<test_flags>(test_flags::read, test_flags::sticky);
   cat::verify(flags[test_flags::read]);
   cat::verify(!flags[test_flags::write]);
   cat::verify(flags[test_flags::sticky]);
   cat::verify(flags.popcount() == 2u);
   // A flag's bit position is its index in the underlying `bitset`.
   cat::verify(flags.countr_zero() == 0u);
   cat::verify(flags.countl_zero() == 0u);

   flags[test_flags::read] = false;
   flags[test_flags::execute] = true;
   cat::verify(flags.countr_zero() == 2u);
   cat::verify(flags.at(test_flags::execute).value());

   // `at()` is empty, rather than asserting, for a flag that is not a single
   // declared bit.
   flags_type const& const_flags = flags;
   static_assert(noexcept(flags.at(test_flags::read)));
   static_assert(noexcept(const_flags.at(test_flags::read)));
   cat::verify(const_flags.at(test_flags::execute).value());
   cat::verify(!const_flags.at(test_flags::read).value());
   cat::verify(!flags.at(static_cast<test_flags>(1u << 3u)).has_value());
   cat::verify(!const_flags.at(static_cast<test_flags>(1u << 3u)).has_value());
   cat::verify(!flags.at(test_flags::read | test_flags::write).has_value());
   cat::verify(!flags.at(static_cast<test_flags>(0u)).has_value());
   flags.at(test_flags::write).value() = true;
   cat::verify(flags[test_flags::write]);
   flags[test_flags::write] = false;

   flags_type const other =
      cat::make_bitset_enum<test_flags>(test_flags::execute, test_flags::write);
   cat::verify(
      (flags & other) == cat::make_bitset_enum<test_flags>(test_flags::execute)
   );
   cat::verify((flags | other).popcount() == 3u);
   cat::verify(
      (flags ^ other)
      == cat::make_bitset_enum<test_flags>(
         test_flags::write, test_flags::sticky
      )
   );
   flags_type xor_accum = flags;
   xor_accum ^= other;
   cat::verify(xor_accum == (flags ^ other));
   static_assert(noexcept(xor_accum ^= other));
   // Only the four bits named by `test_flags` enumerators are ever set.
   cat::verify(
      (~flags)
      == cat::make_bitset_enum<test_flags>(test_flags::read, test_flags::write)
   );
   cat::verify(cat::make_bitset_enum_filled<test_flags>(true).popcount() == 4u);
   cat::verify(cat::make_bitset_enum_filled<test_flags>(true).all_of());
   cat::verify(!flags.all_of());
   cat::verify((flags | ~flags).all_of());

   flags_type random_flags =
      cat::make_bitset_enum_filled_random<test_flags>([] {
         return true;
      });
   cat::verify(random_flags.popcount() == 4u);
   cat::verify(random_flags.all_of());
}

#ifndef NDEBUG
namespace {
constinit cat::idx undeclared_flag_hits{};

void
count_undeclared_flag(cat::source_location const& /*unused*/) {
   ++undeclared_flag_hits;
}
}  // namespace

$test(bitset_enum_undeclared_bit) {
   undeclared_flag_hits = 0u;
   {
      cat::scoped_assert_handler const guard(&count_undeclared_flag);
      cat::bitset_enum<test_flags> flags;
      // No `test_flags` enumerator names bit 3.
      auto _ = static_cast<bool>(flags[static_cast<test_flags>(1u << 3u)]);
   }
   cat::verify(undeclared_flag_hits == 1u);
}
#endif
