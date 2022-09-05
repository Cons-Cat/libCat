#include <cat/linux>

auto nix::tty_set_attributes(FileDescriptor terminal, TtySetMode terminal_mode,
                             TtyIoSerial const& configuration)
    -> ScaredyLinux<void> {
    return sys_ioctl(terminal, static_cast<IoRequests>(terminal_mode),
                     &configuration);
}
