#include <cat/memory>
#include <cat/optional>
#include <cat/unique>
#include <cat/utility>

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
        ++global_int;
    }
    NonTrivial(NonTrivial const& in) : data(in.data + 2) {
        ++global_int;
    }
    // This gets called once, making `data` == 2.
    NonTrivial(NonTrivial&& in) : data(in.data + 1) {
        ++global_int;
    }
    ~NonTrivial() {
        this->data = 0;
        ++global_int;
    }
    NonTrivial(int, int, char) {
        this->data = 2;
    }
};

struct ConstNonTrivial {
    int4 const data;
    constexpr ConstNonTrivial() : data(1) {
    }
    constexpr ConstNonTrivial(ConstNonTrivial const& in) : data(in.data + 2) {
    }
    constexpr ConstNonTrivial(ConstNonTrivial&& in) : data(in.data + 1) {
    }
};

auto main() -> int {
    // Initialize empty.
    cat::Optional<int4> foo(nullopt);
    Result(!foo.has_value()).or_exit();

    cat::Optional<int4> inplace_1{};
    Result(!inplace_1.has_value()).or_exit();

    // `int4` default-initializes to 0.
    cat::Optional<int4> inplace_2{in_place};
    Result(inplace_2.value() == 0).or_exit();

    // Assign a value.
    foo = 1;
    Result(foo.has_value()).or_exit();

    // Remove a value.
    foo = nullopt;
    Result(!foo.has_value()).or_exit();

    // Unwrap a value.
    cat::Optional<int4> moo = 1;
    moo = 2;
    Result(moo.value() == 2).or_exit();

    moo = nullopt;
    Result(moo.value_or(100) == 100).or_exit();

    // `Optional` reference.
    cat::Optional<int4&> ref(nullopt);
    cat::Optional<int4&> ref_2 = nullopt;

    Result(!ref.has_value()).or_exit();
    Result(!ref_2.has_value()).or_exit();

    // Rebind.
    int4 goo = 1;
    cat::Optional<int4&> boo = goo;
    ref = boo;
    boo = nullopt;

    // Because `boo` was rebinded when assigned `nullopt`, `ref` should still
    // hold a value.
    Result(ref.has_value()).or_exit();

    Result(ref.value() == 1).or_exit();
    goo = 2;
    Result(ref.value() == 2).or_exit();

    int4 goo_2 = 3;
    // `ref` is rebinded to `goo_2`, instead of `3` assigning through into
    // `goo`.
    ref = goo_2;
    Result(goo == 2).or_exit();
    goo = 0;
    Result(ref.value() == 3).or_exit();

    ref = nullopt;
    Result(!ref.has_value()).or_exit();

    ref_2 = goo;
    Result(ref_2.has_value()).or_exit();
    Result(ref_2.value() == goo).or_exit();

    // `Optional` with a predicate.
    cat::Optional<cat::Compact<int4,
                               [](int4 input) -> bool {
                                   return input >= 0;
                               },
                               -1>>
        positive(nullopt);
    Result(!positive.has_value()).or_exit();

    positive = -10;
    Result(!positive.has_value()).or_exit();

    positive = 0;
    Result(positive.has_value()).or_exit();
    _ = positive.or_exit();

    positive = 10;
    Result(positive.has_value()).or_exit();

    positive = nullopt;
    Result(!positive.has_value()).or_exit();

    // `Optional<void>` with a predicate.
    cat::Optional<cat::Compact<cat::MonostateStorage<int, 0>,
                               [](int input) -> bool {
                                   return input >= 0;
                               },
                               -1>>
        predicate_void(nullopt);
    Result(!predicate_void.has_value()).or_exit();
    predicate_void = monostate;
    Result(predicate_void.has_value()).or_exit();
    _ = predicate_void.or_exit();

    // Test the sentinel predicate.
    cat::Optional<cat::Sentinel<int4, 0>> nonzero = nullopt;
    Result(!nonzero.has_value()).or_exit();

    nonzero = 1;
    Result(nonzero.has_value()).or_exit();

    nonzero = 0;
    Result(!nonzero.has_value()).or_exit();

    // Test `OptionalPtr`.
    int4 get_addr = 0;
    cat::OptionalPtr<int4> opt_ptr = &get_addr;
    Result(opt_ptr.has_value()).or_exit();
    Result(opt_ptr.value() == &get_addr).or_exit();
    Result(*opt_ptr.value() == 0).or_exit();
    Result(opt_ptr.p_value() == &get_addr).or_exit();

    opt_ptr = nullopt;
    Result(!opt_ptr.has_value()).or_exit();
    opt_ptr = nullptr;
    Result(!opt_ptr.has_value()).or_exit();

    // Converting assignments. `foo` is `int4`.
    foo = int{1};
    foo = short{2};

    // Monadic methods.
    moo = 2;

    // Type converting transform.
    Result(moo.transform([](int4 input) -> uint8 {
                  return static_cast<uint8>(input * 2);
              })
               .value() == 4u)
        .or_exit();

    moo.or_else([] {
        cat::exit(1);
    });

    moo = nullopt;
    Result(!moo.transform([](int4 input) {
                   return input * 2;
               })
                .has_value())
        .or_exit();

    _ = moo.and_then([](int4 input) -> cat::Optional<int4> {
        cat::exit(1);
        return input;
    });

    Result(!moo.transform([](int4 input) {
                   return input * 2;
               })
                .and_then([](int4 input) -> cat::Optional<int4> {
                    return input;
                })
                .has_value())
        .or_exit();

    positive = nullopt;
    Result(!positive
                .transform([](int4 input) -> int4 {
                    return input * 2;
                })
                .has_value())
        .or_exit();

    Result(!positive
                .transform([](int4 input) {
                    return input * 2;
                })
                .and_then([](int4 input) -> cat::Optional<int4> {
                    return input;
                })
                .has_value())
        .or_exit();

    decltype(positive) default_predicate_1{};
    Result(!default_predicate_1.has_value()).or_exit();

    // Test function calls.
    auto return_int = [](int4 input) -> int4 {
        return input + 1;
    };
    auto return_none = [](int4) -> cat::Optional<int4> {
        return nullopt;
    };
    auto return_opt = [](int4 input) -> cat::Optional<int4> {
        return input;
    };
    auto return_void = [](int4) -> void {
    };
    auto return_opt_void = [](int4) -> cat::Optional<void> {
        return monostate;
    };
    auto nothing = []() -> void {
    };
    auto maybe_nothing = []() -> cat::Optional<void> {
        return nullopt;
    };

    foo.transform(return_int).and_then(return_opt_void).or_else(nothing);

    _ = foo.transform(return_int)
            .and_then(return_opt_void)
            .or_else(maybe_nothing);

    cat::Optional<int4> monadic_int;
    monadic_int = return_none(0).and_then(return_opt);
    Result(!monadic_int.has_value()).or_exit();

    monadic_int = return_opt(1).transform(return_int);
    Result(monadic_int.has_value()).or_exit();
    Result(monadic_int.value() == 2).or_exit();

    cat::Optional<void> monadic_void =
        return_opt(1).transform(return_int).transform(return_void);
    Result(monadic_void.has_value()).or_exit();

    // Test monadic methods on reference types.
    int4 monadic_int_ref = 1;
    cat::Optional<void> monadic_void_ref =
        cat::Optional{monadic_int_ref}.and_then(return_opt_void);
    // Be sure that this did not assign through.
    Result(monadic_void_ref.has_value()).or_exit();

    // The default value of `int4` is `0`.
    decltype(positive) default_predicate_2{in_place};
    Result(default_predicate_2.value() == 0).or_exit();

    // Test monadic methods on move-only types.
    cat::Optional<cat::Unique<int4>> monadic_move = 1;
    monadic_move = return_none(0).and_then(return_opt);
    Result(!monadic_move.has_value()).or_exit();

    monadic_move = return_opt(1).transform(return_int);
    Result(monadic_move.has_value()).or_exit();
    Result(monadic_move.value().borrow() == 2).or_exit();

    // Test copying `Optional`s into other `Optional`s.
    cat::Optional<int4> opt_original = 10;
    cat::Optional<int4> opt_copy_1 = cat::Optional{opt_original};
    cat::Optional<int4> opt_copy_2 = opt_original;
    Result(opt_copy_1.value() = 10).or_exit();
    Result(opt_copy_2.value() = 10).or_exit();

    // Getting pointers.
    foo = 1;
    int4 const& ref_foo = foo.value();
    Result(&ref_foo == &foo.value()).or_exit();
    Result(foo.p_value() == &foo.value()).or_exit();
    Result(foo.p_value() == addressof(foo.value())).or_exit();

    // TODO: Test non-trivial reference.

    // `Optional const`
    cat::Optional<int4> const constant_val = 1;
    [[maybe_unused]] cat::Optional<int4> const constant_null = nullopt;
    [[maybe_unused]] auto con = constant_val.value();

    // Test constant references.
    int4 nonconstant_int = 10;
    int4 const constant_int = 9;
    cat::Optional<int4 const&> constant_ref = constant_int;
    Result(constant_ref.value() == 9).or_exit();
    constant_ref = nonconstant_int;
    Result(constant_ref.value() == 10).or_exit();

    // Test move-only types.
    Movable mov;
    cat::Optional<Movable> maybe_movs(cat::move(mov));

    // Non-trivial constructor and destructor.
    cat::Optional<NonTrivial> nontrivial = NonTrivial();
    Result(nontrivial.value().data == 2).or_exit();
    // Result(global_int == 3).or_exit();
    // Result(nontrivial.value().data == 3).or_exit();

    // `Optional<void>` default-initializes empty:
    cat::Optional<void> optvoid;
    Result(!optvoid.has_value()).or_exit();
    // `monostate` initializes a value:
    cat::Optional<void> optvoid_2{monostate};
    Result(optvoid_2.has_value()).or_exit();

    // `in_place` initializes a value:
    cat::Optional<void> optvoid_4{in_place};
    Result(optvoid_4.has_value()).or_exit();
    // `nullopt` initializes empty:
    cat::Optional<void> optvoid_5{nullopt};
    Result(!optvoid_5.has_value()).or_exit();

    cat::Optional<NonTrivial> in_place_nontrivial_1{in_place};
    Result(in_place_nontrivial_1.has_value()).or_exit();
    Result(in_place_nontrivial_1.value().data == 1).or_exit();

    cat::Optional<NonTrivial> in_place_nontrivial_2{in_place, 1, 2, 'a'};
    Result(in_place_nontrivial_2.has_value()).or_exit();
    Result(in_place_nontrivial_2.value().data == 2).or_exit();

    // Test `Optional` in a `constexpr` context.
    auto constant = []() constexpr {
        [[maybe_unused]] constexpr cat::Optional<int> const_int_default;

        constexpr cat::Optional<ConstNonTrivial> const_nontrivial_default;
        Result(!const_nontrivial_default.has_value()).or_exit();

        constexpr cat::Optional<ConstNonTrivial> const_nontrivial =
            ConstNonTrivial{};
        Result(const_nontrivial.has_value()).or_exit();

        constexpr cat::Optional<ConstNonTrivial> const_nontrivial_in_place = {
            in_place, ConstNonTrivial{}};
        Result(const_nontrivial_in_place.has_value()).or_exit();

        // Test `Optional<Compact<T>>`.
        constexpr cat::OptionalPtr<void> const_optptr = nullptr;
        cat::OptionalPtr<void> optptr = nullptr;
        optptr = nullptr;
        optptr = const_optptr;
        [[maybe_unused]] cat::OptionalPtr<void> optptr2 = optptr;
        [[maybe_unused]] cat::OptionalPtr<void> optptr3 = const_optptr;
        [[maybe_unused]] cat::OptionalPtr<void> optptr4;

        [[maybe_unused]] constexpr cat::OptionalPtr<NonTrivial>
            const_nontrivial_optptr = nullptr;
        [[maybe_unused]] constexpr cat::OptionalPtr<NonTrivial>
            const_nontrivial_default_optptr;
        [[maybe_unused]] cat::OptionalPtr<NonTrivial> nontrivial_optptr =
            nullptr;
        [[maybe_unused]] cat::OptionalPtr<NonTrivial> nontrivial_default_optptr;
    };
    _ = cat::constant_evaluate(constant);

    // Assign value:
    optvoid = monostate;
    Result(optvoid.has_value()).or_exit();
    // Remove value:
    optvoid = nullopt;
    Result(!optvoid.has_value()).or_exit();
}
