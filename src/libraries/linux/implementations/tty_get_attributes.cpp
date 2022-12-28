#include <cat/linux>

auto nix::tty_get_attributes(file_descriptor terminal)
    -> cat::scaredy<TtyIoSerial, linux_error> {
    TtyIoSerial configuration;
    cat::scaredy result =
        sys_ioctl(terminal, io_requests::tcgets, &configuration);
    if (result.has_value()) {
        return configuration;
    }
    return result.error();
}
