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
    Optional<int4> foo(none);
    Result(!foo.has_value()).or_panic();

    // Assign a value.
    foo = 1;
    Result(foo.has_value()).or_panic();

    // Remove a value.
    foo = none;
    Result(!foo.has_value()).or_panic();

    // Unwrap a value.
    Optional<int4> moo = 1;
    moo = 2;
    Result(moo.value() == 2).or_panic();

    moo = none;
    Result(moo.value_or(100) == 100).or_panic();

    // Optional reference.
    Optional<int4&> ref(none);
    Optional<int4&> ref_2 = none;

    Result(!ref.has_value()).or_panic();
    Result(!ref_2.has_value()).or_panic();

    // Rebind.
    int4 goo = 0;
    Optional<int4&> boo = goo;
    ref = boo;
    boo = none;

    Result(ref.has_value()).or_panic();

    ref = none;
    Result(!ref.has_value()).or_panic();

    // TODO: This isn't working:
    // ref_2 = goo;
    // Result(ref_2.has_value()).or_panic();

    // Optional with a predicate.
    Optional<Predicate<int4,
                       [](int4 input) -> bool {
                           return input >= 0;
                       },
                       -1>>
        positive(none);
    Result(!positive.has_value()).or_panic();

    positive = -10;
    Result(!positive.has_value()).or_panic();

    positive = 0;
    Result(positive.has_value()).or_panic();

    positive = 10;
    Result(positive.has_value()).or_panic();

    positive = none;
    Result(!positive.has_value()).or_panic();

    // This crashes clangd, but it passes tests, last I checked.
    /*
    Optional<Sentinel<int4, 0>> nonzero = none;
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

    moo = none;
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

    positive = none;
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
    // TODO: Test `Optional<> const`.
    // TODO: Test constructing from another `Optional<>`.

    Movable mov;
    Optional<Movable> maybe_movs(cat::move(mov));

    // Non-trivial constructor and destructor.
    Optional<NonTrivial> nontrivial = NonTrivial();
    Result(global_int == 3).or_panic();
    Result(nontrivial.value().data == 2).or_panic();

    cat::exit();
};
