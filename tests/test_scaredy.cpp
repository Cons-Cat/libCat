#include <scaredy>

// Minimal result types usable for `cat::Scaredy`.
struct ResultOne {
    int8 code;

    constexpr auto error() const -> int8 {
        return this->code;
    }
};

struct ResultTwo {
    int8 code;

    constexpr auto error() const -> int8 {
        return this->code;
    }
};

auto one() -> ResultOne {
    ResultOne one{1};
    return one;
}

auto two() -> ResultTwo {
    ResultTwo two{2};
    return two;
}

auto union_errors(int4 error) -> cat::Scaredy<int8, ResultOne, ResultTwo> {
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
    Result(!result.has_value()).or_panic();
    Result(result.holds_alternative<ResultOne>()).or_panic();
    Result(!result.holds_alternative<int8>()).or_panic();

    result = union_errors(1);
    Result(!result.has_value()).or_panic();
    Result(result.holds_alternative<ResultTwo>()).or_panic();
    Result(!result.holds_alternative<int8>()).or_panic();

    result = union_errors(2);
    Result(result.has_value()).or_panic();
    Result(result.value() == 3).or_panic();
    Result(result.holds_alternative<int8>()).or_panic();

    result = union_errors(3);
    Result(result.has_value()).or_panic();
    Result(result.value() == 3).or_panic();
    Result(result.holds_alternative<int8>()).or_panic();

    cat::exit();
}
