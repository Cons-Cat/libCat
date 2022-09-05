#include <cat/linux>

auto nix::is_a_tty(FileDescriptor file_descriptor) -> ScaredyLinux<void> {
    // `size` must be written into, but it is not returned.
    TtySize size;
    return sys_ioctl(file_descriptor, IoRequests::tiocgwinsz, &size);
}

auto nix::is_a_tty(TtyDescriptor) -> ScaredyLinux<void> {
    return monostate;
}
