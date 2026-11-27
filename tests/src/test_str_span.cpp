#include <cat/simd>
#include <cat/string>

#include "../unit_tests.hpp"

// `cat::basic_str_span::find` does its own bounded SIMD scan (`size`-bounded,
// no over-read), so these tests can use small inputs as long as the haystack
// is constructed without going through SIMD `string_length`. Each haystack is
// a `basic_str_span{ptr, len}` over a stack array.
//
// Every character type runs the same `find` fixtures through
// `verify_find<CharT>()`, so a width-specific kernel in `cat::find_memory` or
// `cat::find_subspan_memory` cannot drift from the others.

namespace {

// An ASCII string widened into a `CharT` array.
template <typename CharT>
struct widened {
   CharT data[256]{};
   idx size = 0u;

   widened(char const* p_ascii) {
      while (p_ascii[size.raw] != '\0') {
         data[size.raw] = static_cast<CharT>(p_ascii[size.raw]);
         ++size;
      }
   }

   operator cat::basic_str_span<CharT const>() const {
      return {data, size};
   }
};

template <typename CharT>
void
verify_find() {
   using view = cat::basic_str_span<CharT const>;
   using text = widened<CharT>;
   idx const lanes = cat::native_simd<CharT>::size();

   // Single characters, misses, and `from_position`.
   text const abcabc = "abcabc";
   view const short_haystack = abcabc;
   cat::verify(short_haystack.find(CharT('a')).value() == 0u);
   cat::verify(short_haystack.find(CharT('b')).value() == 1u);
   cat::verify(short_haystack.find(CharT('c')).value() == 2u);
   cat::verify(short_haystack.find(CharT('z')).is_empty());
   cat::verify(short_haystack.find(CharT('a'), 1u).value() == 3u);
   cat::verify(short_haystack.find(CharT('b'), 2u).value() == 4u);
   cat::verify(short_haystack.find(CharT('c'), 5u).value() == 5u);
   cat::verify(short_haystack.find(CharT('a'), 6u).is_empty());

   // A character past the first SIMD chunk.
   CharT chunked[256]{};
   for (idx i = 0u; i < lanes; ++i) {
      chunked[i.raw] = CharT('a');
   }
   idx const past_first_chunk = lanes + 3u;
   chunked[past_first_chunk.raw] = CharT('Z');
   view const chunked_haystack{chunked, past_first_chunk + 1u};
   cat::verify(chunked_haystack.find(CharT('Z')).value() == past_first_chunk);
   cat::verify(chunked_haystack.find(CharT('q')).is_empty());

   // A character in the scalar tail after whole SIMD chunks.
   CharT tail[256]{};
   idx const tail_index = lanes * 2u + 1u;
   for (idx i = 0u; i < tail_index; ++i) {
      tail[i.raw] = CharT('.');
   }
   tail[tail_index.raw] = CharT('x');
   view const tail_haystack{tail, tail_index + 1u};
   cat::verify(tail_haystack.find(CharT('x')).value() == tail_index);

   // An element whose low byte is zero would falsely match a needle of zero
   // under a naive per-element "has zero byte" SWAR. Only the real zero
   // element may match.
   if constexpr (sizeof(CharT) > 1u) {
      CharT const high_bytes[3] = {
         static_cast<CharT>(0x0100), static_cast<CharT>(0x0000),
         static_cast<CharT>(0xABCD)
      };
      view const high_haystack{high_bytes, 3u};
      cat::verify(high_haystack.find(static_cast<CharT>(0x0000)).value() == 1u);
      cat::verify(high_haystack.find(static_cast<CharT>(0xABCD)).value() == 2u);
   }

   // Substrings, including the empty needle.
   text const fox = "the quick brown fox";
   view const fox_haystack = fox;
   cat::verify(fox_haystack.find(text("quick")).value() == 4u);
   cat::verify(fox_haystack.find(text("fox")).value() == 16u);
   cat::verify(fox_haystack.find(text("the")).value() == 0u);
   cat::verify(fox_haystack.find(text("missing")).is_empty());
   cat::verify(fox_haystack.find(text("")).value() == 0u);

   // A needle longer than the haystack.
   text const abc = "abc";
   view const abc_haystack = abc;
   cat::verify(abc_haystack.find(text("abcd")).is_empty());
   cat::verify(abc_haystack.find(text("abc")).value() == 0u);

   // A one character needle delegates to the character search.
   cat::verify(short_haystack.find(text("c")).value() == 2u);
   cat::verify(short_haystack.find(text("z")).is_empty());

   // Short needles with overlapping candidates.
   text const abab = "ababcabab";
   view const abab_haystack = abab;
   cat::verify(abab_haystack.find(text("ab")).value() == 0u);
   cat::verify(abab_haystack.find(text("ba")).value() == 1u);
   cat::verify(abab_haystack.find(text("aba")).value() == 0u);
   cat::verify(abab_haystack.find(text("bab")).value() == 1u);
   cat::verify(abab_haystack.find(text("cab")).value() == 4u);

   // A needle whose prefix repeats one element.
   text const repeated = "aaaaaaaaaaaaaaaaaaaaaaaaabaaaaaaaaaaaaaaaaaaab";
   view const repeated_haystack = repeated;
   cat::verify(repeated_haystack.find(text("aab")).value() == 23u);
   cat::verify(repeated_haystack.find(text("aaaab")).value() == 21u);

   // Candidates that pass the filter but fail verification.
   text const false_positives = "axc axc abc";
   view const false_haystack = false_positives;
   cat::verify(false_haystack.find(text("abc")).value() == 8u);

   // Substrings from a position.
   text const triple = "abc abc abc";
   view const triple_haystack = triple;
   cat::verify(triple_haystack.find(text("abc"), 0u).value() == 0u);
   cat::verify(triple_haystack.find(text("abc"), 1u).value() == 4u);
   cat::verify(triple_haystack.find(text("abc"), 5u).value() == 8u);
   cat::verify(triple_haystack.find(text("abc"), 9u).is_empty());
   cat::verify(triple_haystack.find(text(""), 11u).value() == 11u);
   cat::verify(triple_haystack.find(text("a"), 12u).is_empty());

   // A substring that ends the haystack after several SIMD chunks.
   CharT end[256]{};
   idx const fill_end = lanes * 4u;
   for (idx i = 0u; i < fill_end; ++i) {
      end[i.raw] = CharT('.');
   }
   idx const start = idx(fill_end - 2u);
   end[start.raw] = CharT('e');
   end[(start + 1u).raw] = CharT('n');
   end[(start + 2u).raw] = CharT('d');
   view const end_haystack{end, start + 3u};
   cat::verify(end_haystack.find(text("end")).value() == start);

   // Needles at the length > 8 boundary of the 1-byte anomaly heuristic.
   text const eight = "abcdefgh";
   view const eight_haystack = eight;
   cat::verify(eight_haystack.find(text("abcdefgh")).value() == 0u);
   cat::verify(eight_haystack.find(text("abcdefghi")).is_empty());
}

// The 1-byte anomaly selector skips UTF-8 leading bytes (`> 191`) for needles
// longer than 8 bytes, so the filter anchors on ASCII or continuation bytes
// rather than a low-entropy rune prefix. Wider characters never take that
// path, so only the byte types run these fixtures.
template <typename CharT>
void
verify_find_utf8() {
   using view = cat::basic_str_span<CharT const>;

   // Each Devanagari codepoint is a 3-byte UTF-8 sequence, a leading byte in
   // `0xE0..0xE4` then two `0x80..0xBF` continuation bytes.
   cat::u8str_view const devanagari = u8"नममगबदलविद्यारतीयभारतीयसंस्कृत";
   view const haystack{
      reinterpret_cast<CharT const*>(devanagari.data()), devanagari.size()
   };

   // A 6-byte needle of two runes, under the length > 8 threshold.
   cat::u8str_view const na_ma = u8"नम";
   cat::verify(
      haystack
         .find(view{reinterpret_cast<CharT const*>(na_ma.data()), na_ma.size()})
         .value()
      == 0u
   );

   // A 9-byte needle of three runes, over the length > 8 threshold.
   cat::u8str_view const ma_ga_ba = u8"मगब";
   cat::verify(
      haystack
         .find(
            view{
               reinterpret_cast<CharT const*>(ma_ga_ba.data()), ma_ga_ba.size()
            }
         )
         .value()
      == 6u
   );
   cat::verify(haystack.find(widened<CharT>("missing")).is_empty());

   // "abÀabÀq" where À (U+00C0) is the 2-byte rune `0xC3 0x80`. The needle
   // starts on the leading byte and must match at the correct byte offset.
   cat::u8str_view const grave_text = u8"abÀabÀq";
   view const grave_haystack{
      reinterpret_cast<CharT const*>(grave_text.data()), grave_text.size()
   };
   cat::u8str_view const a_grave = u8"À";
   cat::verify(
      grave_haystack
         .find(
            view{reinterpret_cast<CharT const*>(a_grave.data()), a_grave.size()}
         )
         .value()
      == 2u
   );
}

}  // namespace

$test(str_span_find_char) {
   verify_find<char>();
   verify_find_utf8<char>();
}

$test(str_span_find_char8) {
   verify_find<char8_t>();
   verify_find_utf8<char8_t>();
}

$test(str_span_find_char16) {
   verify_find<char16_t>();
}

$test(str_span_find_char32) {
   verify_find<char32_t>();
}

$test(str_span_find_wchar) {
   verify_find<wchar_t>();
}

static_assert(cat::is_nothrow_default_constructible<cat::str_view>);
static_assert(cat::is_nothrow_copy_assignable<cat::str_view>);

static_assert(cat::is_whitespace(' '));
static_assert(cat::is_whitespace('\t'));
static_assert(cat::is_whitespace('\n'));
static_assert(cat::is_whitespace('\v'));
static_assert(cat::is_whitespace('\f'));
static_assert(cat::is_whitespace('\r'));
static_assert(cat::is_whitespace(u8' '));
static_assert(!cat::is_whitespace('a'));
static_assert(!cat::is_whitespace('\0'));
static_assert(!cat::is_whitespace('\x0e'));
