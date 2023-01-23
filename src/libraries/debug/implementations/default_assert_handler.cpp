#include <cat/debug>
#include <cat/format>
#include <cat/page_allocator>

void cat::detail::print_assert_location(source_location const& callsite) {
    page_allocator allocator;
    // TODO: This will leak. An `inline_allocator` should be used.
    _ = eprint(format(allocator, "assert failed on line {}, in:\n    ",
                      callsite.line())
                   .or_exit());
    // TODO: Truncate to only the last one or two directories.
    _ = eprint(callsite.file_name());
    _ = eprint("\ncalled from:\n    ");
    _ = eprintln(callsite.function_name());
}

void cat::default_assert_handler(source_location const& callsite) {
    detail::print_assert_location(callsite);

    // TODO: Colorize this input prompt.
    _ = print("Press: 1 (Continue), 2 (Debug), 3 (Abort)\n");

    while (true) {
        unsigned char input = nix::read_char().or_exit();
        if (input >= '1' && input <= '3') {
            // ASCII trick that converts an inputted char to a digit.
            uint1 digit = input - 49_u1;

            // The value of `digit` is one less than what was inputted.
            switch (digit.raw) {
                case 0:
                    // Ignore the assert failure.
                    return;
                case 1:
                    // Break in a debugger.
                    breakpoint();
                    return;
                case 2:
                    // Abort the program.
                    _ = eprint("Program aborted!\n");
                    exit(1);
                default:
                    __builtin_unreachable();
            }

            return;
        }
        _ = eprint("Invalid input!\n");
    }
}
