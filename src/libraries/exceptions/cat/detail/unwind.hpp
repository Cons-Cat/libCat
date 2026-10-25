// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>

namespace cat::detail {

enum class [[clang::enum_extensibility(
   closed
)]] unwind_reason_code : uint1::raw_type {
   no_reason = 0,
   foreign_exception_caught = 1,
   fatal_phase2_error = 2,
   fatal_phase1_error = 3,
   normal_stop = 4,
   end_of_stack = 5,
   handler_found = 6,
   install_context = 7,
   continue_unwind = 8,
};

// Personality receives a bit-or of these to describe the current phase
// and whether this frame is the handler frame chosen in phase 1.
enum class [[clang::flag_enum]] unwind_action : uint1::raw_type {
   none = 0,
   search_phase = 1,
   cleanup_phase = 2,
   handler_frame = 4,
   force_unwind = 8,
   end_of_stack_action = 16,
};

struct unwind_exception;
struct unwind_context;

using unwind_exception_cleanup_fn =
   void (*_Nonnull)(unwind_reason_code, unwind_exception* _Nonnull);

// Itanium layout. `private_1` and `private_2` are reserved for the
// unwinder. `private_2` identifies the handler frame. The struct is
// 16-byte aligned per the SysV ABI.
struct alignas(16) unwind_exception {
   uint8 exception_class;
   unwind_exception_cleanup_fn exception_cleanup;
   uword private_1;
   uword private_2;
};

// libCat's exception class. The 8-byte word encodes vendor + language
// as `"CATCC++\0"`. Foreign runtimes are unsupported.
inline constexpr uint8 exception_class_primary = 0x43415443'432b2b00u;

[[nodiscard]]
inline auto
is_native_exception(uint8 exception_class) -> bool {
   return exception_class == exception_class_primary;
}

// ABI prototypes. Definitions live under `implementations/`.
// `extern "C"` keeps the names unmangled.
extern "C" {

auto
_Unwind_RaiseException(unwind_exception* _Nonnull p_exception_object)
   -> unwind_reason_code;

void
_Unwind_Resume(unwind_exception* _Nonnull p_exception_object);

void
_Unwind_DeleteException(unwind_exception* _Nonnull p_exception_object);

auto
_Unwind_GetIP(unwind_context* _Nonnull p_context) -> uword;

auto
_Unwind_GetIPInfo(unwind_context* _Nonnull p_context, int4* _Nonnull p_before)
   -> uword;

void
_Unwind_SetIP(
   unwind_context* _Nonnull p_context, uword new_instruction_pointer
);

auto
_Unwind_GetGR(unwind_context* _Nonnull p_context, int4 register_number)
   -> uword;

void
_Unwind_SetGR(
   unwind_context* _Nonnull p_context, int4 register_number, uword value
);

auto
_Unwind_GetLanguageSpecificData(unwind_context* _Nonnull p_context) -> uword;

auto
_Unwind_GetRegionStart(unwind_context* _Nonnull p_context) -> uword;

auto
_Unwind_GetCFA(unwind_context* _Nonnull p_context) -> uword;

auto
_Unwind_GetDataRelBase(unwind_context* _Nonnull p_context) -> uword;

auto
_Unwind_GetTextRelBase(unwind_context* _Nonnull p_context) -> uword;

}  // extern "C"

}  // namespace cat::detail

template <>
struct cat::enum_flag_trait<cat::detail::unwind_action> : cat::true_trait {};
