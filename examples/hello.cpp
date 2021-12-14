#include <stdlib.h>
#include <unistd.h>

void meow() {
    debugger_entry_point();
    char8_t const* p_str = u8"Hello, Conscat!\n";
    write(1, p_str, 16).or_panic();
}
