#include <memory>
#include <optional>
#include <utility>

struct Movable {
    Movable() = default;
    Movable(Movable&&) = default;
    auto operator=(Movable&&){};
};

int4 global_int = 0;
struct NonTrivial {
    int4 data;
    NonTrivial() {
        this->data = 1;
        global_int++;
    }
    // This should never get called. It would make `data` == 3.
    NonTrivial(NonTrivial const& in) : data(in.data + 2) {
        global_int++;
    }
    // This gets called once, making `data` == 2.
    NonTrivial(NonTrivial&& in) : data(in.data + 1) {
        global_int++;
    }
    ~NonTrivial() {
        this->data = 0;
        global_int++;
    }
};

void meow() {
    // Initialize empty.
    Optional<int4> foo(nullopt);
    Result(!foo.has_value()).or_panic();

    // Assign a value.
    foo = 1;
    Result(foo.has_value()).or_panic();

    // Remove a value.
    foo = nullopt;
    Result(!foo.has_value()).or_panic();

    // Unwrap a value.
    Optional<int4> moo = 1;
    moo = 2;
    Result(moo.value() == 2).or_panic();

    moo = nullopt;
    Result(moo.value_or(100) == 100).or_panic();

    // Optional reference.
    Optional<int4&> ref(nullopt);
    Optional<int4&> ref_2 = nullopt;

    Result(!ref.has_value()).or_panic();
    Result(!ref_2.has_value()).or_panic();

    // Rebind.
    int4 goo = 0;
    Optional<int4&> boo = goo;
    ref = boo;
    boo = nullopt;

    // Because `boo` was rebinded when assigned `nullopt`, `ref` should still
    // hold a value.
    Result(ref.has_value()).or_panic();

    ref = nullopt;
    Result(!ref.has_value()).or_panic();

    ref_2 = goo;
    Result(ref_2.has_value()).or_panic();
    Result(ref_2.value() == goo).or_panic();

    // Optional with a predicate.
    Optional<Predicate<int4,
                       [](int4 input) -> bool {
                           return input >= 0;
                       },
                       -1>>
        positive(nullopt);
    Result(!positive.has_value()).or_panic();

    positive = -10;
    Result(!positive.has_value()).or_panic();

    positive = 0;
    Result(positive.has_value()).or_panic();

    positive = 10;
    Result(positive.has_value()).or_panic();

    positive = nullopt;
    Result(!positive.has_value()).or_panic();

    // This crashes clangd, but it passes tests, last I checked.
    /*
    Optional<Sentinel<int4, 0>> nonzero = nullopt;
    Result(!nonzero.has_value()).or_panic();

    nonzero = 1;
    Result(nonzero.has_value()).or_panic();

    nonzero = 0;
    Result(!nonzero.has_value()).or_panic();
    */

    // Monadic methods.
    moo = 2;
    Result(moo.transform([](auto input) {
                  return input * 2;
              })
               .value() == 4)
        .or_panic();

    _ = moo.or_else([]() {
        cat::exit(1);
    });

    moo = nullopt;
    Result(!moo.transform([](int4 input) {
                   return input * 2;
               })
                .has_value())
        .or_panic();

    _ = moo.and_then([](Optional<int4> input) -> Optional<int4> {
        cat::exit(1);
        return input;
    });

    Result(!moo.transform([](int4 input) {
                   return input * 2;
               })
                .and_then([](int4 input) {
                    return input;
                })
                .has_value())
        .or_panic();

    positive = nullopt;
    Result(!positive
                .transform([](int4 input) {
                    return input * 2;
                })
                .has_value())
        .or_panic();

    Result(!positive
                .transform([](int4 input) {
                    return input * 2;
                })
                .and_then([](int4 input) {
                    return input;
                })
                .has_value())
        .or_panic();

    // TODO: Test monadic methods on reference types.
    // TODO: Test monadic methods on move-only types.
    // TODO: Test constructing from another `Optional<>`.

    // `Optional const`
    Optional<int4> const constant_val = 1;
    Optional<int4> const constant_null = nullopt;
    auto con = constant_val.value();

    // TODO: This does not work:
    // int4 const constant_int = 0;
    // Optional<int4 const&> const constant_ref = constant_int;

    Movable mov;
    Optional<Movable> maybe_movs(cat::move(mov));

    // Non-trivial constructor and destructor.
    Optional<NonTrivial> nontrivial = NonTrivial();
    Result(global_int == 3).or_panic();
    Result(nontrivial.value().data == 2).or_panic();

    cat::exit();
}
