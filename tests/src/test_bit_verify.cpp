// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations and the constant argument checks in
// `<cat/bit>`. The `*_unchecked` checks are in `test_arithmetic_verify.cpp`.
// Run with `just test safety`.

#include <cat/array>
#include <cat/bit>

using namespace cat::literals;
using namespace cat::arithmetic;

using words_type = cat::array<uint1, 4u>;

auto
borrow_bits() -> cat::bit_span<uint1>;

// Positive cases: a temporary view's bits outlive the view.

auto
valid_temporary_subscript() -> cat::bit_reference<uint1> {
   return borrow_bits()[0u];
}

auto
valid_temporary_iterator() -> cat::bit_stepanov_iterator<
   cat::detail::container_stepanov_iterator<uint1>> {
   return borrow_bits().begin();
}

auto
valid_temporary_dereference() -> cat::bit_reference<uint1> {
   return *borrow_bits().begin();
}

auto
valid_temporary_slice() -> cat::bit_span<uint1> {
   return borrow_bits().slice(0u, 1u);
}

auto
valid_bit_ptr(uint1& word [[clang::lifetimebound]])
   -> cat::bit_reference<uint1> {
   cat::bit_ptr<uint1> p_bits(&word);
   return *p_bits;
}

// Negative cases: `bit_span` and its iterators outlive their words.

auto
dangling_bit_span() -> cat::bit_span<uint1> {
   words_type words{};
   // expected-warning@+1 2 {{local variable 'words'}}
   return cat::bit_span<uint1>(words);
}

auto
dangling_bit_span_slice() -> cat::bit_span<uint1> {
   words_type words{};
   // expected-warning@+1 {{local variable 'words' is returned}}
   cat::bit_span<uint1> bits(words);
   return bits.slice(0u, 8u);
}

auto
dangling_bit_span_subscript() -> cat::bit_reference<uint1> {
   words_type words{};
   // expected-warning@+1 {{local variable 'words' is returned}}
   cat::bit_span<uint1> bits(words);
   return bits[0u];
}

auto
dangling_bit_iterator() -> cat::bit_stepanov_iterator<
   cat::detail::container_stepanov_iterator<uint1>> {
   words_type words{};
   // expected-warning@+1 {{local variable 'words' is returned}}
   cat::bit_span<uint1> bits(words);
   return bits.begin();
}

auto
dangling_bit_iterator_dereference() -> cat::bit_reference<uint1> {
   words_type words{};
   // expected-warning@+1 {{local variable 'words' is returned}}
   cat::bit_span<uint1> bits(words);
   return *bits.begin();
}

void
use_after_scope_bit_span() {
   cat::bit_span<uint1> bits;
   {
      words_type words{};
      // expected-warning@+1 {{local variable 'words' does not live long}}
      bits = cat::bit_span<uint1>(words);
   }
   bits[0u] = true;
}

// Negative cases: `bit_ptr` and `bit_reference` outlive their word.

auto
dangling_bit_ptr() -> cat::bit_ptr<uint1> {
   uint1 word = 0u;
   // expected-warning@+1 2 {{local variable 'word'}}
   return cat::bit_ptr<uint1>(&word);
}

auto
dangling_bit_ptr_dereference() -> cat::bit_reference<uint1> {
   uint1 word = 0u;
   // expected-warning@+1 {{local variable 'word' is returned}}
   cat::bit_ptr<uint1> p_bits(&word);
   return *p_bits;
}

auto
dangling_bit_ptr_subscript() -> cat::bit_reference<uint1> {
   uint1 word = 0u;
   // expected-warning@+1 {{local variable 'word' is returned}}
   cat::bit_ptr<uint1> p_bits(&word);
   return p_bits[1];
}

auto
dangling_bit_reference() -> cat::bit_reference<uint1> {
   uint1 word = 0u;
   // expected-warning@+1 2 {{local variable 'word'}}
   return cat::make_bit_reference_from_mask(word, 1u);
}

// Constant argument checks. These must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit.

void
bit_constant_arguments() {
   auto _ = cat::funnel_shift_left(1_u1, 2_u1, 7)
            + cat::funnel_shift_right(1_u1, 2_u1, 7);

   // expected-error@cat/bit:* 2 {{Shift is out of range!}}
   cat::funnel_shift_left(1_u1, 2_u1, 8);
   cat::funnel_shift_right(1_u1, 2_u1, -1);
}
