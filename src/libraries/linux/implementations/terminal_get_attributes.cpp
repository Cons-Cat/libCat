#include <cat/linux>

auto nix::terminal_get_attributes(FileDescriptor terminal)
    -> cat::Scaredy<TerminalIoSerial, LinuxError> {
    TerminalIoSerial configuration;
    cat::Scaredy result =
        sys_ioctl(terminal, IoRequests::tcgets, &configuration);
    if (result.has_value()) {
        return configuration;
    }
    return result.error();
}
