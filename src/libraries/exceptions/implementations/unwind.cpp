#if __has_feature(cxx_exceptions)
#include <cat/detail/cxa_abi.hpp>
#include <cat/detail/eh_frame_hdr.hpp>
#include <cat/detail/register_context.hpp>
#include <cat/detail/unwind.hpp>

#include <cat/runtime>

#include "cfi_program.hpp"

// The two-phase walk follows the Itanium spec. The unwinder calls
// `__gxx_personality_v0` directly, with no CIE personality pointer
// indirection.

namespace cat::detail {

extern "C" auto
// NOLINTNEXTLINE(bugprone-reserved-identifier)
__gxx_personality_v0(
   int4 version, unwind_action actions, uint8 exception_class,
   unwind_exception* _Nonnull p_exception_object,
   unwind_context* _Nonnull p_context
) -> unwind_reason_code;

// `unwind_context` is opaque to `_Unwind_Get*` and `_Unwind_Set*`
// callers. The layout lives here so those accessors can compile
// against the incomplete declaration in the header.
struct cat_unwind_context_body {
   register_context* _Nonnull p_registers;
   cfi::fde_info const* _Nullable p_fde;
   uword canonical_frame_address = 0u;
   uword instruction_pointer = 0u;
};

struct unwind_context : cat_unwind_context_body {};

namespace {

[[nodiscard]]
auto
run_phase_one(
   register_context snapshot, unwind_exception* _Nonnull p_exception,
   uword& out_stop_frame_address
) -> unwind_reason_code {
   register_context frame_registers = snapshot;
   cfi::fde_info fde{};

   while (true) {
      uword const instruction_pointer_before_step =
         frame_registers.registers[register_rip];

      if (instruction_pointer_before_step == 0u) {
         return unwind_reason_code::end_of_stack;
      }

      fde_lookup_result const lookup =
         find_fde_for_ip(instruction_pointer_before_step - 1u);

      if (lookup.p_fde == nullptr) {
         return unwind_reason_code::end_of_stack;
      }

      if (!cfi::parse_fde(lookup.p_fde, fde)) {
         return unwind_reason_code::fatal_phase1_error;
      }

      cfi::cfi_state state{};
      if (!cfi::compute_state_at(
             fde, instruction_pointer_before_step - 1u, state
          )) {
         return unwind_reason_code::fatal_phase1_error;
      }

      if (fde.cie.has_personality) {
         unwind_context personality_context{};
         personality_context.p_registers = &frame_registers;
         personality_context.p_fde = &fde;
         personality_context.instruction_pointer =
            instruction_pointer_before_step;
         personality_context.canonical_frame_address = 0u;
         unwind_reason_code const result = __gxx_personality_v0(
            /*version=*/1, unwind_action::search_phase,
            p_exception->exception_class, p_exception, &personality_context
         );
         if (result == unwind_reason_code::handler_found) {
            register_context scratch = frame_registers;
            if (cfi::step_frame(state, fde.cie, scratch).is_empty()) {
               return unwind_reason_code::fatal_phase1_error;
            }
            out_stop_frame_address = scratch.registers[register_rsp];
            return unwind_reason_code::handler_found;
         }
         if (result != unwind_reason_code::continue_unwind) {
            return unwind_reason_code::fatal_phase1_error;
         }
      }

      auto const new_instruction_pointer =
         cfi::step_frame(state, fde.cie, frame_registers);
      if (new_instruction_pointer.is_empty()) {
         return unwind_reason_code::end_of_stack;
      }
   }
}

// ASan inserts a pthread TLS hook before noreturn register install.
[[nodiscard, gnu::no_sanitize_address]]
auto
run_phase_two(
   register_context snapshot, unwind_exception* _Nonnull p_exception,
   uword stop_frame_address
) -> unwind_reason_code {
   register_context frame_registers = snapshot;
   cfi::fde_info fde{};

   while (true) {
      uword const instruction_pointer_before_step =
         frame_registers.registers[register_rip];

      if (instruction_pointer_before_step == 0u) {
         return unwind_reason_code::end_of_stack;
      }

      fde_lookup_result const lookup =
         find_fde_for_ip(instruction_pointer_before_step - 1u);

      if (lookup.p_fde == nullptr) {
         return unwind_reason_code::end_of_stack;
      }

      if (!cfi::parse_fde(lookup.p_fde, fde)) {
         return unwind_reason_code::fatal_phase2_error;
      }

      cfi::cfi_state state{};
      if (!cfi::compute_state_at(
             fde, instruction_pointer_before_step - 1u, state
          )) {
         return unwind_reason_code::fatal_phase2_error;
      }

      register_context scratch = frame_registers;
      if (cfi::step_frame(state, fde.cie, scratch).is_empty()) {
         return unwind_reason_code::fatal_phase2_error;
      }
      uword const this_frame_address = scratch.registers[register_rsp];

      if (fde.cie.has_personality) {
         unwind_action action = unwind_action::cleanup_phase;
         if (this_frame_address == stop_frame_address) {
            action |= unwind_action::handler_frame;
         }

         unwind_context personality_context{};
         personality_context.p_registers = &frame_registers;
         personality_context.p_fde = &fde;
         personality_context.instruction_pointer =
            instruction_pointer_before_step;
         personality_context.canonical_frame_address = this_frame_address;
         unwind_reason_code const result = __gxx_personality_v0(
            /*version=*/1, action, p_exception->exception_class, p_exception,
            &personality_context
         );

         if (result == unwind_reason_code::install_context) {
            // The register file was mutated in place by `_Unwind_SetGR`
            // and `_Unwind_SetIP` through `p_registers`.
            cat_register_context_install(&frame_registers);
         }

         if (result != unwind_reason_code::continue_unwind) {
            return unwind_reason_code::fatal_phase2_error;
         }
      }

      auto const new_instruction_pointer =
         cfi::step_frame(state, fde.cie, frame_registers);
      if (new_instruction_pointer.is_empty()) {
         return unwind_reason_code::end_of_stack;
      }
   }
}

}  // namespace

extern "C" [[gnu::noinline]]
auto
_Unwind_RaiseException(unwind_exception* _Nonnull p_exception_object)
   -> unwind_reason_code {
   register_context snapshot{};
   cat_register_context_capture(&snapshot);

   unwind_reason_code const phase1_result = run_phase_one(
      snapshot, p_exception_object, p_exception_object->private_2
   );

   if (phase1_result != unwind_reason_code::handler_found) {
      return phase1_result;
   }

   register_context phase2_snapshot{};
   cat_register_context_capture(&phase2_snapshot);

   return run_phase_two(
      phase2_snapshot, p_exception_object, p_exception_object->private_2
   );
}

extern "C" [[gnu::noinline]]
void
_Unwind_Resume(unwind_exception* p_exception_object) {
   register_context snapshot{};
   cat_register_context_capture(&snapshot);
   auto _ = run_phase_two(
      snapshot, p_exception_object, p_exception_object->private_2
   );

   terminate();
}

extern "C" void
_Unwind_DeleteException(unwind_exception* _Nonnull p_exception_object) {
   if (p_exception_object->exception_cleanup != nullptr) {
      p_exception_object->exception_cleanup(
         unwind_reason_code::foreign_exception_caught, p_exception_object
      );
   }
}

extern "C" auto
_Unwind_GetIP(unwind_context* _Nonnull p_context) -> uword {
   return p_context->instruction_pointer;
}

extern "C" auto
_Unwind_GetIPInfo(unwind_context* _Nonnull p_context, int4* _Nonnull p_before)
   -> uword {
   *p_before = 0;
   return p_context->instruction_pointer;
}

extern "C" void
_Unwind_SetIP(
   unwind_context* _Nonnull p_context, uword new_instruction_pointer
) {
   p_context->p_registers->registers[register_rip] = new_instruction_pointer;
   p_context->instruction_pointer = new_instruction_pointer;
}

extern "C" auto
_Unwind_GetGR(unwind_context* _Nonnull p_context, int4 register_number)
   -> uword {
   if (register_number < 0 || register_number >= register_count) {
      return 0u;
   }
   return p_context->p_registers
      ->registers[narrow_cast<idx>(register_number).assert()];
}

extern "C" void
_Unwind_SetGR(
   unwind_context* _Nonnull p_context, int4 register_number, uword value
) {
   if (register_number < 0 || register_number >= register_count) {
      return;
   }
   p_context->p_registers
      ->registers[narrow_cast<idx>(register_number).assert()] = value;
}

extern "C" auto
_Unwind_GetLanguageSpecificData(unwind_context* _Nonnull p_context) -> uword {
   if (p_context->p_fde == nullptr) {
      return 0u;
   }
   return p_context->p_fde->lsda_pointer;
}

extern "C" auto
_Unwind_GetRegionStart(unwind_context* _Nonnull p_context) -> uword {
   if (p_context->p_fde == nullptr) {
      return 0u;
   }
   return p_context->p_fde->initial_location;
}

extern "C" auto
_Unwind_GetCFA(unwind_context* _Nonnull p_context) -> uword {
   return p_context->canonical_frame_address;
}

extern "C" auto
_Unwind_GetDataRelBase(unwind_context* _Nonnull /*context*/) -> uword {
   // Static linking does not distinguish a data-rel base.
   // Return 0 so `DW_EH_PE_datarel` decoding stays consistent with the
   // `eh_frame_hdr` base applied inside `fde_parser`.
   return 0u;
}

extern "C" auto
_Unwind_GetTextRelBase(unwind_context* _Nonnull /*context*/) -> uword {
   return 0u;
}

}  // namespace cat::detail

#endif
