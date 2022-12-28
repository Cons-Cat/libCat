#include <cat/linux>

auto nix::read_char() -> scaredy_nix<char> {
    TtyIoSerial old_settings = tty_get_attributes(stdin).value();
    TtyIoSerial new_settings = old_settings;

    new_settings.local_flags = tty_configuration_flags{
        cat::to_underlying(new_settings.local_flags) &
        ~(cat::to_underlying(tty_configuration_flags::icanon) |
          cat::to_underlying(tty_configuration_flags::echo))};

    auto result = tty_set_attributes(stdin, TtySetMode::now, new_settings);
    if (!result.has_value()) {
        return result.error();
    }

    char input;
    _ = sys_read(stdin, &input, 1);
    tty_set_attributes(stdin, TtySetMode::now, old_settings);
    return input;
}
