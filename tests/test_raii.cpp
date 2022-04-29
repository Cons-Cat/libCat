#include <string>
#include <unique>
#include <utility>

int4 global = 0;

struct Foo {
    cat::String data;

    Foo() = default;

    Foo(cat::String string) : data(std::move(string)) {
        _ = cat::print(data);
        _ = cat::print_line(" constructor");
    }

    ~Foo() {
        _ = cat::print("~");
        _ = cat::print_line(this->data);
    }

    auto operator=(cat::String string) {
        this->data = std::move(string);
        return *this;
    }

    void raii() const {
        _ = cat::print(this->data);
        _ = cat::print_line(" calls raii()!");
        global++;
    }
};

void func(cat::UniqueWeak<Foo>){};

void meow() {
    _ = cat::print_line("Construct objects.");
    // Test constructor.
    cat::UniqueWeak<Foo> foo(cat::String("foo"));
    // Test assignment.
    foo = "foo";
    Result(foo.has_ownership()).or_panic();

    cat::UniqueWeak<Foo> moo(cat::String("moo"));
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

    // Default construct `	cat::Unique<Foo>`.
    cat::UniqueWeak<Foo> goo;
    Result(goo.has_ownership()).or_panic();
    // Extract goo.
    _ = goo.borrow();
    Result(!goo.has_ownership()).or_panic();

    // `raii()` should have been called exactly twice.
    Result(global == 2).or_panic();

    // Deduction guides should work.
    cat::UniqueWeak weak = 1;
    cat::Unique unique = weak.borrow();

    // Borrowing `weak`'s data makes it lose ownership.
    Result(!weak.has_ownership()).or_panic();
    weak = 2;
    Result(weak.has_ownership()).or_panic();

    // Permanately transferring ownership a `	cat::Unique's storage is unsafe,
    // but possible:
    weak = unique.borrow();
    Result(weak.has_ownership()).or_panic();

    // 	cat::Unique can be assigned over, which will call its old data's
    // destructor.
    unique = 2;

    cat::Unique<int> original = 0;
    cat::Unique<int8> into = cat::move(original);

    cat::exit();
}
