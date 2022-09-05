#include <cat/linux>

auto nix::terminal_set_attributes(FileDescriptor terminal,
                                  TerminalSetMode terminal_mode,
                                  TerminalIoSerial const& configuration)
    -> ScaredyLinux<void> {
    return sys_ioctl(terminal, static_cast<IoRequests>(terminal_mode),
                     &configuration);
}
