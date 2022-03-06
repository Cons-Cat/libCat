#include <linux>

void meow() {
    int4 argc = load_argc();
    char8_t** argv = load_argv();
    for (int4 i = 1; i < argc; i++) {
        int4 length = 0;
        while (argv[i][length] != 0) {
            length++;
        }
        argv[i][length] = ' ';
        write(1, argv[i], length + 1).discard_result();
    }
    write(1, u8"\n", 1).discard_result();
    exit(0);
}
