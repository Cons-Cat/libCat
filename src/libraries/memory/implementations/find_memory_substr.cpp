#include <cat/arithmetic>
#include <cat/maybe>
#include <cat/memory>
#include <cat/simd>
#include <cat/simd_switch>

namespace cat {

namespace {

struct needle_anomalies {
   idx first;
   idx mid;
   idx last;
};

template <typename Raw>
[[nodiscard]]
auto
load_element(byte const* p_data, idx position) -> Raw {
   Raw value;
   __builtin_memcpy_inline(
      &value, p_data + position * sizeof(Raw), sizeof(Raw)
   );
   return value;
}

template <typename Raw>
[[nodiscard]]
auto
locate_needle_anomalies(byte const* p_needle, idx length) -> needle_anomalies {
   needle_anomalies a{
      .first = 0u,
      .mid = length / 2u,
      .last = idx(length - 1u),
   };
   if (length <= 3u) {
      return a;
   }

   Raw const first = load_element<Raw>(p_needle, a.first);
   if (first == load_element<Raw>(p_needle, a.mid)) {
      while (a.mid < a.last && load_element<Raw>(p_needle, a.mid) == first) {
         ++a.mid;
      }
   }
   Raw const mid = load_element<Raw>(p_needle, a.mid);
   if (
      load_element<Raw>(p_needle, a.last) == first
      || load_element<Raw>(p_needle, a.last) == mid
   ) {
      while (
         a.last > a.mid
         && (load_element<Raw>(p_needle, a.last) == first || load_element<Raw>(p_needle, a.last) == mid)) {
         a.last = idx(a.last - 1u);
      }
   }

   if constexpr (sizeof(Raw) == 1u) {
      if (length <= 8u) {
         return a;
      }

      Raw const last = load_element<Raw>(p_needle, a.last);
      idx vibrant_mid = a.mid;
      while ((load_element<Raw>(p_needle, vibrant_mid) > 191u
              || load_element<Raw>(p_needle, vibrant_mid) == last)
             && vibrant_mid + 1u < a.last) {
         ++vibrant_mid;
      }
      if (load_element<Raw>(p_needle, vibrant_mid) <= 191u) {
         a.mid = vibrant_mid;
      }

      Raw const vibrant_mid_value = load_element<Raw>(p_needle, a.mid);
      idx vibrant_first = a.first;
      while ((load_element<Raw>(p_needle, vibrant_first) > 191u
              || load_element<Raw>(p_needle, vibrant_first) == vibrant_mid_value
              || load_element<Raw>(p_needle, vibrant_first) == last)
             && vibrant_first + 1u < a.mid) {
         ++vibrant_first;
      }
      if (load_element<Raw>(p_needle, vibrant_first) <= 191u) {
         a.first = vibrant_first;
      }
   }
   return a;
}

[[nodiscard]]
inline auto
subspan_matches(byte const* p_haystack, byte const* p_needle, idx bytes)
   -> bool {
   return compare_memory_scalar(p_haystack, p_needle, bytes)
          == std::strong_ordering::equal;
}

template <typename Simd>
[[nodiscard, gnu::always_inline]]
auto
subspan_matches_simd(byte const* p_haystack, byte const* p_needle, idx bytes)
   -> bool {
   using byte_simd = rebind_simd<unsigned char, Simd>;
   constexpr idx lanes = byte_simd::size();

   idx position = 0u;
   while (bytes - position >= lanes) {
      byte_simd haystack;
      byte_simd needle;
      haystack.load(
         reinterpret_cast<unsigned char const*>(p_haystack + position)
      );
      needle.load(reinterpret_cast<unsigned char const*>(p_needle + position));
      if (!haystack.equal_lanes(needle).all_of()) {
         return false;
      }
      position += lanes;
   }
   return subspan_matches(
      p_haystack + position, p_needle + position, idx(bytes - position)
   );
}

[[nodiscard]]
auto
find_byte_tail(byte const* p_haystack, idx size, unsigned char needle)
   -> maybe<idx> {
   constexpr uword broadcast = 0x01010101'01010101ull;
   constexpr uword high_bits = 0x80808080'80808080ull;
   uword const needle_word = broadcast * needle;
   idx position = 0u;
   while (size - position >= 8u) {
      uword chunk;
      __builtin_memcpy_inline(&chunk, p_haystack + position, 8u);
      uword diff = chunk ^ needle_word;
      uword has_zero = (diff - broadcast) & ~diff & high_bits;
      if (has_zero != 0u) {
         return position + has_zero.countr_zero() / 8u;
      }
      position += 8u;
   }
   for (; position < size; ++position) {
      if (load_element<unsigned char>(p_haystack, position) == needle) {
         return position;
      }
   }
   return nullopt;
}

template <typename Simd>
[[nodiscard]]
auto
find_subspan_byte_tail(
   byte const* p_haystack, byte const* p_needle, idx size, idx needle_size,
   needle_anomalies anomalies
) -> maybe<idx> {
   idx const candidate_count = idx(size - needle_size) + 1u;
   unsigned char const anchor =
      load_element<unsigned char>(p_needle, anomalies.last);
   idx search_position = 0u;
   while (search_position < candidate_count) {
      maybe<idx> const hit = find_byte_tail(
         p_haystack + search_position + anomalies.last,
         idx(candidate_count - search_position), anchor
      );
      if (hit.is_empty()) {
         return nullopt;
      }

      idx const candidate = search_position + hit.value();
      byte const* p_match = p_haystack + candidate;
      if (
         load_element<unsigned char>(p_match, anomalies.first)
            == load_element<unsigned char>(p_needle, anomalies.first)
         && load_element<unsigned char>(p_match, anomalies.mid)
               == load_element<unsigned char>(p_needle, anomalies.mid)
         && (needle_size != 4u || load_element<unsigned char>(p_match, 1u) == load_element<unsigned char>(p_needle, 1u))
         && (needle_size <= 4u || subspan_matches_simd<Simd>(p_match, p_needle, needle_size))
      ) {
         return candidate;
      }
      search_position = candidate + 1u;
   }
   return nullopt;
}

template <typename Simd, typename Raw>
[[nodiscard, gnu::always_inline]]
auto
find_subspan_large(
   byte const* p_haystack, byte const* p_needle, idx size, idx needle_size
) -> maybe<idx> {
   using simd_type = Simd;
   constexpr idx lanes = simd_type::size();
   using lane_type = simd_type::memory_lane;
   idx const needle_bytes = needle_size * sizeof(Raw);
   needle_anomalies const a =
      locate_needle_anomalies<Raw>(p_needle, needle_size);

   simd_type first_vec;
   simd_type mid_vec;
   simd_type last_vec;
   simd_type second_vec;
   first_vec.fill(load_element<Raw>(p_needle, a.first));
   mid_vec.fill(load_element<Raw>(p_needle, a.mid));
   last_vec.fill(load_element<Raw>(p_needle, a.last));
   if (needle_size == 4u) {
      second_vec.fill(load_element<Raw>(p_needle, 1u));
   }

   lane_type const* p_current =
      __builtin_bit_cast(lane_type const*, p_haystack);
   idx position = 0u;
#pragma unroll 2
   while (size - position >= needle_size + lanes - 1u) {
      simd_type h_first;
      simd_type h_mid;
      simd_type h_last;
      h_first.load(p_current + a.first);
      h_mid.load(p_current + a.mid);
      h_last.load(p_current + a.last);

      auto candidate_mask = h_first.equal_lanes(first_vec)
                            & h_mid.equal_lanes(mid_vec)
                            & h_last.equal_lanes(last_vec);
      if (needle_size == 4u) {
         simd_type h_second;
         h_second.load(p_current + 1u);
         candidate_mask &= h_second.equal_lanes(second_vec);
      }
      uword const candidates = candidate_mask.to_uword();
      if (candidates == 0u) {
         position += lanes;
         p_current += lanes;
         continue;
      }

      if (needle_size <= 4u) {
         return position + candidates.countr_zero();
      }

      for (uword bits = candidates; bits != 0u; bits &= bits - 1u) {
         idx const offset = bits.countr_zero();
         if (
            subspan_matches_simd<Simd>(
               p_haystack + (position + offset) * sizeof(Raw), p_needle,
               needle_bytes
            )
         ) {
            return position + offset;
         }
      }

      position += lanes;
      p_current += lanes;
   }

   if constexpr (sizeof(Raw) == 1u) {
      return position
             + $prop(
                find_subspan_byte_tail<Simd>(
                   p_haystack + position, p_needle, idx(size - position),
                   needle_size, a
                )
             );
   } else {
      for (idx offset = 0u; offset + needle_size <= size - position; ++offset) {
         byte const* p_candidate =
            p_haystack + (position + offset) * sizeof(Raw);
         if (
            load_element<Raw>(p_candidate, a.first)
               != load_element<Raw>(p_needle, a.first)
            || load_element<Raw>(p_candidate, a.mid)
                  != load_element<Raw>(p_needle, a.mid)
            || load_element<Raw>(p_candidate, a.last)
                  != load_element<Raw>(p_needle, a.last)
            || (needle_size == 4u && load_element<Raw>(p_candidate, 1u) != load_element<Raw>(p_needle, 1u))
         ) {
            continue;
         }
         if (
            needle_size <= 4u
            || subspan_matches(p_candidate, p_needle, needle_bytes)
         ) {
            return position + offset;
         }
      }
      return nullopt;
   }
}

template <typename Raw>
[[nodiscard]]
auto
find_subspan_simd(
   byte const* p_haystack, byte const* p_needle, idx size, idx needle_size
) -> maybe<idx> {
   return $simd_switch($abi((avx512, avx2, sse2), {
      return find_subspan_large<native_simd<Raw>, Raw>(
         p_haystack, p_needle, size, needle_size
      );
   }));
}

}  // namespace

[[nodiscard]]
auto
detail::find_subspan_memory_impl(
   byte const* _Nonnull p_haystack, byte const* _Nonnull p_needle,
   idx element_bytes, idx size, idx needle_size
) -> maybe<idx> {
   if (needle_size == 1u) {
      return detail::find_memory_impl(
         p_haystack, p_needle, element_bytes, size
      );
   }

   switch (element_bytes.raw) {
      case 1u:
         return find_subspan_simd<unsigned char>(
            p_haystack, p_needle, size, needle_size
         );
      case 2u:
         return find_subspan_simd<__UINT16_TYPE__>(
            p_haystack, p_needle, size, needle_size
         );
      case 4u:
         return find_subspan_simd<__UINT32_TYPE__>(
            p_haystack, p_needle, size, needle_size
         );
      case 8u:
         return find_subspan_simd<__UINT64_TYPE__>(
            p_haystack, p_needle, size, needle_size
         );
      default:
         idx const needle_bytes = needle_size * element_bytes;
         for (idx position = 0u; position <= size - needle_size; ++position) {
            if (
               subspan_matches(
                  p_haystack + position * element_bytes, p_needle, needle_bytes
               )
            ) {
               return maybe<idx>{position};
            }
         }
         return nullopt;
   }
}

}  // namespace cat
