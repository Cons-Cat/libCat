#include <cat/arithmetic>
#include <cat/bit>
#include <cat/maybe>
#include <cat/memory>
#include <cat/propagate>
#include <cat/simd>
#include <cat/simd_switch>

namespace cat {

namespace {

template <typename Raw>
[[nodiscard, clang::no_builtin("memchr")]]
auto
load_element(byte const* p_data, idx position) -> Raw {
   Raw value;
   __builtin_memcpy_inline(
      &value, p_data + position * sizeof(Raw), sizeof(Raw)
   );
   return value;
}

[[nodiscard, clang::no_builtin("memchr")]]
auto
find_byte_small(byte const* p_haystack, idx size, unsigned char needle)
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
         return position + (has_zero.countr_zero() / 8u);
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

template <typename Raw>
[[nodiscard]]
auto
find_element_small(byte const* p_haystack, idx size, Raw needle) -> maybe<idx> {
   for (idx position = 0u; position < size; ++position) {
      if (load_element<Raw>(p_haystack, position) == needle) {
         return position;
      }
   }
   return nullopt;
}

template <typename Simd, typename Raw>
[[nodiscard, gnu::always_inline]]
auto
find_element_large(byte const* p_haystack, byte const* p_needle, idx size)
   -> maybe<idx> {
   using simd_type = Simd;
   using lane_type = simd_type::memory_lane;
   constexpr idx lanes = simd_type::size();
   Raw const needle = load_element<Raw>(p_needle, 0u);
   simd_type needles;
   needles.fill(needle);

   lane_type const* p_base = __builtin_bit_cast(lane_type const*, p_haystack);
   lane_type const* p_current = p_base;
   lane_type const* p_end = p_base + (size / lanes) * lanes;

#pragma unroll 4
   while (p_current < p_end) {
      simd_type chunk;
      chunk.load(p_current);
      auto const matches = chunk.equal_lanes(needles);
      if (matches.any_of()) {
         return idx(p_current - p_base) + matches.find_if_true();
      }
      p_current += lanes;
   }

   idx const processed = idx(p_current - p_base);
   byte const* p_tail = p_haystack + processed * sizeof(Raw);
   if constexpr (sizeof(Raw) == 1u) {
      return processed
             + $prop(find_byte_small(
                p_tail, idx(size - processed),
                static_cast<unsigned char>(needle)
             ));
   } else {
      return processed
             + $prop(find_element_small(p_tail, idx(size - processed), needle));
   }
}

template <typename Raw>
[[nodiscard]]
auto
find_element_simd(byte const* p_haystack, byte const* p_needle, idx size)
   -> maybe<idx> {
   return $simd_switch($abi((avx512, avx2, sse2), {
      return find_element_large<native_simd<Raw>, Raw>(
         p_haystack, p_needle, size
      );
   }));
}

template <typename Simd, idx element_bytes>
[[nodiscard, gnu::always_inline]]
auto
find_grouped_element_large(
   byte const* p_haystack, byte const* p_needle, idx size
) -> maybe<idx> {
   constexpr idx lanes = Simd::size();
   constexpr idx elements_per_chunk = lanes / element_bytes;
   constexpr idx loaded_elements =
      idx((lanes + element_bytes - 1u) / element_bytes);
   constexpr uword element_starts = [] {
      constexpr idx chunk_elements = Simd::size() / element_bytes;
      uword starts = 0u;
      for (idx element = 0u; element < chunk_elements; ++element) {
         starts |= uword{1u} << (element * element_bytes);
      }
      return starts;
   }();

   Simd needles;
   for (idx byte_index = 0u; byte_index < lanes; ++byte_index) {
      needles.set_lane(
         byte_index,
         static_cast<unsigned char>(p_needle[byte_index % element_bytes])
      );
   }

   idx position = 0u;
#pragma unroll 4
   while (size - position >= loaded_elements) {
      Simd chunk;
      chunk.load(
         reinterpret_cast<unsigned char const*>(
            p_haystack + position * element_bytes
         )
      );
      auto const matches = chunk.equal_lanes(needles);
      if (matches.none_of()) {
         position += elements_per_chunk;
         continue;
      }
      uword const bits = matches.to_uword();
      uword complete_elements = bits;
#pragma unroll
      for (idx byte_index = 1u; byte_index < element_bytes; ++byte_index) {
         complete_elements &= bits >> byte_index;
      }
      complete_elements &= element_starts;
      if (complete_elements != 0u) {
         return position + (complete_elements.countr_zero() / element_bytes);
      }
      position += elements_per_chunk;
   }

   for (; position < size; ++position) {
      if (
         compare_memory_scalar(
            p_haystack + position * element_bytes, p_needle, element_bytes
         )
         == std::strong_ordering::equal
      ) {
         return position;
      }
   }
   return nullopt;
}

template <idx element_bytes>
[[nodiscard]]
auto
find_grouped_element_simd(
   byte const* p_haystack, byte const* p_needle, idx size
) -> maybe<idx> {
   return $simd_switch($abi((avx512, avx2, sse2), {
      return find_grouped_element_large<
         native_simd<unsigned char>, element_bytes>(p_haystack, p_needle, size);
   }));
}

}  // namespace

[[nodiscard]]
auto
detail::find_memory_impl(
   byte const* _Nonnull p_haystack, byte const* _Nonnull p_needle,
   idx element_bytes, idx size
) -> maybe<idx> {
   if (size == 0u) {
      return nullopt;
   }

   switch (element_bytes.raw) {
      case 1u:
         return find_element_simd<unsigned char>(p_haystack, p_needle, size);
      case 2u:
         return find_element_simd<__UINT16_TYPE__>(p_haystack, p_needle, size);
      case 3u:
         return find_grouped_element_simd<3u>(p_haystack, p_needle, size);
      case 4u:
         return find_element_simd<__UINT32_TYPE__>(p_haystack, p_needle, size);
      case 8u:
         return find_element_simd<__UINT64_TYPE__>(p_haystack, p_needle, size);
      default:
         for (idx position = 0u; position < size; ++position) {
            if (
               compare_memory_scalar(
                  p_haystack + position * element_bytes, p_needle, element_bytes
               )
               == std::strong_ordering::equal
            ) {
               return maybe<idx>{position};
            }
         }
         return nullopt;
   }
}

}  // namespace cat
