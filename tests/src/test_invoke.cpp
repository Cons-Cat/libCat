#include <cat/functional>

#include "../unit_tests.hpp"

void invoke_fn_void() {
}

auto invoke_fn_int() -> int {
    return 1;
}

TEST(test_invoke) {
    // TODO: This does not compile yet.
    // _ = cat::invoke(&invoke_fn_void);
}
