#include <cat/bit>
#include <cat/math>
#include <cat/runtime>
#include <cat/stacktrace>

namespace cat {
namespace {

struct stack_frame {
   stack_frame* p_next;
   void* p_instruction;
};

[[nodiscard]]
auto
is_readable_frame(
   stack_frame* p_frame, uintptr<void> stack_low, uintptr<void> stack_high
) -> bool {
   if (p_frame == nullptr) {
      return false;
   }

   uintptr<void> const address = static_cast<void*>(p_frame);
   if (address <= stack_low || address >= stack_high) {
      return false;
   }
   
   return is_aligned(p_frame, alignof(stack_frame));
}

[[nodiscard]]
auto
is_plausible_instruction(void* p_instruction) -> bool {
   return uintptr<void>(p_instruction) >= page_size;
}

}  // namespace

auto
stacktrace::current(dyn_allocator allocator, idx skip, idx max_depth)
   -> maybe<stacktrace> {
   stacktrace trace(allocator);
   if (max_depth == 0u) {
      return move(trace);
   }

   uintptr<void> const stack_low = get_stack_pointer();
   uintptr<void> const stack_high = stack_low + 8_umi;
   auto* p_frame = static_cast<stack_frame*>(__builtin_frame_address(0));
   void* p_instruction = __builtin_return_address(0);
   idx frame_index = 0u;

   while (trace.size() < max_depth) {
      if (!is_plausible_instruction(p_instruction)) {
         break;
      }

      if (frame_index >= skip) {
         $prop(trace.m_frames.push_back(stacktrace_entry(p_instruction)));
      }

      ++frame_index;
      if (!is_readable_frame(p_frame, stack_low, stack_high)) {
         break;
      }

      stack_frame* const p_next = p_frame->p_next;
      if (!is_readable_frame(p_next, stack_low, stack_high)) {
         break;
      }

      if (
         uintptr<void>(static_cast<void*>(p_next))
         <= uintptr<void>(static_cast<void*>(p_frame))
      ) {
         break;
      }

      p_instruction = p_next->p_instruction;
      p_frame = p_next;
   }

   return move(trace);
}

}  // namespace cat
