#include <cat/arithmetic>
#include <cat/maybe>

#include "../unit_tests.hpp"

// Exercises `cat::narrow_cast`: safe widening (still `maybe` wrapped), true
// narrowing with same signed-ness, mixed signed-ness, and `idx` ->
// `maybe<idx>`.

namespace {

// Compile-time invariants (fail the build if these regress):

static_assert(cat::is_same<decltype(cat::narrow_cast<cat::idx>(cat::uword{0u})),
                           cat::maybe<cat::idx>>);

static_assert(sizeof(cat::maybe<cat::idx>) == sizeof(cat::idx),
              "`maybe<idx>` must not grow past `idx` (high-bit niche in "
              "`narrow_cast<idx>`)");

// Safe widening still produces a `maybe` and preserves the value.
static_assert([] {
   auto const m = cat::narrow_cast<cat::int4>(cat::int2{42});
   return m.has_value() && m.value().raw == 42;
}());

// `int4` -> `int8` is non-narrowing in the hierarchy. No loss check on the
// "narrowing" name. Value must still round-trip inside `maybe`.
static_assert([] {
   auto const m = cat::narrow_cast<cat::int8>(cat::int4{-99});
   return m.has_value() && m.value() == -99_i8;
}());

}  // namespace

test(narrow_cast_safe_widening_and_no_check_small_to_large) {
   {
      auto const m = cat::narrow_cast<cat::int4>(cat::int2{100});
      cat::verify(m.has_value());
      cat::verify(m.value().raw == 100);
   }
   {
      auto const m = cat::narrow_cast<cat::int8>(cat::int4{0x12345678_i4});
      cat::verify(m.has_value());
      cat::verify(m.value() == 0x12345678_i8);
   }
   {
      auto const m = cat::narrow_cast<cat::uint4>(cat::uint1{200_u1});
      cat::verify(m.has_value());
      cat::verify(m.value().raw == 200u);
   }
}

test(narrow_cast_narrowing_same_signedness_int8_to_int4) {
   cat::int4 const in_range{42};
   {
      auto const m = cat::narrow_cast<cat::int4>(cat::int8{in_range.raw});
      cat::verify(m.has_value());
      cat::verify(m.value() == in_range);
   }
   {
      auto const m =
         cat::narrow_cast<cat::int4>(cat::int8{cat::limits<cat::int4>::min()});
      cat::verify(m.has_value());
      cat::verify(m.value() == cat::limits<cat::int4>::min());
   }
   {
      auto const m =
         cat::narrow_cast<cat::int4>(cat::int8{cat::limits<cat::int4>::max()});
      cat::verify(m.has_value());
      cat::verify(m.value() == cat::limits<cat::int4>::max());
   }

   cat::iword const one_past_max =
      static_cast<cat::iword>(cat::limits<cat::int4>::max()) + 1_sz;
   cat::verify(
      !cat::narrow_cast<cat::int4>(cat::int8{one_past_max}).has_value());

   cat::iword const one_below_min =
      static_cast<cat::iword>(cat::limits<cat::int4>::min()) - 1_sz;
   cat::verify(
      !cat::narrow_cast<cat::int4>(cat::int8{one_below_min}).has_value());
}

test(narrow_cast_mixed_width_signed_to_unsigned) {
   // `int4` and `uint4` are the same width but opposite signed-ness. Needs the
   // `raw_mixed_integral_spaceship` path. Negative `int4` must not bit-cast
   // into a huge `uint4`.
   cat::verify(!cat::narrow_cast<cat::uint4>(cat::int4{-1}).has_value());
   cat::verify(
      !cat::narrow_cast<cat::uint4>(cat::int4{cat::limits<cat::int4>::min()})
          .has_value());

   {
      auto const m = cat::narrow_cast<cat::uint4>(cat::int4{0});
      cat::verify(m.has_value() && m.value().raw == 0u);
   }
   {
      auto const m = cat::narrow_cast<cat::uint4>(cat::int4{42});
      cat::verify(m.has_value() && m.value().raw == 42u);
   }
   {
      // Largest positive int32 is inside uint32. Must not use naive < on
      // unsigned/signed (would reject valid positives).
      auto const m = cat::narrow_cast<cat::uint4>(cat::int4{0x7FFFFFFF_i4});
      cat::verify(m.has_value() && m.value().raw == 0x7FFFFFFFu);
   }
}

test(narrow_cast_mixed_width_unsigned_to_signed) {
   // Value above `int4::max` must be rejected without mis-promoting
   // `limits<int4>::min()` in an unsigned/signed compare.
   cat::verify(
      !cat::narrow_cast<cat::int4>(cat::uint4{0x80000000u}).has_value());
   cat::verify(
      !cat::narrow_cast<cat::int4>(cat::uint4{0xFFFFFFFFu}).has_value());

   {
      auto const m = cat::narrow_cast<cat::int4>(cat::uint4{0u});
      cat::verify(m.has_value() && m.value() == 0_i4);
   }
   {
      auto const m = cat::narrow_cast<cat::int4>(cat::uint4{0x7FFFFFFFu});
      cat::verify(m.has_value() && m.value() == 0x7FFFFFFF_i4);
   }
}

test(narrow_cast_large_integrals_toward_smaller) {
   cat::verify(
      !cat::narrow_cast<cat::int4>(cat::iword{1_sz} << 32u).has_value());
   {
      auto const m = cat::narrow_cast<cat::int4>(cat::iword{42});
      cat::verify(m.has_value() && m.value() == 42_i4);
   }
}

test(narrow_cast_iword_to_uword_and_back) {
   // Same `sizeof` but not `is_safe_arithmetic_comparison`. Must still reject
   // negative `iword` and accept non-negative values that fit.
   cat::verify(!cat::narrow_cast<cat::uword>(cat::iword{-1}).has_value());
   cat::verify(
      !cat::narrow_cast<cat::uword>(cat::iword{cat::limits<cat::iword>::min()})
          .has_value());

   {
      auto const m = cat::narrow_cast<cat::uword>(cat::iword{0});
      cat::verify(m.has_value() && m.value() == 0_uz);
   }
   {
      auto const m = cat::narrow_cast<cat::uword>(cat::iword{1'000});
      cat::verify(m.has_value() && m.value() == 1'000_uz);
   }
}

test(narrow_cast_to_idx_is_maybe_of_idx) {
   {
      auto const m = cat::narrow_cast<cat::idx>(cat::uword{0_uz});
      cat::verify(m.has_value());
      cat::verify(m.value() == cat::idx{0u});
   }
   {
      // In-range: top bit clear.
      auto const m = cat::narrow_cast<cat::idx>(cat::uword{1_uz << 62u});
      cat::verify(m.has_value());
   }
   {
      // `idx` is 63 bits of magnitude. 2^63 and above set bit 63 => out of
      // range and must be empty (also exercises `raw_mixed_integral_spaceship`
      // for unsigned vs `limits<idx>::max()`).
      cat::verify(
         !cat::narrow_cast<cat::idx>(cat::uword{1_uz << 63u}).has_value());
   }
   {
      // Full `uword` is far above `idx` max.
      cat::verify(
         !cat::narrow_cast<cat::idx>(cat::uword{0xFFFFFFFF'FFFFFFFF_uz})
             .has_value());
   }
   {
      // Negative `iword` is below `idx` min (0).
      cat::verify(!cat::narrow_cast<cat::idx>(cat::iword{-1}).has_value());
   }
   {
      auto const m = cat::narrow_cast<cat::idx>(cat::iword{42});
      cat::verify(m.has_value() && m.value().raw == 42u);
   }
}

test(narrow_cast_fundamental_char_and_small_unsigned) {
   {
      auto const m = cat::narrow_cast<cat::int4>(static_cast<char>(-5));
      cat::verify(m.has_value() && m.value().raw == -5);
   }
   {
      cat::verify(
         !cat::narrow_cast<cat::uint1>(cat::uword{300_uz}).has_value());
   }
   {
      auto const m = cat::narrow_cast<cat::uint1>(cat::uword{0_uz});
      cat::verify(m.has_value() && m.value().raw == 0u);
   }
   {
      auto const m = cat::narrow_cast<cat::uint1>(cat::uword{255_uz});
      cat::verify(m.has_value() && m.value().raw == 255u);
   }
}
