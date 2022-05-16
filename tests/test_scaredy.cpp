#include <scaredy>

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
    switch (error.c()) {
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

void meow() {
    cat::Scaredy result = union_errors(0);
    // The `Scaredy` here adds a flag to the `int8`, which is padded out to 16
    // bytes. No storage cost exists for the error types.
    static_assert(sizeof(result) == 16);

    Result(!result.has_value()).or_panic();
    Result(result.holds_alternative<ErrorOne>()).or_panic();
    Result(!result.holds_alternative<int8>()).or_panic();

    result = union_errors(1);
    Result(!result.has_value()).or_panic();
    Result(result.holds_alternative<ErrorTwo>()).or_panic();
    Result(!result.holds_alternative<int8>()).or_panic();

    result = union_errors(2);
    Result(result.has_value()).or_panic();
    Result(result.holds_alternative<int8>()).or_panic();

    result = union_errors(3);
    Result(result.has_value()).or_panic();
    Result(result.value() == 3).or_panic();
    Result(result.holds_alternative<int8>()).or_panic();

    cat::Scaredy<cat::Predicate<int4,
                                [](int4 input) {
                                    return input >= 0;
                                },
                                -1>,
                 ErrorOne>
        predicate = -1;
    // The `Scaredy` here adds no storage bloat to an `int4`.
    static_assert(sizeof(predicate) == sizeof(int4));

    Result(!predicate.has_value()).or_panic();
    predicate = 0;
    Result(predicate.has_value()).or_panic();
    predicate = 10;
    Result(predicate.has_value()).or_panic();

    cat::exit();
}
