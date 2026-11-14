// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/map>`, `<cat/raii_map>`,
// `<cat/set>`, and `<cat/raii_set>`.
// Any `expected-error` checks must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit. Run
// with `just test safety`.

#include <cat/allocator_page>
#include <cat/map>
#include <cat/raii_map>
#include <cat/raii_set>
#include <cat/set>
#include <cat/string>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// `cat::hash` does not hash strings directly.
struct str_hash {
   static constexpr auto
   operator()(cat::str_view string) -> uint8 {
      return cat::hash(cat::as_bytes(string));
   }
};

inline cat::map<int4, cat::str_view> g_map;
inline cat::raii::map<int4, cat::str_view> g_raii_map{
   cat::allocator_dyn(g_page)
};
inline cat::set<cat::str_view, str_hash> g_set;
inline cat::raii::set<cat::str_view, str_hash> g_raii_set{
   cat::allocator_dyn(g_page)
};

// Positive cases.

auto
valid_map_value_copy() -> cat::maybe<int4> {
   cat::map<int4, int4> local;
   return local[1];
}

auto
valid_map_borrow(cat::map<int4, int4>& caller_map [[clang::lifetimebound]])
   -> cat::maybe<int4&> {
   return caller_map[1];
}

// String literals have static storage, so capturing them cannot dangle.
void
valid_capture_static_string() {
   cat::str_view const string = "static";
   (void)g_map.insert(cat::allocator_dyn(g_page), 1, string);
   (void)g_map.update(cat::allocator_dyn(g_page), 1, string);
   (void)g_raii_map.insert(1, string);
   (void)g_raii_map.update(1, string);
   (void)g_set.insert(cat::allocator_dyn(g_page), string);
   (void)g_raii_set.insert(string);
}

// Negative cases: values outlive their map.

auto
dangling_map_subscript() -> cat::maybe<int4&> {
   cat::map<int4, int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[1];
}

auto
dangling_map_value() -> int4& {
   cat::map<int4, int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[1].value();
}

void
use_after_scope_map() {
   int4* p_escaped = nullptr;
   {
      cat::map<int4, int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local[1].value();
   }
   *p_escaped = 2;
}

auto
dangling_raii_map_subscript() -> cat::maybe<int4&> {
   cat::raii::map<int4, int4> local{cat::allocator_dyn(g_page)};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[1];
}

auto
dangling_raii_map_value() -> int4& {
   cat::raii::map<int4, int4> local{cat::allocator_dyn(g_page)};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[1].value();
}

void
use_after_scope_raii_map() {
   int4* p_escaped = nullptr;
   {
      cat::raii::map<int4, int4> local{cat::allocator_dyn(g_page)};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local[1].value();
   }
   *p_escaped = 2;
}

// clang-format off
// TODO: Enable these when map and set keys and values can be annotated with
// `[[clang::lifetime_capture_by_this]]`. They are taken by `const&` and `&&`,
// like `boost::unordered_flat_map` and `std::flat_map`, but annotating a
// reference parameter captures the referenced object itself, so a local
// `int4` key would dangle too. Clang only infers this attribute for `std::`
// containers, where it annotates parameters that reference a `gsl::Pointer`
// type, and libCat cannot detect that. Rename each
// `disabled-expected-warning` to `expected-warning` when enabling them, since
// `-verify` reads directives inside comments.

// Negative cases: a map or set captures a view of a temporary.

// void
// dangling_capture_map_insert() {
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'g_map'}}
//    (void)g_map.insert(cat::allocator_dyn(g_page), 1, cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_map_update() {
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'g_map'}}
//    (void)g_map.update(cat::allocator_dyn(g_page), 1, cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_raii_map_insert() {
//    // disabled-expected-warning@+2 {{object whose reference is captured by 'g_raii_map'}}
//    // disabled-expected-warning@+1 {{escapes to the global variable 'g_raii_map'}}
//    (void)g_raii_map.insert(1, cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_raii_map_update() {
//    // disabled-expected-warning@+2 {{object whose reference is captured by 'g_raii_map'}}
//    // disabled-expected-warning@+1 {{escapes to the global variable 'g_raii_map'}}
//    (void)g_raii_map.update(1, cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_set_insert() {
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'g_set'}}
//    (void)g_set.insert(cat::allocator_dyn(g_page), cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_raii_set_insert() {
//    // disabled-expected-warning@+2 {{object whose reference is captured by 'g_raii_set'}}
//    // disabled-expected-warning@+1 {{escapes to the global variable 'g_raii_set'}}
//    (void)g_raii_set.insert(cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_map_ref() {
//    cat::map_ref<int4, cat::str_view> ref(g_map);
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'ref'}}
//    (void)ref.insert(cat::allocator_dyn(g_page), 1, cat::str_inplace<8>("a"));
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'ref'}}
//    (void)ref.update(cat::allocator_dyn(g_page), 1, cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_dyn_map() {
//    cat::dyn_map<int4, cat::str_view> ref(g_map);
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'ref'}}
//    (void)ref.insert(cat::allocator_dyn(g_page), 1, cat::str_inplace<8>("a"));
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'ref'}}
//    (void)ref.update(cat::allocator_dyn(g_page), 1, cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_set_ref() {
//    cat::set_ref<cat::str_view, str_hash> ref(g_set);
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'ref'}}
//    (void)ref.insert(cat::allocator_dyn(g_page), cat::str_inplace<8>("a"));
// }

// void
// dangling_capture_dyn_set() {
//    cat::dyn_set<cat::str_view> ref(g_set);
//    // disabled-expected-warning@+1 {{object whose reference is captured by 'ref'}}
//    (void)ref.insert(cat::allocator_dyn(g_page), cat::str_inplace<8>("a"));
// }

// Negative cases: a map or set captures a view of a local.

// void
// use_after_scope_capture_raii_map() {
//    cat::raii::map<int4, cat::str_view> map{cat::allocator_dyn(g_page)};
//    {
//       cat::str_inplace<8> local("a");
//       // disabled-expected-warning@+1 {{local variable 'local' does not live long}}
//       (void)map.insert(1, local);
//    }
//    (void)map[1];
// }

// void
// use_after_scope_capture_raii_set() {
//    cat::raii::set<cat::str_view, str_hash> set{cat::allocator_dyn(g_page)};
//    {
//       cat::str_inplace<8> local("a");
//       // disabled-expected-warning@+1 {{local variable 'local' does not live long}}
//       (void)set.insert(local);
//    }
//    (void)set.insert("b");
// }

// void
// dangling_capture_local_raii_map() {
//    cat::str_inplace<8> local("a");
//    // disabled-expected-warning@+1 {{escapes to the global variable 'g_raii_map'}}
//    (void)g_raii_map.insert(1, local);
// }

// void
// dangling_capture_local_raii_set() {
//    cat::str_inplace<8> local("a");
//    // disabled-expected-warning@+1 {{escapes to the global variable 'g_raii_set'}}
//    (void)g_raii_set.insert(local);
// }

// auto
// dangling_capture_returned_raii_map() -> cat::raii::map<int4, cat::str_view> {
//    cat::raii::map<int4, cat::str_view> map{cat::allocator_dyn(g_page)};
//    cat::str_inplace<8> local("a");
//    // disabled-expected-warning@+1 {{local variable 'local' is returned}}
//    (void)map.insert(1, local);
//    return map;
// }

// TODO: Enable these when Clang tracks a capture through a derived-to-base
// conversion of `this`. `cat::map` and `cat::set` inherit `insert()` and
// `update()` from `map_base` and `set_base`, and `raii::set` inherits
// `contains()`, so Clang drops these captures and uses.

// void
// use_after_scope_capture_map_insert() {
//    cat::map<int4, cat::str_view> map;
//    {
//       cat::str_inplace<8> local("a");
//       // disabled-expected-warning@+1 {{'local' does not live long}}
//       (void)map.insert(cat::allocator_dyn(g_page), 1, local);
//    }
//    (void)map[1];
// }

// void
// use_after_scope_capture_map_update() {
//    cat::map<int4, cat::str_view> map;
//    {
//       cat::str_inplace<8> local("a");
//       // disabled-expected-warning@+1 {{'local' does not live long}}
//       (void)map.update(cat::allocator_dyn(g_page), 1, local);
//    }
//    (void)map[1];
// }

// void
// use_after_scope_capture_set() {
//    cat::set<cat::str_view, str_hash> set;
//    {
//       cat::str_inplace<8> local("a");
//       // disabled-expected-warning@+1 {{'local' does not live long}}
//       (void)set.insert(cat::allocator_dyn(g_page), local);
//    }
//    (void)set.contains("a");
// }

// void
// use_after_scope_capture_raii_set_contains() {
//    cat::raii::set<cat::str_view, str_hash> set{cat::allocator_dyn(g_page)};
//    {
//       cat::str_inplace<8> local("a");
//       // disabled-expected-warning@+1 {{'local' does not live long}}
//       (void)set.insert(local);
//    }
//    (void)set.contains("a");
// }

// void
// dangling_capture_local_map() {
//    cat::str_inplace<8> local("a");
//    // disabled-expected-warning@+1 {{escapes to the global variable 'g_map'}}
//    (void)g_map.insert(cat::allocator_dyn(g_page), 1, local);
// }

// auto
// dangling_capture_returned_map() -> cat::map<int4, cat::str_view> {
//    cat::map<int4, cat::str_view> map;
//    cat::str_inplace<8> local("a");
//    // disabled-expected-warning@+1 {{local variable 'local' is returned}}
//    (void)map.insert(cat::allocator_dyn(g_page), 1, local);
//    return map;
// }

// void
// dangling_capture_local_set() {
//    cat::str_inplace<8> local("a");
//    // disabled-expected-warning@+1 {{escapes to the global variable 'g_set'}}
//    (void)g_set.insert(cat::allocator_dyn(g_page), local);
// }

// TODO: Enable this when Clang tracks a capture by a `[[gsl::Pointer]]` into
// the object that it references.

// void
// use_after_scope_capture_map_ref() {
//    cat::map_ref<int4, cat::str_view> ref(g_map);
//    {
//       cat::str_inplace<8> local("a");
//       // disabled-expected-warning@+1 {{'local' does not live long}}
//       (void)ref.insert(cat::allocator_dyn(g_page), 1, local);
//    }
//    (void)ref[1];
// }
// clang-format on
