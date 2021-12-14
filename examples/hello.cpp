#include <stdlib.h>
#include <unistd.h>

void meow() {
    debugger_entry_point();
    char const* p_str = "Hello, Conscat!\n";
    write(1, p_str, 16).or_panic();
}
