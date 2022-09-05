#include <cat/linux>

auto nix::read_char() -> ScaredyLinux<char> {
    TtyIoSerial old_settings = tty_get_attributes(stdin).value();
    TtyIoSerial new_settings = old_settings;

    new_settings.local_flags = TtyConfigurationFlags{
        cat::to_underlying(new_settings.local_flags) &
        ~(cat::to_underlying(TtyConfigurationFlags::icanon) |
          cat::to_underlying(TtyConfigurationFlags::echo))};

    auto result = tty_set_attributes(stdin, TtySetMode::now, new_settings);
    if (!result.has_value()) {
        return result.error();
    }

    char input;
    _ = sys_read(stdin, &input, 1);
    tty_set_attributes(stdin, TtySetMode::now, old_settings);
    return input;
}
