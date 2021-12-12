#include <memory.h>
#include <unistd.h>
#include <utility.h>

struct Foo {
    Foo() {
        write(1, "Foo()\n", 6).unsafe_discard();
    }
};

void func(RAII<Foo> input) {
}

void meow() {
    // Call Foo() constructor:
    RAII<Foo> foo;
    // Call Foo() constructor, then RAII() move-constructor:
    func(move(foo));
}
