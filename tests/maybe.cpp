#include <maybe>

// TODO: Use `Result<>` asserts.
void meow() {
    // Initialize empty.
    Maybe<int4> foo;
    if (foo.has_value()) {
        std::exit(1);
    }

    // Assign a value.
    foo = 1;
    if (!foo.has_value()) {
        std::exit(1);
    }

    // Remove a value.
    foo = none;
    if (foo.has_value()) {
        std::exit(1);
    }

    // Unwrap a value.
    Maybe<int4> moo = 1;
    moo = 2;
    if (moo.unwrap() != 2) {
        std::exit(1);
    }

    // Maybe reference.
    int4 myint = 0;
    Maybe<int4&> goo;
    Maybe<int4&> goo_2 = none;
    Maybe<int4&> boo = myint;
    goo = boo;

    if (!goo.has_value()) {
        std::exit(1);
    }

    goo = none;

    if (goo.has_value()) {
        std::exit(1);
    }
    if (goo_2.has_value()) {
        std::exit(1);
    }

    // Maybe with predicate.
    Maybe<Predicate<int4,
                    [](int4 input) -> bool {
                        return input >= 0;
                    },
                    -1>>
        positive;
    if (positive.has_value()) {
        std::exit(1);
    }

    positive = -10;
    if (positive.has_value()) {
        std::exit(1);
    }

    positive = 0;
    if (!positive.has_value()) {
        std::exit(1);
    }

    positive = 10;
    if (!positive.has_value()) {
        std::exit(1);
    }

    positive = none;
    if (positive.has_value()) {
        std::exit(1);
    }

    std::exit();
};
