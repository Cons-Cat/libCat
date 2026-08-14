#include <cat/format>
#include <cat/memory>
#include <cat/simd>
#include <cat/simd_switch>

namespace cat::detail {
namespace {

template <typename Simd>
struct debug_escape_scanner {
   char quote;
   Simd quote_lanes;
   Simd slash_lanes;
   Simd newline_lanes;
   Simd carriage_lanes;
   Simd tab_lanes;

   explicit debug_escape_scanner(char quote_character)
       : quote(quote_character) {
      quote_lanes.fill(quote);
      slash_lanes.fill('\\');
      newline_lanes.fill('\n');
      carriage_lanes.fill('\r');
      tab_lanes.fill('\t');
   }

   [[nodiscard]]
   auto
   first_escape_in(Simd const& chunk, idx lane_limit) const -> maybe<idx> {
      auto const hits = chunk.equal_lanes(quote_lanes)
                        | chunk.equal_lanes(slash_lanes)
                        | chunk.equal_lanes(newline_lanes)
                        | chunk.equal_lanes(carriage_lanes)
                        | chunk.equal_lanes(tab_lanes);
      if (!hits.any_of()) {
         return nullopt;
      }
      idx const lane = hits.find_if_true();
      if (lane >= lane_limit) {
         return nullopt;
      }
      return lane;
   }

   [[nodiscard]]
   auto
   find(char const* _Nonnull p_current, char const* _Nonnull p_end) const
      -> char const* _Nonnull {
      constexpr idx vector_size = sizeof(Simd);
      iword remaining = p_end - p_current;
      while (remaining >= vector_size) {
         Simd chunk;
         chunk.load_unaligned(p_current);
         if (
            maybe<idx> const lane = first_escape_in(chunk, vector_size);
            lane.has_value()
         ) {
            return p_current + lane.value();
         }
         p_current += vector_size;
         remaining -= vector_size;
      }

      if (remaining == 0u) {
         return p_end;
      }

      // Pad the tail so a 3-byte `{:?}` still takes one vector compare.
      // Extra lanes are 0, which is not an escape.
      alignas(Simd) char pad[sizeof(Simd)]{};
      copy_memory(p_current, pad, idx(remaining));
      Simd tail;
      tail.load_unaligned(pad);
      if (
         maybe<idx> const lane = first_escape_in(tail, idx(remaining));
         lane.has_value()
      ) {
         return p_current + lane.value();
      }
      return p_end;
   }
};

auto
append_escape_char(format_context& context, char character)
   -> scaredy_format<void> {
   if (character == '\n') {
      return context.append("\\n");
   }
   if (character == '\r') {
      return context.append("\\r");
   }
   if (character == '\t') {
      return context.append("\\t");
   }
   scaredy_format<void> result = context.append('\\');
   if (result.is_empty()) {
      return result;
   }
   return context.append(character);
}

template <typename Find>
auto
append_escaped_loop(
   format_context& context, str_view string, char quote, Find find
) -> scaredy_format<void> {
   scaredy_format<void> result = context.append(quote);
   if (result.is_empty()) {
      return result;
   }

   char const* p_current = string.data();
   char const* const p_end = p_current + string.size().raw;
   while (p_current != p_end) {
      char const* _Nonnull const p_escape = find(p_current, p_end, quote);
      if (p_escape != p_current) {
         result =
            context.append(str_view(p_current, idx(p_escape - p_current)));
         if (result.is_empty()) {
            return result;
         }
      }
      if (p_escape == p_end) {
         break;
      }
      result = append_escape_char(context, *p_escape);
      if (result.is_empty()) {
         return result;
      }
      p_current = p_escape + 1;
   }
   return context.append(quote);
}

}  // namespace

auto
append_escaped(format_context& context, str_view string, char quote)
   -> scaredy_format<void> {
   if (string.size() == 0u) {
      scaredy_format<void> result = context.append(quote);
      if (result.is_empty()) {
         return result;
      }
      return context.append(quote);
   }
   return $simd_switch($abi((avx2, sse2), {
      debug_escape_scanner<char1x_> const scanner(quote);
      return append_escaped_loop(
         context, string, quote,
         [&](
            char const* _Nonnull p_current, char const* _Nonnull p_end,
            char /*quote*/
         ) {
            return scanner.find(p_current, p_end);
         }
      );
   }));
}

}  // namespace cat::detail
