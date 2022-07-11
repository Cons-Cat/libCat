#include <cat/string>
#include <cat/unique>
#include <cat/utility>

int4 global = 0;

struct Foo {
    cat::String data;

    Foo() = default;

    Foo(cat::String string) : data(std::move(string)) {
        _ = cat::print(data);
        _ = cat::println(" constructor");
    }

    ~Foo() {
        _ = cat::print("~");
        _ = cat::println(this->data);
    }

    auto operator=(cat::String string) {
        this->data = std::move(string);
        return *this;
    }

    void raii() const {
        _ = cat::print(this->data);
        _ = cat::println(" calls raii()!");
        ++global;
    }
};

void func(cat::UniqueWeak<Foo>){};

int main() {
    // TODO: Fix `Unique` and re-enable these tests.
    _ = cat::println("Construct objects.");
    // Test constructor.
    cat::UniqueWeak<Foo> foo(cat::String("foo"));
    // Test assignment.
    foo = cat::String("foo");
    Result(foo.has_ownership()).or_exit();

    cat::UniqueWeak<Foo> moo(cat::String("moo"));
    Result(moo.has_ownership()).or_exit();

    // Test move-assignment.
    _ = cat::println("Move moo into foo.");
    foo = cat::move(moo);
    Result(!moo.has_ownership()).or_exit();

    _ = cat::println("Move foo into func().");
    // `cat::move()` is required:
    func(cat::move(foo));
    Result(!foo.has_ownership()).or_exit();

    // This is deliberately ill-formed:
    // func(foo);

    _ = cat::println("Everything falls out of scope.");

    // Default construct `	cat::Unique<Foo>`.
    cat::UniqueWeak<Foo> goo;
    Result(goo.has_ownership()).or_exit();
    // Extract goo.
    _ = goo.borrow();
    Result(!goo.has_ownership()).or_exit();

    // `raii()` should have been called exactly three times.
    Result(global == 3).or_exit();

    // Deduction guides should work.
    cat::UniqueWeak weak = 1;
    cat::Unique unique = weak.borrow();

    // Borrowing `weak`'s data makes it lose ownership.
    Result(!weak.has_ownership()).or_exit();
    weak = 2;
    Result(weak.has_ownership()).or_exit();

    // Permanately transferring ownership a `cat::Unique`'s storage is unsafe,
    // but possible:
    weak = unique.borrow();
    Result(weak.has_ownership()).or_exit();

    // `cat::Unique` can be assigned over, which will call its old data's
    // destructor.
    unique = 2;

    cat::Unique<int> original = 0;
    cat::Unique<int8> into = cat::move(original);

    cat::exit();
}
