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
    return;
}

void meow() {
    // Call `Foo()` constructor:
    Raii<Foo> foo;
    /* Call `Foo()` constructor, then `Raii()` move-constructor, then `Foo()`
     * destructor: */
    func(meta::move(foo));
    std::exit();
}
