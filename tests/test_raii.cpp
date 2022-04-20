#include <string>
#include <unique>
#include <utility>

int4 global = 0;

struct Foo {
    String data;

    Foo() = default;

    Foo(String string) : data(std::move(string)) {
        _ = cat::print(data);
        _ = cat::print_line(" constructor");
    }

    ~Foo() {
        _ = cat::print("~");
        _ = cat::print_line(this->data);
    }

    auto operator=(String string) {
        this->data = std::move(string);
        return *this;
    }

    void raii() const {
        _ = cat::print(this->data);
        _ = cat::print_line(" calls raii()!");
        global++;
    }
};

void func(UniqueWeak<Foo>){};

void meow() {
    _ = cat::print_line("Construct objects.");
    // Test constructor.
    UniqueWeak<Foo> foo(String("foo"));
    // Test assignment.
    foo = "foo";
    Result(foo.has_ownership()).or_panic();

    UniqueWeak<Foo> moo(String("moo"));
    Result(moo.has_ownership()).or_panic();

    // Test move-assignment.
    _ = cat::print_line("Move moo into foo.");
    foo = cat::move(moo);
    Result(!moo.has_ownership()).or_panic();

    _ = cat::print_line("Move foo into func().");
    // `cat::move()` is required:
    func(cat::move(foo));
    Result(!foo.has_ownership()).or_panic();

    // This is correctly ill-formed:
    // func(foo);

    _ = cat::print_line("Everything falls out of scope.");

    // Default construct `Unique<Foo>`.
    UniqueWeak<Foo> goo;
    Result(goo.has_ownership()).or_panic();
    // Extract goo.
    _ = goo.borrow();
    Result(!goo.has_ownership()).or_panic();

    // `raii()` should have been called exactly twice.
    Result(global == 2).or_panic();

    // Deduction guides should work.
    UniqueWeak weak = 1;
    Unique unique = weak.borrow();

    // Borrowing `weak`'s data makes it lose ownership.
    Result(!weak.has_ownership()).or_panic();
    weak = 2;
    Result(weak.has_ownership()).or_panic();

    // Permanately transferring ownership a `Unique's storage is unsafe, but
    // possible:
    weak = unique.borrow();
    Result(weak.has_ownership()).or_panic();

    // Unique can be assigned over, which will call its old data's destructor.
    unique = 2;

    cat::exit();
}
