#include <cat/linux>

// TODO: Simplify with `cat::span`.
// TODO: Make this cross-platform.
auto
main(int argc, char* p_argv[]) -> int {
    for (cat::iword i = 1; i < argc; ++i) {
        cat::iword length = 0;
        // TODO: Use a `string_length_scalar()`.
        while (p_argv[i.get_raw()][length.get_raw()] != 0) {
            ++length;
        }

        p_argv[i.get_raw()][length.get_raw()] = ' ';
        auto _ = nix::sys_write(nix::file_descriptor(1), p_argv[i.get_raw()],
                                length + 1);
    }

    auto _ = nix::sys_write(nix::file_descriptor(1), "\n");
}
