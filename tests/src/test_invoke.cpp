#include <cat/utility>

#include "../unit_tests.hpp"

void
invoke_fn_void() {
}

auto
invoke_fn_int() -> int {
   return 1;
}

$test(invoke) {
   cat::invoke(&invoke_fn_void);
   cat::verify(cat::invoke(&invoke_fn_int) == 1);
}
