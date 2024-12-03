#include <cat/linux>
#include <cat/string>

// TODO: Return `idx` here.
auto
cat::println(str_view string) -> iword {
   // There is no reasonable way for a `write` syscall for `nix::stdout` to
   // fail, except by running out of buffer space, which fails gracefully
   // anyways.
   iword output_length = nix::sys_write(nix::stdout, string).value();
   output_length += nix::sys_write(nix::stdout, "\n").value();
   return output_length;
}

auto
cat::println() -> iword {
   return nix::sys_write(nix::stdout, "\n").value();
}
