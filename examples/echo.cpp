#include <cat/linux>

// TODO: This seems to be broken in -O0, but not -03.
// TODO: Simplify with `cat::Span`.
// TODO: Make this cross-platform.
void meow(int argc, char* p_argv[]) {
    for (int4 i = 1; i < argc; i++) {
        int4 length = 0;
        // TODO: Use a `string_length_scalar()`.
        while (p_argv[i.raw][length.raw] != 0) {
            length++;
        }

        p_argv[i.raw][length.raw] = ' ';
        _ = nix::write(nix::FileDescriptor{1}, p_argv[i.raw], length + 1);
    }

    _ = nix::write(nix::FileDescriptor{1}, "\n", 1);
    cat::exit();
}
