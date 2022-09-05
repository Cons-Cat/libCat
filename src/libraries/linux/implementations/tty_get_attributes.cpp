#include <cat/linux>

auto nix::tty_get_attributes(FileDescriptor terminal)
    -> cat::Scaredy<TtyIoSerial, LinuxError> {
    TtyIoSerial configuration;
    cat::Scaredy result =
        sys_ioctl(terminal, IoRequests::tcgets, &configuration);
    if (result.has_value()) {
        return configuration;
    }
    return result.error();
}
