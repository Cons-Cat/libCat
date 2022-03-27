#include <string>

void meow() {
    constexpr StringView string = "Hello, Conscat!\n";
    cat::print(string).or_panic();
    cat::exit();
}
