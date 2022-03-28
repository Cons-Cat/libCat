#include <string>
#include <type_traits>

void meow() {
    cat::print(meta::constant_evaluate(StringView("Hello, Conscat!\n")));
    cat::exit();
}
