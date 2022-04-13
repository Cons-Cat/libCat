#include <string>

void meow() {
    cat::print("Hello, world!\n").discard_result();
    cat::exit();
}
