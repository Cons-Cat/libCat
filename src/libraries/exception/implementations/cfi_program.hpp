// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Private cross-TU interface. `cfi_interpreter.cpp` produces per-frame
// CFI state and `unwind.cpp` consumes it to advance the register file.
// Not exposed to library consumers.

#include <cat/detail/dwarf_cfi.hpp>
#include <cat/detail/register_context.hpp>

#include <cat/arithmetic>
#include <cat/maybe>

namespace cat::detail::cfi {

enum class rule_kind : uint1::raw_type {
   undefined = 0,
   same_value,
   offset,
   val_offset,
   from_register,
   expression,
   val_expression,
   architectural,
};

struct register_rule {
   rule_kind kind = rule_kind::same_value;
   int8 offset = 0;
   idx register_number = 0u;
   byte const* _Nullable p_expression = nullptr;
};

enum class canonical_frame_address_kind : uint1::raw_type {
   from_register,
   from_expression,
};

struct canonical_frame_address_rule {
   canonical_frame_address_kind kind =
      canonical_frame_address_kind::from_register;
   idx register_number = 7u;  // rsp
   int8 offset = 0;
   byte const* _Nullable p_expression = nullptr;
};

struct cfi_state {
   canonical_frame_address_rule canonical_frame_address;
   register_rule registers[register_count];
};

// `has_personality` records that the CIE augmentation had `P`. The
// personality address is discarded. The unwinder calls
// `__gxx_personality_v0` directly.
struct cie_info {
   uint8 code_alignment_factor = 1u;
   int8 data_alignment_factor = -8;
   uint8 return_address_register = register_rip;
   dw_eh_pe fde_encoding = dw_eh_pe::absptr;
   dw_eh_pe lsda_encoding = dw_eh_pe::omit;
   bool has_lsda = false;
   bool has_augmentation = false;
   bool has_personality = false;
   byte const* _Nullable initial_instructions_start = nullptr;
   byte const* _Nullable initial_instructions_end = nullptr;
   uword cie_start_address = 0u;
};

struct fde_info {
   cie_info cie;
   uword initial_location = 0u;
   uword address_range = 0u;
   uword lsda_pointer = 0u;
   byte const* _Nullable instructions_start = nullptr;
   byte const* _Nullable instructions_end = nullptr;
   uword fde_start_address = 0u;
};

// `p_fde` starts at the length field. Its CIE pointer is a backward
// offset. Returns false on malformed input.
[[nodiscard]]
auto
parse_fde(byte const* _Nonnull p_fde, fde_info& out_fde) noexcept(!__has_feature(cxx_exceptions)) -> bool;

// Returns false if interpretation runs off the end or hits an
// unsupported opcode. The state is the DWARF snapshot at exactly
// `target_program_counter`.
[[nodiscard]]
auto
compute_state_at(
   fde_info const& fde, uword target_program_counter, cfi_state& out_state
) -> bool;

// Advances `registers` by one frame. The value is the caller's program
// counter. An empty result means end-of-stack.
[[nodiscard]]
auto
step_frame(
   cfi_state const& state, cie_info const& cie, register_context& registers
) noexcept(!__has_feature(cxx_exceptions)) -> maybe_non_zero<uword>;

}  // namespace cat::detail::cfi
