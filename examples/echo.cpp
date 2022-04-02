#include <linux>

// TODO: Make this cross-platform.
void meow(int argc, char* p_argv[]) {
    for (int4 i = 1; i < argc; i++) {
        int4 length = 0;
        while (p_argv[i][length] != 0) {
            length++;
        }
        p_argv[i][length] = ' ';
        nix::write(1, p_argv[i], length + 1).discard_result();
    }

    nix::write(1, "\n", 1).discard_result();
    cat::exit();
}
