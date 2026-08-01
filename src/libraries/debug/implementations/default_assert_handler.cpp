#include <cat/detail/itoa_jeaiii.hpp>

#include <cat/debug>
#include <cat/linux>
#include <cat/string>

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

void
cat::default_assert_handler(source_location const& callsite) {
   detail::print_assert_location(callsite);

   if (!nix::is_a_tty(nix::stdin).has_value()) {
      eprint("assert failed with stdin not a tty; exiting.\n").or_exit();
      exit(1);
   }

   // TODO: Colorize this input prompt.
   print("Press: 1 (Continue), 2 (Debug), 3 (Abort)\n").or_exit();

   while (true) {
      unsigned char const input = nix::read_char().or_exit();
      if (input >= '1' && input <= '3') {
         // ASCII trick that converts an inputted `char` to a digit.
         uint1 const digit = input - 49_u1;

         // The value of `digit` is one less than what was inputted.
         switch (digit.raw) {
            case 0:
               // Ignore the assert failure.
               return;
            case 1:
               // Historically this called `breakpoint()` (`int3`). That
               // surfaces as SIGTRAP under Release plus sanitizers or ambiguous
               // stdin. Attach a debugger at `default_assert_handler` instead.
               eprint("Debug trap is disabled; exiting.\n").or_exit();
               exit(1);
            case 2:
               {
                  // Abort the program.
                  eprint("Program aborted!\n").or_exit();
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
