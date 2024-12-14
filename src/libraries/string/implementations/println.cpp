#include <cat/linux>
#include <cat/string>

auto
cat::println(str_view string) -> maybe_idx {
   // There is no reasonable way for a `write` syscall for `nix::stdout` to
   // fail, except by running out of buffer space, which fails gracefully
   // anyways.
   idx length = nix::sys_write(nix::stdout, string).value();
   length += nix::sys_write(nix::stdout, "\n").value();
   if (length < string.size() + 1) {
      return nullopt;
   }
   return length;
}

auto
cat::println() -> maybe_idx {
   idx length = nix::sys_write(nix::stdout, "\n").value();
   if (length == 0) {
      return nullopt;
   }
   return length;
}
