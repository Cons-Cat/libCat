#include <cat/detail/itoa_jeaiii.hpp>

#include <cat/debug>
#include <cat/format>
#include <cat/iterable>
#include <cat/linux>
#include <cat/page_allocator>
#include <cat/stacktrace>
#include <cat/string>

namespace {

// Spelling out the overloads of `cat::detail::assert_failed()` makes them
// addressable below.
using assert_failed_function =
   void (*_Nonnull)(cat::assert_handler, cat::source_location const&);
using assert_failed_message_function = void (*_Nonnull)(
   cat::str_view const&, cat::assert_handler, cat::source_location const&
);

// Compare an address to the above functions' addresses.
[[nodiscard]]
auto
is_assert_internal_function(void* _Nullable p_function) -> bool {
   return p_function == __builtin_bit_cast(void*, &cat::default_assert_handler)
          || p_function
                == __builtin_bit_cast(
                   void*, static_cast<assert_failed_function>(
                             &cat::detail::assert_failed
                          )
                )
          || p_function
                == __builtin_bit_cast(
                   void*, static_cast<assert_failed_message_function>(
                             &cat::detail::assert_failed
                          )
                );
}

// A stack trace taken inside `default_assert_handler()` walks out through the
// assert handler machinery before it reaches the code that failed.
[[nodiscard]]
auto
is_assert_frame(
   cat::detail::symbolizer const* _Nullable p_symbolizer,
   cat::stacktrace_entry entry
) -> bool {
   cat::detail::frame_origin const origin =
      cat::detail::resolve_stacktrace_frame(p_symbolizer, entry);
   // `verify()` and `assert()` might be inlined or duplicated by LTO, so we
   // might have to find them in DWARF tables.
   // TODO: Can we eliminate `.or_assert()` calls as well?
   return origin.file.find("libraries/debug/").has_value()
          || is_assert_internal_function(origin.p_function);
}

}  // namespace

void
cat::detail::print_assert_location(source_location const& callsite) {
   // Format the line number into a stack buffer. Avoid `fmt` so assert does not
   // pull the full formatter or Dragonbox into every binary.
   char line_buffer[16];
   char* const p_end =
      u32toa_jeaiii(static_cast<uint4::raw_type>(callsite.line()), line_buffer);
   str_view const line_string(line_buffer, idx(p_end - line_buffer));

   auto _ = eprint("assert failed on line ");
   auto _ = eprint(line_string);
   auto _ = eprint(", in:\n    ");
   // TODO: Truncate to only the last one or two directories.
   auto _ = eprint(callsite.file_name());
   auto _ = eprint("\ncalled from:\n    ");
   // Any failures to print text will cascade to the last `eprint()` call, so
   // only handle failure there.
   eprintln(callsite.function_name()).or_exit();
}

#ifndef CAT_NO_STACKTRACE
// Tail call optimization puts `cat::fmt()` calls into the stack trace.
[[clang::disable_tail_calls]]
#endif
void
cat::default_assert_handler(source_location const& callsite) {
   // We can't propagate internal errors out of assert handlers, so they fail
   // fast.
   detail::print_assert_location(callsite);

   if (nix::is_a_tty(nix::stdin).is_empty()) {
      eprint("Program is not running in an interactive tty!\n").or_exit();
      exit(1);
   }

   // TODO: Colorize this input prompt.
#ifdef CAT_NO_STACKTRACE
   constexpr str_view prompt = "Press: 1 (Continue), 2 (Debug), 3 (Abort)\n";
#else
   constexpr str_view prompt = "Press: 1 (Continue), 2 (Debug), 3 (Stack "
                               "trace), 4 (Verbose stack trace), 5 (Abort)\n";
#endif
   print(prompt).or_exit();

   while (true) {
      unsigned char const input = nix::read_char().or_exit();
#ifdef CAT_NO_STACKTRACE
      if (input >= '1' && input <= '3') {
#else
      if (input >= '1' && input <= '5') {
#endif
         // ASCII trick that converts an inputted `char` to a digit.
         uint1 const digit = input - 49_u1;

         // The value of `digit` is one less than what was inputted.
         switch (digit.raw) {
            case 0:
               // Ignore the assert failure.
               return;
            case 1:
               // Break into a debugger.
               breakpoint();
               return;
            case 2:
#ifndef CAT_NO_STACKTRACE
               {
                  // Print a trace, then ask again.
                  page_allocator allocator;
                  maybe<stacktrace> const trace =
                     stacktrace::current(allocator);
                  if (trace.has_value()) {
                     detail::symbolizer* _Nullable const p_symbolizer =
                        detail::load_symbolizer();
                     span<stacktrace_entry const> trace_frames(trace.value());
                     auto const trace_failure =
                        trace_frames
                        | drop_while([p_symbolizer](stacktrace_entry entry) {
                             return is_assert_frame(p_symbolizer, entry);
                          });
                     eprint_fmt(allocator, "\n{}\n", trace_failure).or_exit();
                     detail::unload_symbolizer(p_symbolizer);
                  } else {
                     eprint("\nStack trace is unavailable!\n\n").or_exit();
                  }
                  print(prompt).or_exit();
                  continue;
               }
            case 3:
               {
                  // Print a verbose stack trace, then ask again.
                  page_allocator allocator;
                  maybe<stacktrace> const trace =
                     stacktrace::current(allocator);
                  if (trace.has_value()) {
                     detail::symbolizer* _Nullable const p_symbolizer =
                        detail::load_symbolizer();
                     span<stacktrace_entry const> trace_frames(trace.value());
                     auto const trace_failure =
                        trace_frames
                        | drop_while([p_symbolizer](stacktrace_entry entry) {
                             return is_assert_frame(p_symbolizer, entry);
                          });
                     eprint_fmt(allocator, "\n{:?}\n", trace_failure).or_exit();
                     detail::unload_symbolizer(p_symbolizer);
                  } else {
                     eprint("\nStack trace is unavailable!\n\n").or_exit();
                  }
                  print(prompt).or_exit();
                  continue;
               }
#endif
               // When stack traces are disabled, `case 2` maps to `case 4`.
               [[fallthrough]];
            case 4:
               {
                  // Abort the program.
                  eprint("\nProgram aborted!\n").or_exit();
                  exit(1);
               }
            default:
               __builtin_unreachable();
         }

         return;
      }
      eprint("Invalid input!\n").or_exit();
   }
}
