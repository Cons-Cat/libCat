#include <string>
#include <type_traits>

void meow() {
    cat::print("Hello, Conscat!\n").or_panic();
    cat::exit();
}
