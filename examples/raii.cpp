#include <linux>
#include <memory.h>
#include <utility>

struct Foo {
    Foo() {
        std::print("Foo()\n").or_panic();
    }
    ~Foo() {
        std::print("~Foo()\n").or_panic();
    }
};

void func(Raii<Foo>) {
}

void meow() {
    // Call Foo() constructor:
    Raii<Foo> foo;
    /* Call Foo() constructor, then RAII() move-constructor, then Foo()
     * destructor: */
    func(meta::move(foo));
    std::exit();
}
