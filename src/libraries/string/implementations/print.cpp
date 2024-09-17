#include <cat/linux>
#include <cat/string>

auto
cat::print(str_span const string) -> iword {
    // There is no reasonable way for a `write` syscall for `nix::stdout` to
    // fail, except by running out of buffer space, which fails gracefully
    // anyways.
    return nix::sys_write(nix::stdout, string).value();
}
