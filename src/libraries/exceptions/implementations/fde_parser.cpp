#if __has_feature(cxx_exceptions)
#include <cat/detail/dwarf_cfi.hpp>

// Shared here so the CFI interpreter and LSDA do not include each
// other.

namespace cat::detail {

auto
decode_eh_pointer(
   byte_cursor& cursor, dw_eh_pe encoding, uword program_counter,
   eh_pointer_bases eh_pointer_bases
) noexcept(!__has_feature(cxx_exceptions)) -> uword {
   if (encoding == dw_eh_pe::omit) {
      return 0u;
   }

   // Signed encodings are sign-extended, and offsets wrap like the address
   // arithmetic they encode.
   uword raw = 0u;
   uword const here_address = program_counter;
   switch (encoding & dw_eh_pe::format_mask) {
      case dw_eh_pe::absptr:
         raw = cursor.read<uint8>();
         break;
      case dw_eh_pe::uleb128:
         raw = cursor.read_uleb128();
         break;
      case dw_eh_pe::udata2:
         raw = cursor.read<uint2>();
         break;
      case dw_eh_pe::udata4:
         raw = cursor.read<uint4>();
         break;
      case dw_eh_pe::udata8:
         raw = cursor.read<uint8>();
         break;
      case dw_eh_pe::sleb128:
         raw = __builtin_bit_cast(uword, int8(cursor.read_sleb128()));
         break;
      case dw_eh_pe::sdata2:
         raw = __builtin_bit_cast(uword, int8(cursor.read<int2>()));
         break;
      case dw_eh_pe::sdata4:
         raw = __builtin_bit_cast(uword, int8(cursor.read<int4>()));
         break;
      case dw_eh_pe::sdata8:
         raw = cursor.read<uint8>();
         break;
      default:
         __builtin_unreachable();
   }

   switch (encoding & dw_eh_pe::base_mask) {
      case dw_eh_pe::absptr:
         break;
      case dw_eh_pe::pcrel:
         if (raw != 0u) {
            raw = raw + here_address;
         }
         break;
      case dw_eh_pe::textrel:
         if (raw != 0u) {
            raw = raw + eh_pointer_bases.text;
         }
         break;
      case dw_eh_pe::datarel:
         if (raw != 0u) {
            raw = raw + eh_pointer_bases.data;
         }
         break;
      case dw_eh_pe::funcrel:
         if (raw != 0u) {
            raw = raw + eh_pointer_bases.function;
         }
         break;
      default:
         break;
   }

   if ((encoding & dw_eh_pe::indirect) != dw_eh_pe::absptr && raw != 0u) {
      // Indirect through the resolved address, typically a global
      // offset table slot.
      auto const* const p_slot = __builtin_bit_cast(uword const*, raw);
      raw = *p_slot;
   }
   return raw;
}

}  // namespace cat::detail

#endif
