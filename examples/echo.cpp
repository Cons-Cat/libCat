#include <linux>

// TODO: Simplify with `Span`.
// TODO: Make this cross-platform.
void meow(int argc, char* p_argv[]) {
    for (int4 i = 1; i < argc; i++) {
        int4 length = 0;
        while (p_argv[i.c()][length.c()] != 0) {
            length++;
        }
        p_argv[i.c()][length.c()] = ' ';
        _ = nix::write(nix::FileDescriptor{1}, p_argv[i.c()], length + 1);
    }

    _ = nix::write(nix::FileDescriptor{1}, "\n", 1);
    cat::exit();
}
