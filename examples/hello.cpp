#include <string>
#include <type_traits>

void meow() {
    cat::print("Hello, Conscat!\n").discard_result();
    cat::exit();
}
