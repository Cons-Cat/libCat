#include <cat/simd>
#include <cat/string>

#include "../unit_tests.hpp"

// `cat::basic_str_span::find` does its own bounded SIMD scan (`size`-bounded,
// no over-read), so its tests can use small inputs as long as the underlying
// `str_view` is constructed without going through SIMD `string_length`. The
// fixtures below feed `find` a `str_view{ptr, len}` over a stack array.

$test(str_view_find_past_first_simd_chunk) {
   idx const lanes = cat::char1x16::size();

   char buffer[128]{};
   for (idx i = 0u; i < lanes; ++i) {
      buffer[i.raw] = 'a';
   }
   idx const past_first_chunk = lanes + 7u;
   buffer[past_first_chunk.raw] = 'Z';

   cat::str_view const haystack_past_chunk{buffer, past_first_chunk + 1u};
   cat::verify(haystack_past_chunk.find('Z').value() == past_first_chunk);
   cat::verify(haystack_past_chunk.find('q').is_empty());

   char buffer2[128]{};
   buffer2[4] = 'm';
   cat::str_view const haystack_first_chunk(buffer2, 48u);
   cat::verify(haystack_first_chunk.find('m').value() == 4);
}

$test(str_view_over_str_inplace_find_single_chars) {
   constexpr cat::str_inplace<7> hello = "Hello, ";
   cat::str_view const hello_view = hello;
   idx const h = hello_view.find('H').value();
   idx const e = hello_view.find('e').value();
   idx const l = hello_view.find('l').value();
   idx const o = hello_view.find('o').value();
   cat::verify(h == 0);
   cat::verify(e == 1);
   cat::verify(l == 2);
   cat::verify(o == 4);
}

$test(str_view_find_char_dispatched_width) {
   idx const lanes = cat::native_simd<char>::size();
   cat::verify(cat::native_simd<char>::size() >= 16u);

   char buffer[256]{};
   idx const past_first_chunk = lanes + 9u;
   buffer[past_first_chunk] = 'Q';
   cat::str_view const haystack{buffer, past_first_chunk + 1u};
   cat::verify(haystack.find('Q').value() == past_first_chunk);
   cat::verify(haystack.find('!').is_empty());

   char tail[256]{};
   idx const tail_size = lanes + 3u;
   tail[tail_size - 1u] = 'x';
   cat::str_view const tail_view{tail, tail_size};
   cat::verify(tail_view.find('x').value() == idx(tail_size - 1u));
}

$test(str_view_find_substr_basic) {
   cat::str_view const haystack = "the quick brown fox";
   cat::verify(haystack.find(cat::str_view{"quick"}).value() == 4);
   cat::verify(haystack.find(cat::str_view{"fox"}).value() == 16);
   cat::verify(haystack.find(cat::str_view{"the"}).value() == 0);
   cat::verify(haystack.find(cat::str_view{"missing"}).is_empty());
   cat::verify(haystack.find(cat::str_view{""}).value() == 0);
}

$test(str_view_find_substr_needle_longer_than_haystack) {
   cat::str_view const haystack = "abc";
   cat::verify(haystack.find(cat::str_view{"abcd"}).is_empty());
   cat::verify(haystack.find(cat::str_view{"abc"}).value() == 0);
}

$test(str_view_find_substr_single_char_delegates) {
   cat::str_view const haystack = "abcdef";
   cat::verify(haystack.find(cat::str_view{"d"}).value() == 3);
   cat::verify(haystack.find(cat::str_view{"z"}).is_empty());
}

$test(str_view_find_substr_short_needles) {
   cat::str_view const haystack = "ababcabab";
   cat::verify(haystack.find(cat::str_view{"ab"}).value() == 0);
   cat::verify(haystack.find(cat::str_view{"ba"}).value() == 1);
   cat::verify(haystack.find(cat::str_view{"aba"}).value() == 0);
   cat::verify(haystack.find(cat::str_view{"bab"}).value() == 1);
   cat::verify(haystack.find(cat::str_view{"cab"}).value() == 4);
}

$test(str_view_find_substr_repeated_prefix_byte) {
   cat::str_view const haystack =
      "aaaaaaaaaaaaaaaaaaaaaaaaabaaaaaaaaaaaaaaaaaaab";
   cat::verify(haystack.find(cat::str_view{"aab"}).value() == 23);
   cat::verify(haystack.find(cat::str_view{"aaaab"}).value() == 21);
}

$test(str_view_find_substr_match_at_tail) {
   idx const lanes = cat::native_simd<char>::size();
   char buffer[256]{};
   idx const fill_end = lanes * 4u;
   for (idx i = 0u; i < fill_end; ++i) {
      buffer[i] = '.';
   }
   idx const start = idx(fill_end - 2u);
   buffer[start] = 'e';
   buffer[start + 1u] = 'n';
   buffer[start + 2u] = 'd';
   cat::str_view const haystack{buffer, start + 3u};
   cat::verify(haystack.find(cat::str_view{"end"}).value() == start);
}

$test(str_view_find_substr_false_positives_verified) {
   cat::str_view const haystack = "axc axc abc";
   cat::verify(haystack.find(cat::str_view{"abc"}).value() == 8);
}

$test(str_view_find_substr_from_position) {
   cat::str_view const haystack = "abc abc abc";
   cat::verify(haystack.find(cat::str_view{"abc"}, 0u).value() == 0);
   cat::verify(haystack.find(cat::str_view{"abc"}, 1u).value() == 4);
   cat::verify(haystack.find(cat::str_view{"abc"}, 5u).value() == 8);
   cat::verify(haystack.find(cat::str_view{"abc"}, 9u).is_empty());
}

// UTF-8 Devanagari. Each Devanagari codepoint is a 3-byte UTF-8
// sequence: a leading byte in `0xE0..0xE4` followed by two
// `0x80..0xBF` continuation bytes. The anomaly selector's
// UTF-8 heuristic shifts `first`/`mid` right past leading bytes
// (`> 191`) for needles longer than 8 bytes, so the filter anchors on
// ASCII or continuation bytes rather than a low-entropy rune prefix.
$test(str_view_find_substr_utf8_devanagari) {
   // "नम" = U+0928 U+092E U+092E in UTF-8 bytes.
   cat::u8str_view const utf8_text = u8"नममगबदलविद्यारतीयभारतीयसंस्कृत";
   cat::str_view const haystack = utf8_text;

   // A 6-byte needle (under the length > 8 threshold) of two runes.
   cat::u8str_view const na_ma = u8"नम";
   cat::str_view const needle = na_ma;
   cat::verify(haystack.find(needle).value() == 0);

   // A 9-byte needle (3 runes) over the length > 8 threshold.
   cat::u8str_view const ma_ga_ba = u8"मगब";
   cat::str_view const long_needle = ma_ga_ba;
   cat::verify(haystack.find(long_needle).value() == 6);
   cat::verify(haystack.find(cat::str_view{"missing"}).is_empty());
}

// A needle whose first byte is a UTF-8 continuation byte (`0x80`).
// The anomaly heuristic must shift `first` past it and still find the
// match at the correct byte offset.
$test(str_view_find_substr_utf8_continuation_first_byte) {
   // "abÀabÀq" where À (U+00C0) is the 2-byte UTF-8 rune 0xC3 0x80.
   cat::u8str_view const utf8_text = u8"abÀabÀq";
   cat::str_view const haystack = utf8_text;
   // Needle is the 2-byte rune À, first byte is a leading byte (0xC3).
   cat::u8str_view const a_grave = u8"À";
   cat::str_view const needle = a_grave;
   cat::verify(haystack.find(needle).value() == 2);
}

// Needles at the length > 8 boundary exercise the UTF-8 shift.
$test(str_view_find_substr_anomaly_length_boundary) {
   cat::str_view const haystack = "abcdefgh";
   cat::verify(haystack.find(cat::str_view{"abcdefgh"}).value() == 0);
   cat::verify(haystack.find(cat::str_view{"abcdefghi"}).is_empty());
}

// Wide-character `find` dispatches to the multi-byte SWAR/SIMD kernel in
// `cat::find_memory`. These fixtures cover `char16_t`, `char32_t`, and
// `wchar_t` (4 bytes on Linux), exercising hits at the start, middle, and tail
// of the haystack plus misses and `from_position` offsets. The haystacks are
// built from stack arrays wrapped in `basic_str_span{ptr, len}` so the SIMD
// scan stays bounded by `size` with no over-read.

$test(wstr_view_find_char16_basic) {
   char16_t const buffer[] = u"abcabc";
   cat::u16str_view const haystack{buffer, 6u};
   cat::verify(haystack.find(u'a').value() == 0);
   cat::verify(haystack.find(u'b').value() == 1);
   cat::verify(haystack.find(u'c').value() == 2);
   cat::verify(haystack.find(u'z').is_empty());
}

$test(wstr_view_find_char16_from_position) {
   char16_t const buffer[] = u"abcabc";
   cat::u16str_view const haystack{buffer, 6u};
   cat::verify(haystack.find(u'a', 1u).value() == 3u);
   cat::verify(haystack.find(u'b', 2u).value() == 4u);
   cat::verify(haystack.find(u'c', 5u).value() == 5u);
   cat::verify(haystack.find(u'a', 6u).is_empty());
}

$test(wstr_view_find_char16_past_first_simd_chunk) {
   idx const lanes = cat::native_simd<char16_t>::size();
   char16_t buffer[128]{};
   for (idx i = 0u; i < lanes; ++i) {
      buffer[i.raw] = u'a';
   }
   idx const past_first_chunk = lanes + 3u;
   buffer[past_first_chunk.raw] = u'Z';
   cat::u16str_view const haystack{buffer, past_first_chunk + 1u};
   cat::verify(haystack.find(u'Z').value() == past_first_chunk);
   cat::verify(haystack.find(u'q').is_empty());
}

$test(wstr_view_find_char16_match_at_tail) {
   idx const lanes = cat::native_simd<char16_t>::size();
   char16_t buffer[128]{};
   idx const tail = lanes * 2u + 1u;
   for (idx i = 0u; i < tail; ++i) {
      buffer[i.raw] = u'.';
   }
   buffer[tail.raw] = u'x';
   cat::u16str_view const haystack{buffer, tail + 1u};
   cat::verify(haystack.find(u'x').value() == tail);
}

$test(wstr_view_find_char16_high_bytes_no_false_positive) {
   // A 0x0100 halfword would falsely match a needle of 0x0000 under the naive
   // per-element "has zero halfword" SWAR. The byte-combined kernel must reject
   // it and only match the real 0x0000 element.
   char16_t buffer[8]{};
   buffer[0] = static_cast<char16_t>(0x0100);
   buffer[1] = static_cast<char16_t>(0x0000);
   buffer[2] = static_cast<char16_t>(0xABCD);
   cat::u16str_view const haystack{buffer, 3u};
   cat::verify(haystack.find(static_cast<char16_t>(0x0000)).value() == 1);
   cat::verify(haystack.find(static_cast<char16_t>(0xABCD)).value() == 2);
}

$test(wstr_view_find_char32_basic) {
   char32_t const buffer[] = U"abcabc";
   cat::u32str_view const haystack{buffer, 6u};
   cat::verify(haystack.find(U'a').value() == 0);
   cat::verify(haystack.find(U'b').value() == 1);
   cat::verify(haystack.find(U'c').value() == 2);
   cat::verify(haystack.find(U'z').is_empty());
}

$test(wstr_view_find_char32_from_position) {
   char32_t const buffer[] = U"abcabc";
   cat::u32str_view const haystack{buffer, 6u};
   cat::verify(haystack.find(U'a', 1u).value() == 3u);
   cat::verify(haystack.find(U'c', 5u).value() == 5u);
   cat::verify(haystack.find(U'a', 6u).is_empty());
}

$test(wstr_view_find_char32_past_first_simd_chunk) {
   idx const lanes = cat::native_simd<char32_t>::size();
   char32_t buffer[128]{};
   for (idx i = 0u; i < lanes; ++i) {
      buffer[i.raw] = U'a';
   }
   idx const past_first_chunk = lanes + 1u;
   buffer[past_first_chunk.raw] = U'Z';
   cat::u32str_view const haystack{buffer, past_first_chunk + 1u};
   cat::verify(haystack.find(U'Z').value() == past_first_chunk);
   cat::verify(haystack.find(U'q').is_empty());
}

$test(wstr_view_find_char32_match_at_tail) {
   idx const lanes = cat::native_simd<char32_t>::size();
   char32_t buffer[128]{};
   idx const tail = lanes * 2u + 1u;
   for (idx i = 0u; i < tail; ++i) {
      buffer[i.raw] = U'.';
   }
   buffer[tail.raw] = U'x';
   cat::u32str_view const haystack{buffer, tail + 1u};
   cat::verify(haystack.find(U'x').value() == tail);
}

$test(wstr_view_find_wchar_basic) {
   wchar_t const buffer[] = L"abcabc";
   cat::wstr_view const haystack{buffer, 6u};
   cat::verify(haystack.find(L'a').value() == 0);
   cat::verify(haystack.find(L'b').value() == 1);
   cat::verify(haystack.find(L'c').value() == 2);
   cat::verify(haystack.find(L'z').is_empty());
}

$test(wstr_view_find_wchar_from_position) {
   wchar_t const buffer[] = L"abcabc";
   cat::wstr_view const haystack{buffer, 6u};
   cat::verify(haystack.find(L'a', 1u).value() == 3u);
   cat::verify(haystack.find(L'c', 5u).value() == 5u);
   cat::verify(haystack.find(L'a', 6u).is_empty());
}

$test(wstr_view_find_wchar_past_first_simd_chunk) {
   idx const lanes = cat::native_simd<wchar_t>::size();
   wchar_t buffer[128]{};
   for (idx i = 0u; i < lanes; ++i) {
      buffer[i.raw] = L'a';
   }
   idx const past_first_chunk = lanes + 1u;
   buffer[past_first_chunk.raw] = L'Z';
   cat::wstr_view const haystack{buffer, past_first_chunk + 1u};
   cat::verify(haystack.find(L'Z').value() == past_first_chunk);
   cat::verify(haystack.find(L'q').is_empty());
}
