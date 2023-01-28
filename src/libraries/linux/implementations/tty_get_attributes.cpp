#include <cat/linux>

auto nix::tty_get_attributes(file_descriptor terminal)
    -> cat::scaredy<tty_io_serial, linux_error> {
    tty_io_serial configuration;
    cat::scaredy result =
        sys_ioctl(terminal, io_requests::tcgets, &configuration);
    if (result.has_value()) {
        return configuration;
    }
    return result.error();
}
