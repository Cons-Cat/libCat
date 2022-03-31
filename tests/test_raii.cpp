#include <linux>
#include <memory>
#include <utility>

struct Foo {
    Foo() {
        cat::print("Foo()\n").or_panic();
    }
    ~Foo() {
        cat::print("~Foo()\n").or_panic();
    }
};

void func(Raii<Foo>) {
}

void meow() {
    // Call `Foo()` constructor:
    Raii<Foo> foo;
    /* Call `Foo()` constructor, then `Raii()` move-constructor, then `Foo()`
     * destructor: */
    func(meta::move(foo));
    cat::exit();
}
