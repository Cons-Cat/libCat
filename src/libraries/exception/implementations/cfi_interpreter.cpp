#if __has_feature(cxx_exceptions)
#include <cat/detail/dwarf_cfi.hpp>
#include <cat/detail/eh_frame_hdr.hpp>

#include <cat/array>

#include "cfi_program.hpp"

// DWARF CFI virtual machine. Parses a CIE + FDE, then walks the CFI
// opcode stream up to a target program counter to obtain the
// register-rule + canonical-frame-address snapshot in effect at that
// instruction. `unwind.cpp` then applies that snapshot to the current
// register file to synthesise the caller's register state.
//
// DW_OP expressions and signal frames are unsupported. Add them before
// unwinding through a foreign compiler or a signal-handler frame.

namespace cat::detail::cfi {

namespace {

// Pushed states from `DW_CFA_remember_state`. Eight saved states.
// Raise this if Clang emits deeper nesting.
inline constexpr idx state_stack_depth = 8u;
inline constexpr uint4 dwarf_length64_marker = 0xffffffffu;
inline constexpr uint8 extended_length_header = 12u;
inline constexpr int8 default_cfa_offset = 8;

struct execution_context {
   cfi_state initial;
   cfi_state current;
   uword current_program_counter = 0u;
   uword target_program_counter = 0u;
   cie_info const* _Nonnull p_cie;

   array<cfi_state, state_stack_depth> state_stack;
   idx state_stack_size = 0u;
};

[[nodiscard]]
auto
read_record_length(byte const* _Nonnull p_start, byte const*& out_body_start)
   -> uint8 {
   auto const frames = eh_frame_span();
   auto const begin = uintptr<byte const>(frames.data());
   auto const address = uintptr<byte const>(p_start);
   if (
      address < begin || address - begin > frames.size()
      || frames.size() - (address - begin) < 4
   ) {
      terminate();
   }
   auto const available = frames.size() - (address - begin);
   uint4 length32 = 0u;
   __builtin_memcpy_inline(&length32, p_start, sizeof(length32));
   out_body_start = p_start + 4;
   if (length32 != dwarf_length64_marker) {
      if (length32 > available - 4) {
         terminate();
      }
      return uint8(length32) + 4u;
   }
   if (available < extended_length_header) {
      terminate();
   }
   uint8 length64 = 0u;
   __builtin_memcpy_inline(&length64, p_start + 4u, sizeof(length64));
   out_body_start = p_start + extended_length_header;
   if (length64 > available - extended_length_header) {
      terminate();
   }
   return length64 + extended_length_header;
}

[[nodiscard]]
auto
parse_cie(byte const* _Nonnull p_cie_record, cie_info& out) noexcept(
   !__has_feature(cxx_exceptions)
) -> bool {
   // `out` may be reused across frames, so nothing may carry over.
   out = cie_info{};
   byte const* p_body = nullptr;
   uint8 const record_bytes = read_record_length(p_cie_record, p_body);
   byte const* const p_record_end = p_cie_record + record_bytes;

   byte_cursor cursor{
      .p_data = p_body,
      .position = 0u,
      .limit = narrow_cast<idx>(p_record_end - p_body).assert(),
   };

   // CIE id field: 4-byte 0 in `.eh_frame`.
   uint4 const cie_id = cursor.read<uint4>();
   if (cie_id != 0u) {
      return false;
   }

   uint1 const version = cursor.read<uint1>();
   if (version != 1u && version != 3u && version != 4u) {
      return false;
   }
   str_view const augmentation = cursor.read_string();

   if (version == 4u) {
      // Skip address_size + segment_size.
      cursor.skip(2u);
   }

   out.code_alignment_factor = cursor.read_uleb128();
   out.data_alignment_factor = cursor.read_sleb128();
   out.return_address_register =
      version == 1u ? uint8(cursor.read<uint1>()) : cursor.read_uleb128();
   out.cie_start_address = __builtin_bit_cast(uword, p_cie_record);

   if (augmentation.size() != 0u && augmentation[0u] == 'z') {
      out.has_augmentation = true;
      uint8 const augmentation_length = cursor.read_uleb128();
      if (augmentation_length > cursor.remaining()) {
         terminate();
      }
      byte const* const p_augmentation_start = p_body + cursor.position;
      byte const* const p_augmentation_end =
         p_augmentation_start + augmentation_length;

      // Clang on x86-64 SysV emits z, zP, zR, zL, zPR, zPL, zLR, or zPLR.
      for (idx i = 1u; i < augmentation.size(); ++i) {
         switch (augmentation[i]) {
            case 'R':
               {
                  out.fde_encoding = cursor.read<dw_eh_pe>();
                  break;
               }
            case 'L':
               {
                  out.lsda_encoding = cursor.read<dw_eh_pe>();
                  out.has_lsda = true;
                  break;
               }
            case 'P':
               {
                  // Personality address is discarded. The unwinder calls
                  // `__gxx_personality_v0` directly.
                  dw_eh_pe const encoding = cursor.read<dw_eh_pe>();
                  uword const here_address =
                     __builtin_bit_cast(uword, p_body) + cursor.position;
                  auto _ =
                     decode_eh_pointer(cursor, encoding, here_address, {});
                  out.has_personality = true;
                  break;
               }
            // `S` is a signal frame and `B` is arch-specific. Both are
            // unsupported.
            case 'S':
               [[fallthrough]];
            case 'B':
               [[fallthrough]];
            default:
               return false;
         }
      }
      auto const target =
         narrow_cast<idx>(p_augmentation_end - p_body).assert();
      if (target < cursor.position || target > cursor.limit) {
         terminate();
      }
      cursor.skip(narrow_cast<idx>(target - cursor.position).assert());
   } else if (augmentation.size() != 0u) {
      // Non-`z` augmentations are not emitted by Clang on x86-64.
      return false;
   }

   out.initial_instructions_start = p_body + cursor.position;
   out.initial_instructions_end = p_record_end;
   return true;
}

[[nodiscard]]
auto
find_cie_for_fde(
   byte const* _Nonnull p_cie_pointer_field, uint4 cie_pointer_value
) -> byte const* _Nonnull {
   return p_cie_pointer_field - cie_pointer_value.raw;
}

}  // namespace

auto
parse_fde(byte const* _Nonnull p_fde, fde_info& out_fde) noexcept(
   !__has_feature(cxx_exceptions)
) -> bool {
   // `out_fde` is reused across frames, and a frame without an LSDA must not
   // inherit the previous frame's.
   out_fde = fde_info{};
   byte const* p_body = nullptr;
   uint8 const record_bytes = read_record_length(p_fde, p_body);
   byte const* const p_record_end = p_fde + record_bytes;

   if (record_bytes < 8u) {
      terminate();
   }
   // `cie_pointer` is relative to its own field.
   byte const* const p_cie_pointer_field = p_body;
   uint4 cie_pointer = 0u;
   __builtin_memcpy_inline(&cie_pointer, p_cie_pointer_field, 4u);
   auto const frames = eh_frame_span();
   if (
      cie_pointer
      > narrow_cast<idx>(p_cie_pointer_field - frames.data()).assert()
   ) {
      terminate();
   }
   if (cie_pointer == 0u) {
      // This is actually a CIE, not an FDE.
      return false;
   }
   byte const* const p_cie_record =
      find_cie_for_fde(p_cie_pointer_field, cie_pointer);
   if (!parse_cie(p_cie_record, out_fde.cie)) {
      return false;
   }

   byte_cursor cursor{
      .p_data = p_body,
      .position = 4u,
      .limit = narrow_cast<idx>(p_record_end - p_body).assert(),
   };
   uword const initial_program_counter =
      __builtin_bit_cast(uword, p_body) + cursor.position;
   out_fde.initial_location = decode_eh_pointer(
      cursor, out_fde.cie.fde_encoding, initial_program_counter, {}
   );
   // Address range keeps the numeric format and drops the base and
   // indirect flags.
   dw_eh_pe const range_encoding =
      out_fde.cie.fde_encoding & dw_eh_pe::format_mask;
   uword const range_program_counter =
      __builtin_bit_cast(uword, p_body) + cursor.position;
   out_fde.address_range =
      decode_eh_pointer(cursor, range_encoding, range_program_counter, {});

   if (out_fde.cie.has_augmentation) {
      // Augmentation-data length precedes the LSDA pointer.
      uint8 const augmentation_length = cursor.read_uleb128();
      if (augmentation_length > cursor.remaining()) {
         terminate();
      }
      byte const* const p_augmentation_end =
         p_body + cursor.position + augmentation_length;
      uword const lsda_program_counter =
         __builtin_bit_cast(uword, p_body) + cursor.position;
      // The CIE may name an LSDA encoding for every FDE. A length of 0
      // means this FDE has no LSDA.
      idx const pointer_bytes = eh_pointer_size(out_fde.cie.lsda_encoding);
      if (
         out_fde.cie.has_lsda && augmentation_length > 0u
         && (pointer_bytes == 0u || pointer_bytes <= augmentation_length)
      ) {
         out_fde.lsda_pointer = decode_eh_pointer(
            cursor, out_fde.cie.lsda_encoding, lsda_program_counter,
            {.function = out_fde.initial_location}
         );
      }
      auto const target =
         narrow_cast<idx>(p_augmentation_end - p_body).assert();
      if (target < cursor.position || target > cursor.limit) {
         terminate();
      }
      cursor.skip(narrow_cast<idx>(target - cursor.position).assert());
   }

   out_fde.instructions_start = p_body + cursor.position;
   out_fde.instructions_end = p_record_end;
   out_fde.fde_start_address = __builtin_bit_cast(uword, p_fde);
   return true;
}

namespace {

// `stop_at_target` false runs CIE initial instructions to the end.
// True stops once the current program counter passes the target.
[[nodiscard]]
auto
run_program(
   execution_context& context, byte const* _Nonnull p_start,
   byte const* _Nonnull p_end, bool stop_at_target
) noexcept(!__has_feature(cxx_exceptions)) -> bool {
   byte_cursor cursor{
      .p_data = p_start,
      .position = 0u,
      .limit = narrow_cast<idx>(p_end - p_start).assert(),
   };

   while (!cursor.done()) {
      if (
         stop_at_target
         && context.current_program_counter > context.target_program_counter
      ) {
         break;
      }
      uint1 const opcode = cursor.read<uint1>();
      auto const primary = static_cast<cfa_primary>(opcode & 0xc0u);
      uint1 const low_bits = uint1(opcode & 0x3fu);

      if (primary == cfa_primary::cfa_advance_loc) {
         context.current_program_counter +=
            uint8(low_bits) * context.p_cie->code_alignment_factor;
         continue;
      }
      if (primary == cfa_primary::cfa_offset) {
         uint8 const factored = cursor.read_uleb128();
         int8 const offset_bytes =
            int8(factored) * context.p_cie->data_alignment_factor;
         if (low_bits < register_count) {
            context.current.registers[low_bits.raw] = {
               .kind = rule_kind::offset,
               .offset = offset_bytes,
            };
         }
         continue;
      }
      if (primary == cfa_primary::cfa_restore) {
         if (low_bits < register_count) {
            context.current.registers[low_bits.raw] =
               context.initial.registers[low_bits.raw];
         }
         continue;
      }

      auto const extended = static_cast<cfa_extended>(opcode);
      switch (extended) {
         case cfa_extended::cfa_nop:
            break;

         case cfa_extended::cfa_set_loc:
            {
               context.current_program_counter = decode_eh_pointer(
                  cursor, context.p_cie->fde_encoding,
                  __builtin_bit_cast(uword, p_start) + cursor.position, {}
               );
               break;
            }
         case cfa_extended::cfa_advance_loc1:
            {
               context.current_program_counter +=
                  uint8(cursor.read<uint1>())
                  * context.p_cie->code_alignment_factor;
               break;
            }
         case cfa_extended::cfa_advance_loc2:
            {
               context.current_program_counter +=
                  uint8(cursor.read<uint2>())
                  * context.p_cie->code_alignment_factor;
               break;
            }
         case cfa_extended::cfa_advance_loc4:
            {
               context.current_program_counter +=
                  uint8(cursor.read<uint4>())
                  * context.p_cie->code_alignment_factor;
               break;
            }

         case cfa_extended::cfa_offset_extended:
            {
               uint8 const register_number = cursor.read_uleb128();
               uint8 const factored = cursor.read_uleb128();
               int8 const offset_bytes =
                  int8(factored) * context.p_cie->data_alignment_factor;
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::offset,
                     .offset = offset_bytes,
                  };
               }
               break;
            }
         case cfa_extended::cfa_offset_extended_sf:
            {
               uint8 const register_number = cursor.read_uleb128();
               int8 const factored = cursor.read_sleb128();
               int8 const offset_bytes =
                  factored * context.p_cie->data_alignment_factor;
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::offset,
                     .offset = offset_bytes,
                  };
               }
               break;
            }
         case cfa_extended::cfa_val_offset:
            {
               uint8 const register_number = cursor.read_uleb128();
               uint8 const factored = cursor.read_uleb128();
               int8 const offset_bytes =
                  int8(factored) * context.p_cie->data_alignment_factor;
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::val_offset,
                     .offset = offset_bytes,
                  };
               }
               break;
            }
         case cfa_extended::cfa_val_offset_sf:
            {
               uint8 const register_number = cursor.read_uleb128();
               int8 const factored = cursor.read_sleb128();
               int8 const offset_bytes =
                  factored * context.p_cie->data_alignment_factor;
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::val_offset,
                     .offset = offset_bytes,
                  };
               }
               break;
            }
         case cfa_extended::cfa_restore_extended:
            {
               uint8 const register_number = cursor.read_uleb128();
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] =
                     context.initial.registers[register_number.raw];
               }
               break;
            }
         case cfa_extended::cfa_undefined:
            {
               uint8 const register_number = cursor.read_uleb128();
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::undefined,
                  };
               }
               break;
            }
         case cfa_extended::cfa_same_value:
            {
               uint8 const register_number = cursor.read_uleb128();
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::same_value,
                  };
               }
               break;
            }
         case cfa_extended::cfa_register:
            {
               uint8 const register_number = cursor.read_uleb128();
               uint8 const source_register = cursor.read_uleb128();
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::from_register,
                     .register_number =
                        narrow_cast<idx>(source_register).assert(),
                  };
               }
               break;
            }
         case cfa_extended::cfa_remember_state:
            {
               if (context.state_stack_size >= state_stack_depth) {
                  return false;
               }
               context.state_stack[context.state_stack_size] = context.current;
               ++context.state_stack_size;
               break;
            }
         case cfa_extended::cfa_restore_state:
            {
               if (context.state_stack_size == 0u) {
                  return false;
               }
               --context.state_stack_size.raw;
               context.current = context.state_stack[context.state_stack_size];
               break;
            }
         case cfa_extended::cfa_def_cfa:
            {
               uint8 const register_number = cursor.read_uleb128();
               uint8 const offset = cursor.read_uleb128();
               context.current.canonical_frame_address = {
                  .kind = canonical_frame_address_kind::from_register,
                  .register_number = narrow_cast<idx>(register_number).assert(),
                  .offset = int8(offset),
                  .p_expression = nullptr,
               };
               break;
            }
         case cfa_extended::cfa_def_cfa_sf:
            {
               uint8 const register_number = cursor.read_uleb128();
               int8 const factored = cursor.read_sleb128();
               context.current.canonical_frame_address = {
                  .kind = canonical_frame_address_kind::from_register,
                  .register_number = narrow_cast<idx>(register_number).assert(),
                  .offset = factored * context.p_cie->data_alignment_factor,
                  .p_expression = nullptr,
               };
               break;
            }
         case cfa_extended::cfa_def_cfa_register:
            {
               uint8 const register_number = cursor.read_uleb128();
               context.current.canonical_frame_address.kind =
                  canonical_frame_address_kind::from_register;
               context.current.canonical_frame_address.register_number =
                  narrow_cast<idx>(register_number).assert();
               context.current.canonical_frame_address.p_expression = nullptr;
               break;
            }
         case cfa_extended::cfa_def_cfa_offset:
            {
               uint8 const offset = cursor.read_uleb128();
               context.current.canonical_frame_address.offset = int8(offset);
               break;
            }
         case cfa_extended::cfa_def_cfa_offset_sf:
            {
               int8 const factored = cursor.read_sleb128();
               context.current.canonical_frame_address.offset =
                  factored * context.p_cie->data_alignment_factor;
               break;
            }
         case cfa_extended::cfa_def_cfa_expression:
            {
               uint8 const expression_length = cursor.read_uleb128();
               byte const* const p_expression = p_start + cursor.position;
               cursor.skip(narrow_cast<idx>(expression_length).assert());
               context.current.canonical_frame_address = {
                  .kind = canonical_frame_address_kind::from_expression,
                  .register_number = 0,
                  .offset = 0,
                  .p_expression = p_expression,
               };
               break;
            }
         case cfa_extended::cfa_expression:
            [[fallthrough]];
         case cfa_extended::cfa_val_expression:
            {
               uint8 const register_number = cursor.read_uleb128();
               uint8 const expression_length = cursor.read_uleb128();
               byte const* const p_expression = p_start + cursor.position;
               cursor.skip(narrow_cast<idx>(expression_length).assert());
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = extended == cfa_extended::cfa_expression
                                ? rule_kind::expression
                                : rule_kind::val_expression,
                     .offset = 0,
                     .register_number = 0,
                     .p_expression = p_expression,
                  };
               }
               break;
            }
         case cfa_extended::cfa_gnu_args_size:
            {
               // Only affects `__builtin_return_address` through partial
               // frames, which this unwinder does not consume.
               auto _ = cursor.read_uleb128();
               break;
            }
         case cfa_extended::cfa_gnu_negative_offset_extended:
            {
               uint8 const register_number = cursor.read_uleb128();
               uint8 const factored = cursor.read_uleb128();
               int8 const offset_bytes =
                  -int8(factored) * context.p_cie->data_alignment_factor;
               if (register_number < register_count) {
                  context.current.registers[register_number.raw] = {
                     .kind = rule_kind::offset,
                     .offset = offset_bytes,
                  };
               }
               break;
            }
         default:
            return false;
      }
   }
   return true;
}

}  // namespace

auto
compute_state_at(
   fde_info const& fde, uword target_program_counter, cfi_state& out_state
) -> bool {
   if (
      target_program_counter < fde.initial_location
      || target_program_counter - fde.initial_location >= fde.address_range
   ) {
      return false;
   }
   // Callers only apply the callee-saved subset, so this default is
   // harmless.
   cfi_state initial{};
   initial.canonical_frame_address = canonical_frame_address_rule{
      .kind = canonical_frame_address_kind::from_register,
      .register_number = register_rsp,
      .offset = default_cfa_offset,
   };
   for (auto& rule : initial.registers) {
      rule = {.kind = rule_kind::same_value};
   }

   execution_context context{
      .initial = initial,
      .current = initial,
      .current_program_counter = fde.initial_location,
      .target_program_counter = target_program_counter,
      .p_cie = &fde.cie,
      .state_stack = {},
      .state_stack_size = 0u,
   };

   if (
      fde.cie.initial_instructions_start != nullptr
      && !run_program(
         context, fde.cie.initial_instructions_start,
         fde.cie.initial_instructions_end,
         /*stop_at_target=*/false
      )
   ) {
      return false;
   }
   context.initial = context.current;
   if (!run_program(
          context, fde.instructions_start, fde.instructions_end,
          /*stop_at_target=*/true
       )) {
      return false;
   }
   out_state = context.current;
   return true;
}

auto
step_frame(
   cfi_state const& state, cie_info const& cie, register_context& registers
) noexcept(!__has_feature(cxx_exceptions)) -> maybe_non_zero<uword> {
   uword canonical_frame_address_value = 0u;
   switch (state.canonical_frame_address.kind) {
      case canonical_frame_address_kind::from_register:
         {
            if (
               state.canonical_frame_address.register_number >= register_count
            ) {
               return nullopt;
            }
            canonical_frame_address_value =
               registers
                  .registers[state.canonical_frame_address.register_number]
               + int8(state.canonical_frame_address.offset);
            break;
         }
      case canonical_frame_address_kind::from_expression:
         // DW_OP expressions are unsupported, so the step fails.
         return nullopt;
   }

   register_context caller_registers{};
   for (idx index = 0u; index < register_count; ++index) {
      caller_registers.registers[index] = registers.registers[index];
   }

   for (idx index = 0u; index < register_count; ++index) {
      register_rule const& rule = state.registers[index];
      switch (rule.kind) {
         case rule_kind::undefined:
            caller_registers.registers[index] = 0u;
            break;
         case rule_kind::same_value:
            [[fallthrough]];
         case rule_kind::architectural:
            break;
         case rule_kind::offset:
            {
               auto const* const p_slot = __builtin_bit_cast(
                  uword const*,
                  canonical_frame_address_value + int8(rule.offset)
               );
               caller_registers.registers[index] = *p_slot;
               break;
            }
         case rule_kind::val_offset:
            caller_registers.registers[index] =
               canonical_frame_address_value + int8(rule.offset);
            break;
         case rule_kind::from_register:
            {
               if (rule.register_number < register_count) {
                  caller_registers.registers[index] =
                     registers.registers[rule.register_number];
               }
               break;
            }
         case rule_kind::expression:
            [[fallthrough]];
         case rule_kind::val_expression:
            // DW_OP rules are unsupported. Add evaluation before
            // compilers that emit expression-based unwind rules.
            return nullopt;
      }
   }

   caller_registers.registers[register_rsp] = canonical_frame_address_value;
   idx const return_address_column =
      narrow_cast<idx>(cie.return_address_register).assert();
   uword new_instruction_pointer = 0u;
   if (return_address_column < register_count) {
      new_instruction_pointer =
         caller_registers.registers[return_address_column];
   }
   caller_registers.registers[register_rip] = new_instruction_pointer;

   for (idx index = 0u; index < register_count; ++index) {
      registers.registers[index] = caller_registers.registers[index];
   }
   return new_instruction_pointer;
}

}  // namespace cat::detail::cfi

#endif
