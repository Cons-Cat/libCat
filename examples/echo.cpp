#include <cat/linux>

// TODO: Simplify with `cat::span`.
// TODO: Make this cross-platform.
auto main(int argc, char* p_argv[]) -> int {
    for (ssize i = 1; i < argc; ++i) {
        ssize length = 0;
        // TODO: Use a `string_length_scalar()`.
        while (p_argv[i.raw][length.raw] != 0) {
            ++length;
        }

        p_argv[i.raw][length.raw] = ' ';
        _ = nix::sys_write(nix::file_descriptor(1), p_argv[i.raw], length + 1);
    }

    _ = nix::sys_write(nix::file_descriptor(1), "\n");
}
