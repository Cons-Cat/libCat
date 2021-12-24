#include <unistd.h>

void meow() {
    debugger_entry_point();
    // TODO: I think the `Any` refactor caused this to segfault.
    Any rbp = load_base_stack_pointer();
    i4 argc = rbp;
    // TODO: Use `load_argv()`
    char8_t** argv = (char8_t**)(rbp) + 1;
    for (i4 i = 1; i < argc; i++) {
        i4 length = 0;
        while (argv[i][length] != 0) {
            length++;
        }
        argv[i][length] = ' ';
        write(1, argv[i], length + 1).discard_result();
    }
    write(1, u8"\n", 1).discard_result();
    exit(0);
}
