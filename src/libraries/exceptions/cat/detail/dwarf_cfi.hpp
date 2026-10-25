// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// DWARF v5 CFI opcodes, encoding constants, and LEB128 decoders shared
// by the CIE and FDE parser and the CFI virtual machine. Encodings are
// the subset Clang emits with `-fasynchronous-unwind-tables` on
// x86-64. The values are the standard DWARF numbers.

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/runtime>
#include <cat/span>
#include <cat/string>
#include <cat/utility>

#include "implementations/narrow_cast.tpp"

namespace cat::detail {

// Call Frame Address has two types of opcodes, "primary" and "extended".

// Primary opcodes carry a 2-bit opcode in the leading two bits.
enum class cfa_primary : uint1::raw_type {
   cfa_advance_loc = 64,
   cfa_offset = 128,
   cfa_restore = 192,
};

// Extended opcodes have those bits clear and use the trailing six
// bits as the opcode.
enum class cfa_extended : uint1::raw_type {
   cfa_nop = 0,
   cfa_set_loc = 1,
   cfa_advance_loc1 = 2,
   cfa_advance_loc2 = 3,
   cfa_advance_loc4 = 4,
   cfa_offset_extended = 5,
   cfa_restore_extended = 6,
   cfa_undefined = 7,
   cfa_same_value = 8,
   cfa_register = 9,
   cfa_remember_state = 10,
   cfa_restore_state = 11,
   cfa_def_cfa = 12,
   cfa_def_cfa_register = 13,
   cfa_def_cfa_offset = 14,
   cfa_def_cfa_expression = 15,
   cfa_expression = 16,
   cfa_offset_extended_sf = 17,
   cfa_def_cfa_sf = 18,
   cfa_def_cfa_offset_sf = 19,
   cfa_val_offset = 20,
   cfa_val_offset_sf = 21,
   cfa_val_expression = 22,
   cfa_gnu_args_size = 46,
   cfa_gnu_negative_offset_extended = 47,
};

// DW_EH_PE_* pointer-encoding constants. The trailing nibble picks the
// numeric format. The leading nibble picks the base and any indirection.
enum class [[clang::flag_enum]] dw_eh_pe : uint1::raw_type {
   absptr = 0,
   uleb128 = 1,
   udata2 = 2,
   udata4 = 3,
   udata8 = 4,
   // Sign bit of the format nibble.
   signed_format = 8,
   sleb128 = 9,
   sdata2 = 10,
   sdata4 = 11,
   sdata8 = 12,

   pcrel = 16,
   textrel = 32,
   datarel = 48,
   funcrel = 64,
   aligned = 80,

   indirect = 128,

   format_mask = 15,
   base_mask = 112,
   omit = 255,
};

}  // namespace cat::detail

template <>
struct cat::enum_flag_trait<cat::detail::dw_eh_pe> : cat::true_trait {};

namespace cat::detail {

// Shared by CFA, FDE, and LSDA parsing.
struct byte_cursor {
   byte const* _Nonnull p_data;
   idx position = 0u;
   idx limit = 0u;

   [[nodiscard]]
   constexpr auto
   remaining() const noexcept(!__has_feature(cxx_exceptions)) -> idx {
      return narrow_cast<idx>(limit - position).assert();
   }

   [[nodiscard]]
   constexpr auto
   done() const -> bool {
      return position >= limit;
   }

   [[nodiscard]]
   constexpr auto
   here() const -> byte const* _Nonnull {
      return p_data + position;
   }

   template <typename T>
   [[nodiscard]]
   auto
   read() -> T {
      if constexpr (is_enum<T>) {
         return static_cast<T>(read<underlying_type<T>>());
      } else if constexpr (is_arithmetic<T> && !is_raw_arithmetic<T>) {
         return read<raw_arithmetic_type<T>>();
      } else {
         if (position > limit || sizeof(T) > remaining()) {
            terminate();
         }
         T value;
         __builtin_memcpy_inline(&value, p_data + position, sizeof(T));
         position += sizeof(T);
         return value;
      }
   }

   void
   skip(idx count) {
      if (position > limit || count > remaining()) {
         terminate();
      }
      position += count;
   }

   [[nodiscard]]
   auto
   read_uleb128() -> uint8 {
      uint8 result = 0u;
      for (idx shift = 0u; shift < 64u; shift += 7u) {
         uint8 const chunk = read<uint1>();
         if (shift == 63u && chunk > 1u) {
            // Only the leading bit of an 8-byte value is left at this shift, so
            // a larger chunk would overflow the result.
            terminate();
         }
         result |= (chunk & 0x7full) << shift;
         if ((chunk & 0x80ull) == 0u) {
            return result;
         }
      }
      terminate();
   }

   [[nodiscard]]
   auto
   read_sleb128() -> int8 {
      uint8 result = 0u;
      for (idx shift = 0u; shift < 64u; shift += 7u) {
         uint8 const chunk = read<uint1>();
         if (shift == 63u && chunk != 0u && chunk != 0x7fu) {
            terminate();
         }
         result |= (chunk & 0x7full) << shift;
         if ((chunk & 0x80ull) == 0u) {
            if (shift < 63u && (chunk & 0x40ull) != 0u) {
               result |= uint8(~0ull) << (shift + 7u);
            }
            return __builtin_bit_cast(int8, result);
         }
      }
      terminate();
   }

   [[nodiscard]]
   auto
   read_string() noexcept(!__has_feature(cxx_exceptions)) -> zstr_view {
      idx const start = position;
      while (position < limit && p_data[position].value != 0u) {
         ++position;
      }
      if (position == limit) {
         terminate();
      }
      ++position;
      return {
         __builtin_bit_cast(char const*, p_data + start),
         narrow_cast<idx>(position - start).assert()
      };
   }
};

struct eh_pointer_bases {
   uword text = 0u;
   uword data = 0u;
   uword function = 0u;
};

// `program_counter` is the address the pointer is read from. It is
// required for `pcrel`. Indirect encodings are resolved here because
// every caller wants those loads done automatically.
[[nodiscard]]
auto
decode_eh_pointer(
   byte_cursor& cursor, dw_eh_pe encoding, uword program_counter,
   eh_pointer_bases eh_pointer_bases
) noexcept(!__has_feature(cxx_exceptions)) -> uword;

// Byte size with no effect on the cursor. FDE parsers use this to skip
// an initial-location field before deciding whether to decode it.
[[nodiscard]]
constexpr auto
eh_pointer_size(dw_eh_pe encoding) -> idx {
   if (encoding == dw_eh_pe::omit) {
      return 0u;
   }

   switch (encoding & dw_eh_pe::format_mask) {
      case dw_eh_pe::absptr:
         return 8u;
      case dw_eh_pe::udata2:
         [[fallthrough]];
      case dw_eh_pe::sdata2:
         return 2u;
      case dw_eh_pe::udata4:
         [[fallthrough]];
      case dw_eh_pe::sdata4:
         return 4u;
      case dw_eh_pe::udata8:
         [[fallthrough]];
      case dw_eh_pe::sdata8:
         return 8u;
      case dw_eh_pe::uleb128:
         [[fallthrough]];
      case dw_eh_pe::sleb128:
         // LEB128 is variable-length. Decode it to advance the cursor.
         return 0u;
      default:
         __builtin_unreachable();
   }
}

}  // namespace cat::detail
