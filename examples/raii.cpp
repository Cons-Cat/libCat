#include <memory.h>
#include <unistd.h>
#include <utility.h>

struct Foo {
    Foo() {
        write(1, u8"Foo()\n", 6).discard_result();
    }
    ~Foo() {
        write(1, u8"~Foo()\n", 7).discard_result();
    }
};

void func(RAII<Foo>) {
}

void meow() {
    debugger_entry_point();
    // Call Foo() constructor:
    RAII<Foo> foo;
    /* Call Foo() constructor, then RAII() move-constructor, then Foo()
     * destructor: */
    func(meta::move(foo));
}
