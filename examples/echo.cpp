#include <linux>

void meow(int argc, char* argv[]) {
    for (int4 i = 1; i < argc; i++) {
        int4 length = 0;
        while (argv[i][length] != 0) {
            length++;
        }
        argv[i][length] = ' ';
        write(1, argv[i], length + 1).discard_result();
    }
    write(1, "\n", 1).discard_result();
    exit();
}
