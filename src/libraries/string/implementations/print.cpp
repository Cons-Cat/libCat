#include <cat/linux>
#include <cat/string>

auto
cat::print(str_view const string) -> maybe_idx {
   // The only way this function can fail is either `sys_write()` returns
   // `nix::linux_error::nospc`, or it prints fewer characters than intended.
   idx length = prop_as(nix::sys_write(nix::stdout, string), nullopt);
   if (length < string.size()) {
      return nullopt;
   }
   return length;
}
