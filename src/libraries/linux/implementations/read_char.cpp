#include <cat/linux>

auto
nix::read_char() -> scaredy_nix<char> {
   // This is functionally similar to POSIX `getchar()`.
   // TODO: A buffered implementation could be much more efficient.
   tty_io_serial const old_settings = prop(tty_get_attributes(nix::stdin));
   tty_io_serial new_settings = old_settings;

   new_settings.local_flags = tty_configuration_flags{
      cat::to_underlying(new_settings.local_flags)
         & ~(cat::to_underlying(tty_configuration_flags::icanon)
             | cat::to_underlying(tty_configuration_flags::echo)),
   };

   prop(tty_set_attributes(nix::stdin, tty_set_mode::now, new_settings));

   char input{};
   prop(sys_read(nix::stdin, &input, 1));

   // If we fail to reset the TTY mode, we're in trouble.
   tty_set_attributes(nix::stdin, tty_set_mode::now, old_settings).assert();

   return input;
}
