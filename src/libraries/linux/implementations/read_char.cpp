#include <cat/linux>

auto nix::read_char() -> ScaredyLinux<char> {
    TerminalIoSerial old_settings = terminal_get_attributes(stdin).value();
    TerminalIoSerial new_settings = old_settings;

    new_settings.local_flags = TerminalConfigurationFlags{
        cat::to_underlying(new_settings.local_flags) &
        ~(cat::to_underlying(TerminalConfigurationFlags::icanon) |
          cat::to_underlying(TerminalConfigurationFlags::echo))};

    auto result =
        terminal_set_attributes(stdin, TerminalSetMode::now, new_settings);
    if (!result.has_value()) {
        return result.error();
    }

    char input;
    _ = sys_read(stdin, &input, 1);
    terminal_set_attributes(stdin, TerminalSetMode::now, old_settings);
    return input;
}
