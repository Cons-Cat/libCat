#if __has_feature(cxx_exceptions)
#include <cat/detail/cxa_abi.hpp>
#include <cat/detail/dwarf_cfi.hpp>
#include <cat/detail/eh_frame_hdr.hpp>

#include <cat/runtime>

// Itanium LSDA dispatch: search for a handler, then install cleanup/catch pads.
namespace cat::detail {
extern "C" auto
// NOLINTNEXTLINE(bugprone-reserved-identifier)
__gxx_personality_v0(
   int4 version, unwind_action actions, uint8 exception_class,
   unwind_exception* p_exception, unwind_context* p_context
) noexcept(!__has_feature(cxx_exceptions)) -> unwind_reason_code {
   if (version != 1 || !is_native_exception(exception_class)) {
      return unwind_reason_code::fatal_phase1_error;
   }

   auto const lsda = _Unwind_GetLanguageSpecificData(p_context);
   if (lsda == 0u) {
      return unwind_reason_code::continue_unwind;
   }

   auto const section = eh_except_table_span();
   auto const* p_data = __builtin_bit_cast(byte const*, lsda);
   auto const* p_end = section.data() + section.size();
   if (p_data < section.data() || p_data >= p_end) {
      terminate();
   }

   byte_cursor cursor{
      .p_data = p_data,
      .position = 0u,
      .limit = narrow_cast<idx>(p_end - p_data).assert(),
   };

   auto const read_pointer = [&](dw_eh_pe encoding) {
      return decode_eh_pointer(
         cursor, encoding, __builtin_bit_cast(uword, cursor.here()),
         {.function = _Unwind_GetRegionStart(p_context)}
      );
   };

   auto const lp_encoding = cursor.read<dw_eh_pe>();
   auto const lp_start = lp_encoding == dw_eh_pe::omit
                            ? _Unwind_GetRegionStart(p_context)
                            : read_pointer(lp_encoding);
   auto const type_encoding = cursor.read<dw_eh_pe>();
   byte const* p_types = nullptr;

   if (type_encoding != dw_eh_pe::omit) {
      auto const offset = cursor.read_uleb128();
      if (offset > cursor.remaining()) {
         terminate();
      }
      p_types = cursor.here() + offset;
   }

   auto const call_encoding = cursor.read<dw_eh_pe>();
   auto const call_bytes = cursor.read_uleb128();

   if (call_bytes > cursor.remaining()) {
      terminate();
   }
   auto const calls_end = cursor.position + call_bytes;
   auto const* p_action_table = p_data + calls_end;
   auto const pc =
      _Unwind_GetIP(p_context) - 1u - _Unwind_GetRegionStart(p_context);
   uword landing_pad = 0u;
   uint8 action = 0u;

   while (cursor.position < calls_end) {
      auto const start = read_pointer(call_encoding);
      auto const length = read_pointer(call_encoding);
      auto const landing = read_pointer(call_encoding);
      auto const entry_action = cursor.read_uleb128();

      if (pc >= start && pc - start < length) {
         landing_pad = landing;
         action = entry_action;
         break;
      }
   }

   if (landing_pad == 0u) {
      return unwind_reason_code::continue_unwind;
   }

   bool cleanup = action == 0u;
   int8 selector = 0;
   auto* p_header = header_from_unwind(p_exception);
   void* p_adjusted = p_header + 1;

   if (action != 0u) {
      if (action > uword(p_end - p_action_table)) {
         terminate();
      }

      auto const* p_record = p_action_table + action - 1u;
      // A valid action chain cannot visit more records than there are bytes.
      for (auto remaining = p_end - p_record; remaining > 0; --remaining) {
         byte_cursor entry{
            .p_data = p_record,
            .position = 0u,
            .limit = narrow_cast<idx>(p_end - p_record).assert(),
         };

         auto const filter = entry.read_sleb128();
         auto const* p_displacement_base = entry.here();
         auto const displacement = entry.read_sleb128();

         if (filter == 0) {
            cleanup = true;
         } else if (filter > 0 && selector == 0) {
            auto const size = eh_pointer_size(type_encoding);
            if (
               p_types == nullptr || size == 0u
               || uint8(filter) > uword(p_types - p_data) / size
            ) {
               terminate();
            }

            auto const* p_slot = p_types - filter * size;
            byte_cursor type_cursor{
               .p_data = p_slot,
               .position = 0u,
               .limit = size,
            };

            auto const type = decode_eh_pointer(
               type_cursor, type_encoding, __builtin_bit_cast(uword, p_slot), {}
            );

            void* p_candidate = p_header + 1;
            if (
               type == 0u
               || exception_matches(
                  __builtin_bit_cast(void const*, type), p_candidate,
                  p_header->type
               )
            ) {
               selector = filter;
               p_adjusted = p_candidate;
            }
         } else if (filter < 0) {
            // A negative filter is not supported by Itanium since C++17.
            terminate();
         }
         if (displacement == 0) {
            break;
         }

         auto const offset = (p_displacement_base - p_data) + displacement;
         if (offset < 0 || offset >= p_end - p_data) {
            terminate();
         }

         p_record = p_data + offset;
         if (remaining == 1) {
            terminate();
         }
      }
   }

   if ((actions & unwind_action::search_phase) != unwind_action::none) {
      return selector != 0 ? unwind_reason_code::handler_found
                           : unwind_reason_code::continue_unwind;
   }

   bool const handler =
      (actions & unwind_action::handler_frame) != unwind_action::none;
   if (handler ? selector == 0 : !cleanup) {
      return unwind_reason_code::continue_unwind;
   }

   if (handler) {
      p_header->adjusted = p_adjusted;
   }

   _Unwind_SetGR(p_context, 0, __builtin_bit_cast(uword, p_exception));
   _Unwind_SetGR(p_context, 1, handler ? uword(selector) : uword(0u));
   _Unwind_SetIP(p_context, lp_start + landing_pad);
   return unwind_reason_code::install_context;
}

}  // namespace cat::detail

#endif
