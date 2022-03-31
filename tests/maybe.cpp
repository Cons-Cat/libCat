#include <maybe>
#include <memory>

void meow() {
    // Initialize empty.
    Maybe<int4> foo(none);
    Result(!foo.has_value()).or_panic();

    // Assign a value.
    foo = 1;
    Result(foo.has_value()).or_panic();

    // Remove a value.
    foo = none;
    Result(!foo.has_value()).or_panic();

    // Unwrap a value.
    Maybe<int4> moo = 1;
    moo = 2;
    Result(moo.value() == 2).or_panic();

    moo = none;
    Result(moo.value_or(100) == 100).or_panic();

    // Maybe reference.
    Maybe<int4&> ref(none);
    Maybe<int4&> ref_2 = none;

    Result(!ref.has_value()).or_panic();
    Result(!ref_2.has_value()).or_panic();

    // Rebind.
    int4 goo = 0;
    Maybe<int4&> boo = goo;
    ref = boo;
    boo = none;

    Result(ref.has_value()).or_panic();

    ref = none;

    Result(!ref.has_value()).or_panic();
    Result(ref_2.has_value()).or_panic();

    // Maybe with a predicate.
    Maybe<Predicate<int4,
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
    Maybe<Sentinel<int4, 0>> nonzero = none;
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
    Result(!moo.transform([](auto input) {
                   return input * 2;
               })
                .has_value())
        .or_panic();

    _ = moo.and_then([](Maybe<int4> input) -> Maybe<int4> {
        cat::exit(1);
        return input;
    });

    Result(!moo.transform([](auto input) {
                   return input * 2;
               })
                .and_then([](auto input) {
                    return input;
                })
                .has_value())
        .or_panic();

    positive = none;
    Result(!positive
                .transform([](auto input) {
                    return input * 2;
                })
                .has_value())
        .or_panic();

    Result(!positive
                .transform([](auto input) {
                    return input * 2;
                })
                .and_then([](auto input) {
                    return input;
                })
                .has_value())
        .or_panic();

    // TODO: Test monadic methods on reference types.
    // TODO: Test monadic methods on move-only types.
    // TODO: Test `Maybe<> const`.
    // TODO: Test constructing from another `Maybe<>`.

    cat::exit();
};
