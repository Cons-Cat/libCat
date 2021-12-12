#include <any.h>
#include <unistd.h>

void meow() {
    Any rbp = load_base_stack_pointer();
    i32 argc = *any_cast<i32*>(rbp);
    char** argv = any_cast<char**>(rbp) + 1;
    for (i32 i = 1; i < argc; i++) {
        i32 length = 0;
        while (argv[i][length] != 0) {
            length++;
        }
        argv[i][length] = ' ';
        write(1, argv[i], length + 1).unsafe_discard();
    }
    write(1, "\n", 1).unsafe_discard();
}
