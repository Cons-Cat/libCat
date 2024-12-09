#include <cat/linux>

auto
nix::read_char() -> scaredy_nix<char> {
   tty_io_serial old_settings = tty_get_attributes(stdin).value();
   tty_io_serial new_settings = old_settings;

   new_settings.local_flags = tty_configuration_flags{
      cat::to_underlying(new_settings.local_flags)
      & ~(cat::to_underlying(tty_configuration_flags::icanon)
          | cat::to_underlying(tty_configuration_flags::echo))};

   prop(tty_set_attributes(stdin, tty_set_mode::now, new_settings));

   char input;
   auto _ = sys_read(stdin, &input, 1);
   tty_set_attributes(stdin, tty_set_mode::now, old_settings);
   return input;
}
