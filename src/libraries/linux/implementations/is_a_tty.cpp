#include <cat/linux>

auto nix::is_a_tty(file_descriptor file_descriptor) -> scaredy_nix<void> {
    // `size` must be written into, but it is not returned.
    tty_window_size size;
    return sys_ioctl(file_descriptor, io_requests::tiocgwinsz, &size);
}

auto nix::is_a_tty(tty_descriptor) -> scaredy_nix<void> {
    return monostate;
}
