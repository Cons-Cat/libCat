#include <cat/linux>

auto
nix::tty_get_attributes(file_descriptor tty)
   -> cat::scaredy<tty_io_serial, linux_error> {
   // `&configuration` is an output parameter.
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
   tty_io_serial configuration;
   cat::scaredy result = sys_ioctl(tty, io_requests::tcgets, &configuration);

   if (result.has_value()) {
      // `configuration` is initialized if `result` is a success.
      return configuration;
   }
   return result.error();
}
