#include <cat/array>
#include <cat/demangle>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

namespace {

struct demangle_case {
   cat::str_view mangled;
   cat::str_view expected;
};

void
verify_cases(cat::span<demangle_case const> cases) {
   for (demangle_case const& current : cases) {
      cat::maybe<cat::demangled_name> const result =
         cat::demangle(pager, current.mangled);
      cat::verify(result.has_value());
      cat::verify(result.value().view() == current.expected);
   }
}

}  // namespace

$test(demangle_names_and_types) {
   constexpr cat::array cases{
      demangle_case{.mangled = "main", .expected = "main"},
      demangle_case{.mangled = "_start", .expected = "_start"},
      demangle_case{
                    .mangled = "_Z11print_traceRN3cat14page_allocatorE",
                    .expected = "print_trace(cat::page_allocator&)",
                    },
      demangle_case{
                    .mangled = "_ZN3foo3barERKN3cat14page_allocatorE",
                    .expected = "foo::bar(cat::page_allocator const&)",
                    },
      demangle_case{
                    .mangled = "_Z3fooPKViOif",
                    .expected = "foo(int volatile const*, int&&, float)",
                    },
      demangle_case{
                    .mangled = "_Z3fooijlmcahstfdbPv",
                    .expected =
            "foo(int, unsigned int, long, unsigned long, char, signed char, "
            "unsigned char, short, unsigned short, float, double, bool, void*)", },
   };
   verify_cases(cases);
}

$test(demangle_substitutions_and_templates) {
   constexpr cat::array cases{
      demangle_case{
                    .mangled = "_ZN3foo3BoxIiE3setERKS1_",
                    .expected = "foo::Box<int>::set(foo::Box<int> const&)",
                    },
      demangle_case{
                    .mangled = "_ZN3foo3barEN3cat4spanIiEE",
                    .expected = "foo::bar(cat::span<int>)",
                    },
      demangle_case{
                    .mangled = "_ZN3foo3bazIN3cat4spanIiEEEEvT_",
                    .expected = "void foo::baz<cat::span<int> >(cat::span<int>)",
                    },
      demangle_case{
                    .mangled = "_Z3fooIiEvT_", .expected = "void foo<int>(int)"
      },
      demangle_case{.mangled = "_Z3fooILi42EEvv", .expected = "void foo<42>()"},
   };
   verify_cases(cases);
}

$test(demangle_special_names) {
   constexpr cat::array cases{
      demangle_case{
                    .mangled = "_ZN3foo3BoxIiEC1Ev",
                    .expected = "foo::Box<int>::Box()",
                    },
      demangle_case{
                    .mangled = "_ZN3foo3BoxIiED1Ev",
                    .expected = "foo::Box<int>::~Box()",
                    },
      demangle_case{.mangled = "_Zplii", .expected = "operator+(int, int)"},
      demangle_case{
                    .mangled = "_ZNK3foo3BoxIiE3getEv",
                    .expected = "foo::Box<int>::get() const",
                    },
      demangle_case{
                    .mangled = "_ZlsRN3foo6streamEPKc",
                    .expected = "operator<<(foo::stream&, char const*)",
                    },
   };
   verify_cases(cases);
}

$test(demangle_malformed_and_unsupported) {
   constexpr cat::array cases{
      demangle_case{.mangled = "_Z", .expected = "_Z"},
      demangle_case{.mangled = "_Z3fo", .expected = "_Z3fo"},
      demangle_case{.mangled = "_Z3fooX", .expected = "_Z3fooX"},
      demangle_case{.mangled = "_Z3fooS9_", .expected = "_Z3fooS9_"},
      demangle_case{
                    .mangled = "_Z3fooILd3ff0000000000000EEvv",
                    .expected = "_Z3fooILd3ff0000000000000EEvv",
                    },
   };
   verify_cases(cases);
}
