#include <cat/scaredy>

#include "../unit_tests.hpp"

// Minimal result types usable for `cat::scaredy`.
struct error_type_one {
    int4 code;

    [[nodiscard]]
    constexpr auto error() const -> int8 {
        return this->code;
    }
};

struct error_type_two {
    int4 code;

    [[nodiscard]]
    constexpr auto error() const -> int8 {
        return this->code;
    }
};

auto one() -> error_type_one {
    error_type_one one{1};
    return one;
}

auto two() -> error_type_two {
    error_type_two two{2};
    return two;
}

auto union_errors(int4 error)
    -> cat::scaredy<int8, error_type_one, error_type_two> {
    switch (error.raw) {
        case 0:
            return one();
        case 1:
            return two();
        case 2:
            return int8{3};
        case 3:
            return 3;
        default:
            __builtin_unreachable();
    }
}

enum class error_set {
    one,
    two
};

auto scaredy_try_success() -> cat::scaredy<int, error_set> {
    cat::scaredy<int, error_set> error{0};
    int boo = TRY(error);
    return boo;
}

auto scaredy_try_fail() -> cat::scaredy<int, error_set> {
    cat::scaredy<int, error_set> error{error_set::one};
    int boo = TRY(error);
    return boo;
}

TEST(test_scaredy) {
    cat::scaredy result = union_errors(0);
    // The `scaredy` here adds a flag to the `int8`, which is padded out to 16
    // bytes. No storage cost exists for the error types.
    static_assert(sizeof(result) == 16);

    cat::verify(!result.has_value());
    cat::verify(result.is<error_type_one>());
    cat::verify(!result.is<int8>());

    result = union_errors(1);
    cat::verify(!result.has_value());
    cat::verify(result.is<error_type_two>());
    cat::verify(!result.is<int8>());

    result = union_errors(2);
    cat::verify(result.has_value());
    cat::verify(result.is<int8>());

    result = union_errors(3);
    cat::verify(result.has_value());
    cat::verify(result.value() == 3);
    cat::verify(result.is<int8>());

    // Test `.error()`.
    cat::scaredy<int, error_type_one> one_error = error_type_one(1);
    cat::verify(one_error.error().code == 1);
    cat::verify(one_error.error<error_type_one>().code == 1);

    cat::scaredy<int, error_type_one, error_type_two> two_error =
        error_type_one(1);
    cat::verify(two_error.error<error_type_one>().code == 1);

    // Test compact optimization.
    cat::scaredy<cat::compact_scaredy<int4,
                                      [](int4 input) {
                                          return input >= 0;
                                      }>,
                 error_type_one>
        predicate = -1;

    // This `scaredy` adds no storage to an `int4`.
    static_assert(sizeof(predicate) == sizeof(int4));
    cat::verify(!predicate.has_value());

    predicate = -1;
    cat::verify(!predicate.has_value());

    predicate = 0;
    cat::verify(predicate.has_value());

    predicate = 10;
    cat::verify(predicate.has_value());

    predicate = error_type_one(-1);
    cat::verify(!predicate.has_value());

    // Test `.value_or()`.
    cat::scaredy<int4, error_type_one> is_error = error_type_one();
    cat::scaredy<int4, error_type_one> is_value = 2;
    cat::scaredy<int4, error_type_one> const const_is_error = error_type_one();
    cat::scaredy<int4, error_type_one> const const_is_value = 2;

    int4 fallback = is_error.value_or(1);
    cat::verify(fallback == 1);

    int4 no_fallback = is_value.value_or(1);
    cat::verify(no_fallback == 2);

    int4 const_fallback = const_is_error.value_or(1);
    cat::verify(const_fallback == 1);

    int4 no_const_fallback = const_is_value.value_or(1);
    cat::verify(no_const_fallback == 2);

    // Test monadic member functions on a mutable `scaredy`.
    auto increment = [](auto input) {
        return input + 1;
    };

    cat::scaredy<int4, error_type_one> mut_scaredy = 1;
    _ = mut_scaredy.transform(increment).and_then(increment);

    // `.transform()` returning `void`.
    mut_scaredy.transform(increment).or_else([]() {
        return;
    });

    _ = mut_scaredy.transform(increment).or_else([]() {
        return decltype(mut_scaredy){};
    });

    // Test monadic member functions on a `const`-qualified `scaredy`.
    cat::scaredy<int4, error_type_one> const const_scaredy = 1;
    _ = const_scaredy.transform(increment).and_then(increment);

    // Test `.is()` on variant `scaredy`.
    bool matched = false;

    cat::scaredy<int4, error_type_one, error_type_two> is_variant_scaredy;
    is_variant_scaredy = 1;

    // Match it against `int4`.
    cat::match(is_variant_scaredy)(  //
        is_a<int4>().then([&]() {
            matched = true;
        }));
    cat::match(is_variant_scaredy)(  //
        is_a<error_type_one>().then([&]() {
            matched = false;
        }));
    cat::match(is_variant_scaredy)(  //
        is_a<error_type_two>().then([&]() {
            matched = false;
        }));
    // `float` can never hold true here, but it should compile.
    cat::match(is_variant_scaredy)(  //
        is_a<float>().then([&]() {
            matched = false;
        }));
    cat::verify(matched);

    // Match it against `error_type_one`.
    matched = false;
    is_variant_scaredy = error_type_one();
    cat::match(is_variant_scaredy)(  //
        is_a<error_type_one>().then([&]() {
            matched = true;
        }));
    cat::match(is_variant_scaredy)(  //
        is_a<int4>().then([&]() {
            matched = false;
        }));
    cat::match(is_variant_scaredy)(  //
        is_a<error_type_two>().then([&]() {
            matched = false;
        }));
    cat::verify(matched);

    // Test member access pattern matching syntax.
    matched = false;
    is_variant_scaredy.match(is_a<error_type_one>().then([&]() {
        matched = true;
    }));
    cat::verify(matched);

    // Test `.is()` on `compact` `scaredy`.
    predicate = 1;

    // Test type comparison.
    matched = false;
    cat::match(predicate)(  //
        is_a<error_type_one>().then([&]() {
            cat::exit(1);
        }),
        is_a<int4>().then([&]() {
            matched = true;
        }));
    cat::verify(matched);

    matched = false;
    predicate = error_type_one(-1);
    cat::match(predicate)(  //
        is_a<int4>().then([&]() {
            cat::exit(1);
        }),
        is_a<error_type_one>().then([&]() {
            matched = true;
        }));
    cat::verify(matched);

    // Test traits.
    static_assert(!cat::is_maybe<cat::scaredy<int, error_type_one>>);
    static_assert(!cat::is_maybe<decltype(result)>);

    static_assert(cat::is_scaredy<cat::scaredy<int, error_type_one>>);
    static_assert(cat::is_scaredy<decltype(result)>);

    // Test `TRY` macro.
    _ = scaredy_try_success().verify();
    cat::scaredy fail = scaredy_try_fail();
    cat::verify(!fail.has_value());
}
