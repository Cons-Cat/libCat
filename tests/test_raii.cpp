#include <linux>
#include <raii>
#include <utility>

int4 global = 0;

struct Foo {
    String data;

    Foo(String string) : data(std::move(string)) {
        cat::print(data).discard_result();
        cat::print_line(" constructor").discard_result();
    }

    ~Foo() {
        cat::print("~").discard_result();
        cat::print_line(this->data).discard_result();
    }

    auto operator=(String string) {
        this->data = string;
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

    Raii<Foo> moo(String("moo"));

    // Test move-assignment.
    cat::print_line("Move moo into foo.").discard_result();
    foo = cat::move(moo);

    cat::print_line("Move foo into func().").discard_result();
    // `cat::move()` is required:
    func(cat::move(foo));

    // This is correctly ill-formed:
    // func(foo);

    cat::print_line("Everything falls out of scope.").discard_result();

    // `raii()` should have been called exactly twice.
    Result(global == 2).or_panic();
    cat::exit();
}
