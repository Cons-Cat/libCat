#include <string>

void meow() {
    StringView string = "Hello, Conscat!\n";
    std::print(string).or_panic();
    std::exit();
}
