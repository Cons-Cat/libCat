#include <any.h>
#include <unistd.h>

void meow() {
    debugger_entry_point();
    Any rbp = load_base_stack_pointer();
    i32 argc = *any_cast<i32*>(rbp);
    char8_t** argv = any_cast<char8_t**>(rbp) + 1;
    for (i32 i = 1; i < argc; i++) {
        i32 length = 0;
        while (argv[i][length] != 0) {
            length++;
        }
        argv[i][length] = ' ';
        write(1, argv[i], length + 1).discard_result();
    }
    write(1, u8"\n", 1).discard_result();
}
