#include <any>
#include <cunistd>

void meow() {
    std::any rbp = load_base_stack_pointer();
    i32 argc = *std::any_cast<i32*>(rbp);
    char** argv = std::any_cast<char**>(rbp) + 1;
    for (i32 i = 1; i < argc; i++) {
        i32 len = 0;
        while (argv[i][len] != 0) {
            len++;
        }
        argv[i][len] = ' ';
        write(1, argv[i], len + 1).or_panic_debug();
    }
    write(1, "\n", 1).or_panic_debug();
}
