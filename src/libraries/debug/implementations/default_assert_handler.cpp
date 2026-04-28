#include <cat/debug>
#include <cat/format>
#include <cat/linux>
#include <cat/page_allocator>

void
cat::detail::print_assert_location(source_location const& callsite) {
   page_allocator allocator;
   // TODO: This will leak. An `inline_allocator` should be used.
   auto _ = eprint(
      // This `idx` cast is very unlikely to overflow, so it's unchecked.
      fmt(allocator, "assert failed on line {}, in:\n    ",
          cat::idx(static_cast<cat::idx::raw_type>(callsite.line())))
         .or_exit());
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
