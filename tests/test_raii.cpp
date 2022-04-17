#include <raii>
#include <string>
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

void func(Raii<Foo>){};

void meow() {
    _ = cat::print_line("Construct objects.");
    // Test constructor.
    Raii<Foo> foo(String("foo"));
    // Test assignment.
    foo = "foo";
    Result(foo.has_ownership()).or_panic();

    Raii<Foo> moo(String("moo"));
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

    // Default construct `Raii<Foo>`.
    Raii<Foo> goo;
    // Extract goo.
    _ = goo.get();
    Result(!goo.has_ownership()).or_panic();

    // `raii()` should have been called exactly twice.
    Result(global == 2).or_panic();
    cat::exit();
}
