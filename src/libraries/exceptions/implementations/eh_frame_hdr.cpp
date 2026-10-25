#if __has_feature(cxx_exceptions)
#include <cat/detail/eh_frame_hdr.hpp>

#include "implementations/narrow_cast.tpp"

// These are implemented by the `libcat.ld` linker script.
extern "C" {
// NOLINTBEGIN(bugprone-reserved-identifier)
extern char __cat_eh_frame_hdr_start[];
extern char __cat_eh_frame_table_start[];
extern char __cat_eh_frame_table_count[];
extern char __cat_eh_frame_start[];
extern char __cat_eh_frame_end[];
extern char __cat_eh_except_table_start[];
extern char __cat_eh_except_table_end[];
// NOLINTEND(bugprone-reserved-identifier)
}

namespace cat::detail {
auto
eh_frame_span() noexcept(!__has_feature(cxx_exceptions)) -> span<byte const> {
   return {
      __builtin_bit_cast(byte const*, &__cat_eh_frame_start),
      narrow_cast<idx>(__cat_eh_frame_end - __cat_eh_frame_start).assert()
   };
}

auto
eh_except_table_span() noexcept(!__has_feature(cxx_exceptions))
   -> span<byte const> {
   return {
      __builtin_bit_cast(byte const*, &__cat_eh_except_table_start),
      narrow_cast<idx>(__cat_eh_except_table_end - __cat_eh_except_table_start)
         .assert()
   };
}

auto
find_fde_for_ip(uword instruction_pointer) noexcept(
   !__has_feature(cxx_exceptions)
) -> fde_lookup_result {
   // lld writes sorted pairs of signed 4-byte offsets from `.eh_frame_hdr`.
   // The linker script supplies the table and its count. No ELF or header scan.
   struct table_entry {
      int4 initial_location;
      int4 fde_offset;
   };

   auto const* p_table =
      __builtin_bit_cast(table_entry const*, &__cat_eh_frame_table_start);
   auto const base =
      uintptr<byte>(__builtin_bit_cast(byte*, &__cat_eh_frame_hdr_start));
   auto const count =
      uintptr<byte>(__builtin_bit_cast(byte*, &__cat_eh_frame_table_count));
   uintptr<byte> lower = 0;
   uintptr<byte> upper = count;

   while (lower < upper) {
      auto const middle = lower + ((upper - lower) / 2);
      idx const table_index = narrow_cast<idx>(middle).assert();
      auto const initial = base + iword(p_table[table_index].initial_location);
      if (initial <= instruction_pointer) {
         lower = middle + 1;
      } else {
         upper = middle;
      }
   }

   if (lower == 0) {
      return {};
   }

   auto const& entry = p_table[narrow_cast<idx>(lower - 1).assert()];
   auto const* p_fde = (base + iword(entry.fde_offset)).get();

   return {
      .p_fde = p_fde,
      .instruction_pointer = base + iword(entry.initial_location),
   };
}

}  // namespace cat::detail

#endif
