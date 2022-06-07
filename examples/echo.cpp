#include <cat/linux>

// TODO: Simplify with `cat::Span`.
// TODO: Make this cross-platform.
void meow(int argc, char* p_argv[]) {
    for (ssize i = 1; i < argc; ++i) {
        ssize length = 0;
        // TODO: Use a `string_length_scalar()`.
        while (p_argv[i.raw][length.raw] != 0) {
            ++length;
        }

        p_argv[i.raw][length.raw] = ' ';
        _ = nix::write(nix::FileDescriptor{1}, p_argv[i.raw], length + 1);
    }

    _ = nix::write(nix::FileDescriptor{1}, "\n");
    cat::exit();
}
