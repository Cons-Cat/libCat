#include <cat/linux>

auto
nix::tty_set_attributes(file_descriptor tty, tty_set_mode tty_mode,
                        tty_io_serial const& configuration)
   -> scaredy_nix<void> {
   return sys_ioctl(tty, static_cast<io_requests>(tty_mode), &configuration);
}
