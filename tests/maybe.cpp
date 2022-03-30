#include <maybe>
#include <memory>

// TODO: Use `Result<>` asserts.
void meow() {
    // Initialize empty.
    Maybe<int4> foo(none);
    if (foo.has_value()) {
        cat::exit(1);
    }

    // Assign a value.
    foo = 1;
    if (!foo.has_value()) {
        cat::exit(1);
    }

    // Remove a value.
    foo = none;
    if (foo.has_value()) {
        cat::exit(1);
    }

    // Unwrap a value.
    Maybe<int4> moo = 1;
    moo = 2;
    if (moo.value() != 2) {
        cat::exit(1);
    }

    // Maybe reference.
    int4 goo = 0;
    Maybe<int4&> ref(none);
    Maybe<int4&> ref_2 = none;
    Maybe<int4&> boo = goo;
    ref = boo;

    if (!ref.has_value()) {
        cat::exit(1);
    }

    ref = none;

    if (ref.has_value()) {
        cat::exit(1);
    }
    if (ref_2.has_value()) {
        cat::exit(1);
    }

    // Maybe with a predicate.
    Maybe<Predicate<int4,
                    [](int4 input) -> bool {
                        return input >= 0;
                    },
                    -1>>
        positive(none);
    if (positive.has_value()) {
        cat::exit(1);
    }

    positive = -10;
    if (positive.has_value()) {
        cat::exit(1);
    }

    positive = 0;
    if (!positive.has_value()) {
        cat::exit(1);
    }

    positive = 10;
    if (!positive.has_value()) {
        cat::exit(1);
    }

    positive = none;
    if (positive.has_value()) {
        cat::exit(1);
    }

    // This crashes clangd, but it passes tests, last I checked.
    /*
    Maybe<Sentinel<int4, 0>> nonzero = none;
if (nonzero.has_value()) {
            cat::exit(1);
    }

nonzero = 1;
if (!nonzero.has_value()) {
     cat::exit(1);
 }

nonzero = 0;
if (nonzero.has_value()) {
     cat::exit(1);
 }
    */

    // Monadic methods.
    if (moo.transform([](auto input) {
               return input * 2;
           })
            .value() != 4) {
        cat::exit(1);
    }

    _ = moo.or_else([]() {
        cat::exit(1);
    });

    moo = none;
    if (moo.transform([](auto input) {
               return input * 2;
           })
            .has_value()) {
        cat::exit(1);
    }

    _ = moo.and_then([](Maybe<int4> input) -> Maybe<int4> {
        cat::exit(1);
        return input;
    });

    if (moo.transform([](auto input) {
               return input * 2;
           })
            .and_then([](auto input) {
                return input;
            })
            .has_value()) {
        cat::exit(1);
    }

    positive = none;
    if (positive
            .transform([](auto input) {
                return input * 2;
            })
            .has_value()) {
        cat::exit(1);
    }

    if (positive
            .transform([](auto input) {
                return input * 2;
            })
            .and_then([](auto input) {
                return input;
            })
            .has_value()) {
        cat::exit(1);
    }

    // TODO: Test monadic methods on reference types.
    // TODO: Test monadic methods on move-only types.
    // TODO: Test `Maybe<> const`.

    cat::exit();
};
