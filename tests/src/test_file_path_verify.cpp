// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/file_path>` and
// `<cat/raii_file_path>`. Any `expected-error` checks must come last, because
// Clang stops its `-Wlifetime-safety` pass after the first error in a
// translation unit. Run with `just test safety`.

#include <cat/allocator_linear>
#include <cat/allocator_page>
#include <cat/file_path>
#include <cat/raii_file_path>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Negative cases: characters and iterators outlive their path.

auto
dangling_file_path_begin() -> auto {
   cat::file_path local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_file_path_data() -> auto {
   cat::file_path local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_file_path_subscript() -> auto& {
   cat::file_path local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_file_path_front() -> auto& {
   cat::file_path local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_raii_file_path_begin() -> auto {
   cat::raii::file_path<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_file_path_data() -> auto {
   cat::raii::file_path<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_file_path_subscript() -> auto& {
   cat::raii::file_path<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_raii_file_path_front() -> auto& {
   cat::raii::file_path<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

// Negative cases: a `raii` file path from a factory or `.clone()`
// outlives its allocator.

auto
dangling_raii_file_path_allocator_empty(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_file_path<cat::allocator_linear>(linear);
}

auto
dangling_raii_file_path_allocator_segment(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_file_path<cat::allocator_linear>(
      linear, cat::str_view("a")
   );
}

auto
dangling_raii_file_path_allocator_dyn(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_file_path(
      cat::allocator_dyn(linear), cat::str_view("a")
   );
}

using linear_raii_file_path =
   __typeof_unqual(cat::raii::make_file_path<cat::allocator_linear>(
                      cat::declval<cat::allocator_linear&>()
   )
                      .value());

auto
dangling_raii_file_path_allocator_clone(
   cat::span<cat::byte> arena, linear_raii_file_path const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_ref<cat::allocator_linear>(linear));
}

auto
dangling_raii_file_path_allocator_clone_dyn(
   cat::span<cat::byte> arena, linear_raii_file_path const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_dyn(linear));
}
