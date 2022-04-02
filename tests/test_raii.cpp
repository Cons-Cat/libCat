#include <raii>
#include <string>
#include <utility>

int4 global = 0;

struct Foo {
    String data;

    Foo() = default;

    Foo(String string) : data(std::move(string)) {
        cat::print(data).discard_result();
        cat::print_line(" constructor").discard_result();
    }

    ~Foo() {
        cat::print("~").discard_result();
        cat::print_line(this->data).discard_result();
    }

    auto operator=(String string) {
        this->data = std::move(string);
        return *this;
    }

    void raii() const {
        cat::print(this->data).discard_result();
        cat::print_line(" calls raii()!").discard_result();
        global++;
    }
};

void func(Raii<Foo>){};

void meow() {
    cat::print_line("Construct objects.").discard_result();
    // Test constructor.
    Raii<Foo> foo(String("foo"));
    // Test assignment.
    foo = "foo";
    Result(foo.has_ownership()).or_panic();

    Raii<Foo> moo(String("moo"));
    Result(moo.has_ownership()).or_panic();

    // Test move-assignment.
    cat::print_line("Move moo into foo.").discard_result();
    foo = cat::move(moo);
    Result(!moo.has_ownership()).or_panic();

    cat::print_line("Move foo into func().").discard_result();
    // `cat::move()` is required:
    func(cat::move(foo));
    Result(!foo.has_ownership()).or_panic();

    // This is correctly ill-formed:
    // func(foo);

    cat::print_line("Everything falls out of scope.").discard_result();

    // Default construct `Raii<Foo>`.
    Raii<Foo> goo;
    // Extract goo.
    _ = goo.get();
    Result(!goo.has_ownership()).or_panic();

    // `raii()` should have been called exactly twice.
    Result(global == 2).or_panic();
    cat::exit();
}
