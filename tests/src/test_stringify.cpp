#include <cat/enum>

#include "../unit_tests.hpp"

namespace foo {
template <auto>
struct stringify_me {};
}

test(stringify) {
   static_assert(cat::nameof(1) == "int");
   static_assert(cat::nameof(1ll) == "long long");
   static_assert(cat::nameof(foo::stringify_me<1>()) == "foo::stringify_me<1>");
}
