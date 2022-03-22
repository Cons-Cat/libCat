#include <string>

void meow() {
    constexpr StringView string = "Hello, Conscat!\n";
    std::print(string).or_panic();
    std::exit();
}
