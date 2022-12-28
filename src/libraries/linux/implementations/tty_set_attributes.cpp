#include <cat/linux>

auto nix::tty_set_attributes(file_descriptor terminal, TtySetMode terminal_mode,
                             TtyIoSerial const& configuration)
    -> scaredy_nix<void> {
    return sys_ioctl(terminal, static_cast<io_requests>(terminal_mode),
                     &configuration);
}
