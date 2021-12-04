#include <cstdlib>
#include <cunistd>

void meow() {
    i32 argc = load_argc();
    write(1, argc, 2).or_panic();
}
