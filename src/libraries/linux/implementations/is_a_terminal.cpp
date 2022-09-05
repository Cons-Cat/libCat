#include <cat/linux>

auto nix::is_a_terminal(FileDescriptor file_descriptor) -> ScaredyLinux<void> {
    // `size` must be written into, but it is not returned.
    TerminalSize size;
    return sys_ioctl(file_descriptor, IoRequests::tiocgwinsz, &size);
}

auto nix::is_a_terminal(TerminalDescriptor) -> ScaredyLinux<void> {
    return monostate;
}
