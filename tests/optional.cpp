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

    Optional<int4> inplace_1{};
    Result(!inplace_1.has_value()).or_panic();

    // `int4` default-initializes to `0`.
    Optional<int4> inplace_2{in_place};
    Result(inplace_2.value() == 0).or_panic();

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
    int4 goo = 1;
    Optional<int4&> boo = goo;
    ref = boo;
    boo = nullopt;

    // Because `boo` was rebinded when assigned `nullopt`, `ref` should still
    // hold a value.
    Result(ref.has_value()).or_panic();

    Result(ref.value() == 1).or_panic();
    goo = 2;
    Result(ref.value() == 2).or_panic();

    int4 goo_2 = 3;
    // `ref` is rebinded to `goo_2`, instead of `3` assigning through into
    // `goo`.
    ref = goo_2;
    Result(goo == 2).or_panic();
    goo = 0;
    Result(ref.value() == 3).or_panic();

    ref = nullopt;
    Result(!ref.has_value()).or_panic();

    ref_2 = goo;
    Result(ref_2.has_value()).or_panic();
    Result(ref_2.value() == goo).or_panic();

    // Optional with a predicate.
    Optional<Predicate<int,
                       [](int input) -> bool {
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

    // Converting assignment.
    foo = 1;
    foo = short{2};

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
                .transform([](int input) {
                    return input * 2;
                })
                .has_value())
        .or_panic();

    Result(!positive
                .transform([](int input) {
                    return input * 2;
                })
                .and_then([](int input) {
                    return input;
                })
                .has_value())
        .or_panic();

    decltype(positive) default_predicate_1{};
    Result(!default_predicate_1.has_value()).or_panic();

    // Test function calls.
    auto return_int = [](auto input) -> int4 {
        return input + 1;
    };
    auto return_none = [](auto) -> Optional<int4> {
        return nullopt;
    };
    auto return_opt = [](auto input) -> Optional<int4> {
        return input;
    };
    auto return_void = [](auto) -> void {
    };

    Optional<int4> monadic;
    monadic = return_none(0).and_then(return_opt);
    Result(!monadic.has_value()).or_panic();

    monadic = return_opt(1).and_then(return_int);
    Result(monadic.has_value()).or_panic();
    Result(monadic.value() == 2).or_panic();

    Optional<void> monadic_void =
        return_opt(1).and_then(return_int).and_then(return_void);
    Result(monadic_void.has_value()).or_panic();

    // The default value of `int4` is `0`.
    decltype(positive) default_predicate_2{in_place};
    Result(default_predicate_2.value() == 0).or_panic();

    // TODO: Test monadic methods on reference types.
    // TODO: Test monadic methods on move-only types.
    // TODO: Test constructing from another `Optional<>`.
    // TODO: Test non-trivial reference.

    // Getting pointers.
    foo = 1;
    int4 const& ref_foo = foo.value();
    Result(&ref_foo == &foo.value()).or_panic();
    Result(foo.p_value() == &foo.value()).or_panic();
    Result(foo.p_value() == addressof(foo.value())).or_panic();

    // `Optional const`
    Optional<int4> const constant_val = 1;
    [[maybe_unused]] Optional<int4> const constant_null = nullopt;
    [[maybe_unused]] auto con = constant_val.value();

    // TODO: This does not work:
    // int4 const constant_int = 0;
    // Optional<int4 const&> const constant_ref = constant_int;

    Movable mov;
    Optional<Movable> maybe_movs(cat::move(mov));

    // Non-trivial constructor and destructor.
    Optional<NonTrivial> nontrivial = NonTrivial();
    Result(global_int == 3).or_panic();
    Result(nontrivial.value().data == 2).or_panic();

    // `Optional<void>` default-initializes empty:
    Optional<void> optvoid;
    Result(!optvoid.has_value()).or_panic();
    // `monostate` initializes a value:
    Optional<void> optvoid_2{monostate};
    Result(optvoid_2.has_value()).or_panic();

    // `in_place` initializes a value:
    Optional<void> optvoid_4{in_place};
    Result(optvoid_4.has_value()).or_panic();
    // `nullopt` initializes empty:
    Optional<void> optvoid_5{nullopt};
    Result(!optvoid_5.has_value()).or_panic();

    // Assign value:
    optvoid = monostate;
    Result(optvoid.has_value()).or_panic();
    // Remove value:
    optvoid = nullopt;
    Result(!optvoid.has_value()).or_panic();

    cat::exit();
}
