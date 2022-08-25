#include <cat/scaredy>

// Minimal result types usable for `cat::Scaredy`.
struct ErrorOne {
    int4 code;

    constexpr auto error() const -> int8 {
        return this->code;
    }
};

struct ErrorTwo {
    int4 code;

    constexpr auto error() const -> int8 {
        return this->code;
    }
};

auto one() -> ErrorOne {
    ErrorOne one{1};
    return one;
}

auto two() -> ErrorTwo {
    ErrorTwo two{2};
    return two;
}

auto union_errors(int4 error) -> cat::Scaredy<int8, ErrorOne, ErrorTwo> {
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

auto main() -> int {
    cat::Scaredy result = union_errors(0);
    // The `Scaredy` here adds a flag to the `int8`, which is padded out to 16
    // bytes. No storage cost exists for the error types.
    static_assert(sizeof(result) == 16);

    Result(!result.has_value()).or_exit();
    Result(result.holds_alternative<ErrorOne>()).or_exit();
    Result(!result.holds_alternative<int8>()).or_exit();

    result = union_errors(1);
    Result(!result.has_value()).or_exit();
    Result(result.holds_alternative<ErrorTwo>()).or_exit();
    Result(!result.holds_alternative<int8>()).or_exit();

    result = union_errors(2);
    Result(result.has_value()).or_exit();
    Result(result.holds_alternative<int8>()).or_exit();

    result = union_errors(3);
    Result(result.has_value()).or_exit();
    Result(result.value() == 3).or_exit();
    Result(result.holds_alternative<int8>()).or_exit();

    cat::Scaredy<cat::Compact<int4,
                              [](int4 input) {
                                  return input >= 0;
                              },
                              -1>,
                 ErrorOne>
        predicate = -1;
    // The `Scaredy` here adds no storage bloat to an `int4`.
    static_assert(sizeof(predicate) == sizeof(int4));

    Result(!predicate.has_value()).or_exit();
    predicate = 0;
    Result(predicate.has_value()).or_exit();
    predicate = 10;
    Result(predicate.has_value()).or_exit();

    // Test `.value_or()`.
    cat::Scaredy<int4, ErrorOne> is_error = ErrorOne{};
    cat::Scaredy<int4, ErrorOne> is_value = 2;
    cat::Scaredy<int4, ErrorOne> const const_is_error = ErrorOne{};
    cat::Scaredy<int4, ErrorOne> const const_is_value = 2;

    int4 fallback = is_error.value_or(1);
    Result(fallback == 1).or_exit();

    int4 no_fallback = is_value.value_or(1);
    Result(no_fallback == 2).or_exit();

    int4 const_fallback = const_is_error.value_or(1);
    Result(const_fallback == 1).or_exit();

    int4 no_const_fallback = const_is_value.value_or(1);
    Result(no_const_fallback == 2).or_exit();

    // Test monadic member functions on a mutable `Scaredy`.
    auto increment = [](auto input) {
        return input + 1;
    };

    cat::Scaredy<int4, ErrorOne> mut_scaredy = 1;
    _ = mut_scaredy.transform(increment).and_then(increment).or_exit();

    // `.transform()` returning `void`.
    mut_scaredy.transform(increment).or_else([]() {
        return;
    });

    _ = mut_scaredy.transform(increment)
            .or_else([]() {
                return decltype(mut_scaredy){};
            })
            .or_exit();

    // Test monadic member functions on a `const`-qualified `Scaredy`.
    cat::Scaredy<int4, ErrorOne> const const_scaredy = 1;
    _ = const_scaredy.transform(increment).and_then(increment).or_exit();
}
