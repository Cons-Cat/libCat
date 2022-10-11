#include <cat/string>
#include <cat/unique>
#include <cat/utility>

#include "../unit_tests.hpp"

int4 raii_counter = 0;

struct Foo {
    cat::String data;

    Foo() = default;

    Foo(cat::String string) : data(std::move(string)) {
        // _ = cat::print(data);
        // _ = cat::println(" constructor");
    }

    // NOLINTNEXTLINE This is a non-trivial destructor.
    ~Foo() {
        // _ = cat::print("~");
        // _ = cat::println(this->data);
    }

    auto operator=(cat::String string) {
        this->data = std::move(string);
        return *this;
    }

    void raii() const {
        // _ = cat::print(this->data);
        // _ = cat::println(" calls raii()!");
        ++raii_counter;
    }
};

// NOLINTNEXTLINE
void pass_by_value(cat::UniqueWeak<Foo>){};

TEST(test_raii) {
    // TODO: Fix `Unique` and re-enable these tests.
    // _ = cat::println("Construct objects.");
    // Test constructor.
    cat::UniqueWeak<Foo> foo(cat::String("foo"));
    // Test assignment.
    foo = cat::String("foo");
    cat::verify(foo.has_ownership());

    cat::UniqueWeak<Foo> moo(cat::String("moo"));
    cat::verify(moo.has_ownership());

    // Test move-assignment.
    // _ = cat::println("Move moo into foo.");
    foo = cat::move(moo);
    cat::verify(!moo.has_ownership());

    // _ = cat::println("Move foo into func().");
    // `cat::move()` is required:
    pass_by_value(move(foo));
    cat::verify(!foo.has_ownership());

    // This is deliberately ill-formed:
    // func(foo);

    // Default construct `	cat::Unique<Foo>`.
    cat::UniqueWeak<Foo> goo;
    cat::verify(goo.has_ownership());
    // Extract goo.
    _ = goo.borrow();
    cat::verify(!goo.has_ownership());

    // `raii()` should have been called exactly three times.
    cat::verify(raii_counter == 3);

    // Deduction guides should work here.
    cat::UniqueWeak weak = 1;
    cat::Unique unique = weak.borrow();

    // Borrowing `weak`'s data makes it lose ownership.
    cat::verify(!weak.has_ownership());
    weak = 2;
    cat::verify(weak.has_ownership());

    // Permanately transferring ownership a `cat::Unique`'s storage is unsafe,
    // but possible:
    weak = unique.borrow();
    cat::verify(weak.has_ownership());

    // `cat::Unique` can be assigned over, which will call its old data's
    // destructor.
    unique = 2;

    cat::Unique<int> original = 0;
    cat::Unique<int8> into = cat::move(original);
}
