// Clang `-verify` checks of the `[[clang::lifetimebound]]` annotations on the
// `<cat/iterable>` API, exercised through `cat::vec` and `cat::list`. These
// cover every view adaptor in both its pipe and member forms, `cat::iterate()`,
// `cat::reverse_iterate()`, and the borrowing members of `iterable_interface`.
// Any `expected-error` checks must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit. Run
// with `just test safety`.

#include <cat/iterable>
#include <cat/list>
#include <cat/vec>

using namespace cat::arithmetic;

inline constexpr auto is_big = [](int4 value) -> bool {
   return value > 1;
};

inline constexpr auto twice = [](int4 value) -> int4 {
   return value * 2;
};

template <typename T>
void
consume(T& value);

// Positive cases: terminals consume a borrowed view before its container dies,
// owning views outlive the container they were moved from, and contexts of
// borrowing views only borrow the container.

auto
valid_vec_terminal() -> int4 {
   cat::vec<int4> local;
   return cat::ref(local) | cat::reverse() | cat::sum();
}

auto
valid_vec_member_terminal() -> idx {
   cat::vec<int4> local;
   return cat::ref(local).filter(is_big).count();
}

auto
valid_vec_owned_view() -> auto {
   cat::vec<int4> local;
   return cat::move(local) | cat::reverse();
}

auto
valid_vec_owned_nested_view() -> auto {
   cat::vec<int4> local;
   return cat::move(local) | cat::reverse() | cat::take(1u);
}

auto
valid_vec_owned_member_view() -> auto {
   cat::vec<int4> local;
   return cat::move(local).reverse().drop(1u);
}

auto
valid_vec_borrowed_parameter(cat::vec<int4>& caller [[clang::lifetimebound]])
   -> auto {
   return cat::ref(caller) | cat::reverse() | cat::take(1u);
}

auto
valid_vec_borrowed_parameter_context(
   cat::vec<int4>& caller [[clang::lifetimebound]]
) -> auto {
   return (cat::ref(caller) | cat::reverse()).iterate();
}

auto
valid_vec_iterate_parameter(cat::vec<int4>& caller [[clang::lifetimebound]])
   -> auto {
   return cat::iterate(cat::ref(caller));
}

void
valid_vec_borrowed_view() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::reverse();
   consume(view);
}

void
valid_vec_nested_context() {
   cat::vec<int4> local;
   auto context = (cat::ref(local) | cat::reverse() | cat::take(2u)).iterate();
   consume(context);
}

void
valid_vec_nested_member_context() {
   cat::vec<int4> local;
   auto context = cat::ref(local).reverse().as_rvalue().iterate();
   consume(context);
}

void
valid_vec_iterate_temporary_view() {
   cat::vec<int4> local;
   auto context = cat::iterate(cat::ref(local) | cat::reverse());
   consume(context);
}

void
valid_vec_reverse_iterate_temporary_view() {
   cat::vec<int4> local;
   auto context = cat::reverse_iterate(cat::ref(local) | cat::as_rvalue());
   consume(context);
}

void
valid_vec_owned_view_context() {
   auto view = cat::vec<int4>{} | cat::reverse();
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_view_of_owned_view() {
   auto view = cat::vec<int4>{} | cat::reverse();
   auto taken = cat::ref(view) | cat::take(1u);
   auto context = taken.iterate();
   consume(context);
}

void
valid_vec_const_borrow() {
   cat::vec<int4> const local;
   auto context = (cat::cref(local) | cat::reverse()).iterate();
   consume(context);
}

void
valid_vec_reverse_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::reverse();
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_as_rvalue_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::as_rvalue();
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_take_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::take(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_drop_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::drop(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_take_last_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::take_last(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_drop_last_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::drop_last(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_take_while_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::take_while(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_drop_while_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::drop_while(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_take_last_while_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::take_last_while(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_drop_last_while_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::drop_last_while(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_filter_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::filter(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_vec_transform_context() {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::transform(twice);
   auto context = view.iterate();
   consume(context);
}

auto
valid_list_terminal() -> int4 {
   cat::list<int4> local;
   return cat::ref(local) | cat::reverse() | cat::sum();
}

auto
valid_list_member_terminal() -> idx {
   cat::list<int4> local;
   return cat::ref(local).filter(is_big).count();
}

auto
valid_list_owned_view() -> auto {
   cat::list<int4> local;
   return cat::move(local) | cat::reverse();
}

auto
valid_list_owned_nested_view() -> auto {
   cat::list<int4> local;
   return cat::move(local) | cat::reverse() | cat::take(1u);
}

auto
valid_list_owned_member_view() -> auto {
   cat::list<int4> local;
   return cat::move(local).reverse().drop(1u);
}

auto
valid_list_borrowed_parameter(cat::list<int4>& caller [[clang::lifetimebound]])
   -> auto {
   return cat::ref(caller) | cat::reverse() | cat::take(1u);
}

auto
valid_list_borrowed_parameter_context(
   cat::list<int4>& caller [[clang::lifetimebound]]
) -> auto {
   return (cat::ref(caller) | cat::reverse()).iterate();
}

auto
valid_list_iterate_parameter(cat::list<int4>& caller [[clang::lifetimebound]])
   -> auto {
   return cat::iterate(cat::ref(caller));
}

void
valid_list_borrowed_view() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::reverse();
   consume(view);
}

void
valid_list_nested_context() {
   cat::list<int4> local;
   auto context = (cat::ref(local) | cat::reverse() | cat::take(2u)).iterate();
   consume(context);
}

void
valid_list_nested_member_context() {
   cat::list<int4> local;
   auto context = cat::ref(local).reverse().as_rvalue().iterate();
   consume(context);
}

void
valid_list_iterate_temporary_view() {
   cat::list<int4> local;
   auto context = cat::iterate(cat::ref(local) | cat::reverse());
   consume(context);
}

void
valid_list_reverse_iterate_temporary_view() {
   cat::list<int4> local;
   auto context = cat::reverse_iterate(cat::ref(local) | cat::as_rvalue());
   consume(context);
}

void
valid_list_owned_view_context() {
   auto view = cat::list<int4>{} | cat::reverse();
   auto context = view.iterate();
   consume(context);
}

void
valid_list_view_of_owned_view() {
   auto view = cat::list<int4>{} | cat::reverse();
   auto taken = cat::ref(view) | cat::take(1u);
   auto context = taken.iterate();
   consume(context);
}

void
valid_list_const_borrow() {
   cat::list<int4> const local;
   auto context = (cat::cref(local) | cat::reverse()).iterate();
   consume(context);
}

void
valid_list_reverse_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::reverse();
   auto context = view.iterate();
   consume(context);
}

void
valid_list_as_rvalue_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::as_rvalue();
   auto context = view.iterate();
   consume(context);
}

void
valid_list_take_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::take(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_list_drop_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::drop(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_list_take_last_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::take_last(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_list_drop_last_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::drop_last(1u);
   auto context = view.iterate();
   consume(context);
}

void
valid_list_take_while_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::take_while(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_list_drop_while_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::drop_while(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_list_filter_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::filter(is_big);
   auto context = view.iterate();
   consume(context);
}

void
valid_list_transform_context() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::transform(twice);
   auto context = view.iterate();
   consume(context);
}

// Negative cases: a borrowed view, or its context, outlives its container.

auto
dangling_vec_reverse() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::reverse();
}

auto
dangling_vec_member_reverse() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).reverse();
}

auto
dangling_vec_reverse_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::reverse();
   return view.iterate();
}

auto
dangling_vec_as_rvalue() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::as_rvalue();
}

auto
dangling_vec_member_as_rvalue() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).as_rvalue();
}

auto
dangling_vec_as_rvalue_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::as_rvalue();
   return view.iterate();
}

auto
dangling_vec_take() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::take(1u);
}

auto
dangling_vec_member_take() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).take(1u);
}

auto
dangling_vec_take_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::take(1u);
   return view.iterate();
}

auto
dangling_vec_drop() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::drop(1u);
}

auto
dangling_vec_member_drop() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).drop(1u);
}

auto
dangling_vec_drop_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::drop(1u);
   return view.iterate();
}

auto
dangling_vec_take_last() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::take_last(1u);
}

auto
dangling_vec_member_take_last() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).take_last(1u);
}

auto
dangling_vec_take_last_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::take_last(1u);
   return view.iterate();
}

auto
dangling_vec_drop_last() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::drop_last(1u);
}

auto
dangling_vec_member_drop_last() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).drop_last(1u);
}

auto
dangling_vec_drop_last_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::drop_last(1u);
   return view.iterate();
}

auto
dangling_vec_take_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::take_while(is_big);
}

auto
dangling_vec_member_take_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).take_while(is_big);
}

auto
dangling_vec_take_while_context() -> auto {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::take_while(is_big);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_vec_drop_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::drop_while(is_big);
}

auto
dangling_vec_member_drop_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).drop_while(is_big);
}

auto
dangling_vec_drop_while_context() -> auto {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::drop_while(is_big);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_vec_take_last_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::take_last_while(is_big);
}

auto
dangling_vec_member_take_last_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).take_last_while(is_big);
}

auto
dangling_vec_take_last_while_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::take_last_while(is_big);
   return view.iterate();
}

auto
dangling_vec_drop_last_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::drop_last_while(is_big);
}

auto
dangling_vec_member_drop_last_while() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).drop_last_while(is_big);
}

auto
dangling_vec_drop_last_while_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::drop_last_while(is_big);
   return view.iterate();
}

auto
dangling_vec_filter() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::filter(is_big);
}

auto
dangling_vec_member_filter() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).filter(is_big);
}

auto
dangling_vec_filter_context() -> auto {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::filter(is_big);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_vec_transform() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::transform(twice);
}

auto
dangling_vec_member_transform() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).transform(twice);
}

auto
dangling_vec_transform_context() -> auto {
   cat::vec<int4> local;
   auto view = cat::ref(local) | cat::transform(twice);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_vec_nested() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::reverse() | cat::take(1u) | cat::as_rvalue();
}

auto
dangling_vec_member_nested() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).reverse().take(1u).as_rvalue();
}

auto
dangling_vec_borrowed_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return (cat::ref(local) | cat::reverse()).iterate();
}

auto
dangling_vec_nested_context() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return (cat::ref(local) | cat::reverse() | cat::take(1u)).iterate();
}

auto
dangling_vec_iterate() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::iterate(local);
}

auto
dangling_vec_reverse_iterate() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::reverse_iterate(local);
}

auto
dangling_vec_iterate_ref() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::iterate(cat::ref(local));
}

auto
dangling_vec_iterate_view() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::iterate(cat::ref(local) | cat::reverse());
}

auto
dangling_vec_view_of_owned_view() -> auto {
   auto view = cat::vec<int4>{} | cat::reverse();
   // expected-warning@+1 1+ {{local variable 'view'}}
   return cat::ref(view) | cat::take(1u);
}

auto
dangling_vec_member_view_of_owned_view() -> auto {
   auto view = cat::vec<int4>{} | cat::reverse();
   // expected-warning@+1 1+ {{local variable 'view'}}
   return cat::ref(view).take(1u);
}

void
dangling_vec_view_after_scope() {
   cat::vec<int4> outer;
   auto view = cat::ref(outer) | cat::reverse();
   {
      cat::vec<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      view = cat::ref(local) | cat::reverse();
   }
   consume(view);
}

void
dangling_vec_owned_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::reverse()).iterate();
   consume(context);
}

void
dangling_vec_owned_member_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = cat::vec<int4>{}.reverse().iterate();
   consume(context);
}

void
dangling_vec_callback_context() {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::ref(local) | cat::filter(is_big)).iterate();
   consume(context);
}

void
dangling_vec_owned_reverse_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::reverse()).iterate();
   consume(context);
}

void
dangling_vec_owned_as_rvalue_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::as_rvalue()).iterate();
   consume(context);
}

void
dangling_vec_owned_take_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::take(1u)).iterate();
   consume(context);
}

void
dangling_vec_owned_drop_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::drop(1u)).iterate();
   consume(context);
}

void
dangling_vec_owned_take_last_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::take_last(1u)).iterate();
   consume(context);
}

void
dangling_vec_owned_drop_last_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::drop_last(1u)).iterate();
   consume(context);
}

void
dangling_vec_owned_take_while_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::take_while(is_big)).iterate();
   consume(context);
}

void
dangling_vec_owned_drop_while_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::drop_while(is_big)).iterate();
   consume(context);
}

void
dangling_vec_owned_take_last_while_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::take_last_while(is_big)).iterate();
   consume(context);
}

void
dangling_vec_owned_drop_last_while_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::drop_last_while(is_big)).iterate();
   consume(context);
}

void
dangling_vec_owned_filter_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::filter(is_big)).iterate();
   consume(context);
}

void
dangling_vec_owned_transform_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::vec<int4>{} | cat::transform(twice)).iterate();
   consume(context);
}

auto
dangling_list_reverse() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::reverse();
}

auto
dangling_list_member_reverse() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).reverse();
}

auto
dangling_list_reverse_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::reverse();
   return view.iterate();
}

auto
dangling_list_as_rvalue() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::as_rvalue();
}

auto
dangling_list_member_as_rvalue() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).as_rvalue();
}

auto
dangling_list_as_rvalue_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::as_rvalue();
   return view.iterate();
}

auto
dangling_list_take() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::take(1u);
}

auto
dangling_list_member_take() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).take(1u);
}

auto
dangling_list_take_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::take(1u);
   return view.iterate();
}

auto
dangling_list_drop() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::drop(1u);
}

auto
dangling_list_member_drop() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).drop(1u);
}

auto
dangling_list_drop_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::drop(1u);
   return view.iterate();
}

auto
dangling_list_take_last() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::take_last(1u);
}

auto
dangling_list_member_take_last() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).take_last(1u);
}

auto
dangling_list_take_last_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::take_last(1u);
   return view.iterate();
}

auto
dangling_list_drop_last() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::drop_last(1u);
}

auto
dangling_list_member_drop_last() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).drop_last(1u);
}

auto
dangling_list_drop_last_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   auto view = cat::ref(local) | cat::drop_last(1u);
   return view.iterate();
}

auto
dangling_list_take_while() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::take_while(is_big);
}

auto
dangling_list_member_take_while() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).take_while(is_big);
}

auto
dangling_list_take_while_context() -> auto {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::take_while(is_big);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_list_drop_while() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::drop_while(is_big);
}

auto
dangling_list_member_drop_while() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).drop_while(is_big);
}

auto
dangling_list_drop_while_context() -> auto {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::drop_while(is_big);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_list_filter() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::filter(is_big);
}

auto
dangling_list_member_filter() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).filter(is_big);
}

auto
dangling_list_filter_context() -> auto {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::filter(is_big);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_list_transform() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::transform(twice);
}

auto
dangling_list_member_transform() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).transform(twice);
}

auto
dangling_list_transform_context() -> auto {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::transform(twice);
   // expected-warning@+1 1+ {{local variable 'view'}}
   return view.iterate();
}

auto
dangling_list_nested() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local) | cat::reverse() | cat::take(1u) | cat::as_rvalue();
}

auto
dangling_list_member_nested() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::ref(local).reverse().take(1u).as_rvalue();
}

auto
dangling_list_borrowed_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return (cat::ref(local) | cat::reverse()).iterate();
}

auto
dangling_list_nested_context() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return (cat::ref(local) | cat::reverse() | cat::take(1u)).iterate();
}

auto
dangling_list_iterate() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::iterate(local);
}

auto
dangling_list_reverse_iterate() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::reverse_iterate(local);
}

auto
dangling_list_iterate_ref() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::iterate(cat::ref(local));
}

auto
dangling_list_iterate_view() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::iterate(cat::ref(local) | cat::reverse());
}

auto
dangling_list_view_of_owned_view() -> auto {
   auto view = cat::list<int4>{} | cat::reverse();
   // expected-warning@+1 1+ {{local variable 'view'}}
   return cat::ref(view) | cat::take(1u);
}

auto
dangling_list_member_view_of_owned_view() -> auto {
   auto view = cat::list<int4>{} | cat::reverse();
   // expected-warning@+1 1+ {{local variable 'view'}}
   return cat::ref(view).take(1u);
}

void
dangling_list_view_after_scope() {
   cat::list<int4> outer;
   auto view = cat::ref(outer) | cat::reverse();
   {
      cat::list<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      view = cat::ref(local) | cat::reverse();
   }
   consume(view);
}

void
dangling_list_owned_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::reverse()).iterate();
   consume(context);
}

void
dangling_list_owned_member_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = cat::list<int4>{}.reverse().iterate();
   consume(context);
}

void
dangling_list_callback_context() {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::ref(local) | cat::filter(is_big)).iterate();
   consume(context);
}

void
dangling_list_owned_reverse_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::reverse()).iterate();
   consume(context);
}

void
dangling_list_owned_as_rvalue_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::as_rvalue()).iterate();
   consume(context);
}

void
dangling_list_owned_take_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::take(1u)).iterate();
   consume(context);
}

void
dangling_list_owned_drop_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::drop(1u)).iterate();
   consume(context);
}

void
dangling_list_owned_take_last_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::take_last(1u)).iterate();
   consume(context);
}

void
dangling_list_owned_drop_last_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::drop_last(1u)).iterate();
   consume(context);
}

void
dangling_list_owned_take_while_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::take_while(is_big)).iterate();
   consume(context);
}

void
dangling_list_owned_drop_while_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::drop_while(is_big)).iterate();
   consume(context);
}

void
dangling_list_owned_filter_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::filter(is_big)).iterate();
   consume(context);
}

void
dangling_list_owned_transform_context() {
   // expected-warning@+1 1+ {{temporary}}
   auto context = (cat::list<int4>{} | cat::transform(twice)).iterate();
   consume(context);
}

// Negative cases: borrowing members of `iterable_interface` outlive their
// container.

auto
dangling_vec_read_at() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.read_at(0u);
}

auto
dangling_vec_slice() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.slice(0u, 1u);
}

auto
dangling_vec_as_span() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.as_span();
}

auto
dangling_vec_fill() -> decltype(auto) {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.fill(1);
}

auto
dangling_vec_pipe_fill() -> decltype(auto) {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local | cat::fill(1);
}

auto
dangling_vec_reverse_inplace() -> decltype(auto) {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.reverse_inplace();
}

auto
dangling_vec_pipe_reverse_inplace() -> decltype(auto) {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local | cat::reverse_inplace();
}

auto
dangling_list_fill() -> decltype(auto) {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.fill(1);
}

// Ill-formed cases.

void
ref_of_rvalue_vec() {
   // expected-error@+2 {{immediately dangle}}
   // expected-error@+1 {{invalid operands}}
   auto view = cat::ref(cat::vec<int4>{}) | cat::reverse();
}

void
ref_of_rvalue_list() {
   // expected-error@+2 {{immediately dangle}}
   // expected-error@+1 {{invalid operands}}
   auto view = cat::ref(cat::list<int4>{}) | cat::take(1u);
}

// A non-`cat::ref()` l-value container is not a borrow-acceptable view base,
// and `cat::list` is not a collection, so it has no `*_last_while` views.
// expected-error@cat/iterable:* 6 {{no matching member function}}

void
pipe_lvalue_vec() {
   cat::vec<int4> local;
   auto view = local | cat::reverse();
}

void
pipe_lvalue_list() {
   cat::list<int4> local;
   auto view = local | cat::take(1u);
}

void
member_view_of_lvalue_vec() {
   cat::vec<int4> local;
   auto view = local.reverse();
}

void
member_view_of_lvalue_list() {
   cat::list<int4> local;
   auto view = local.filter(is_big);
}

void
list_take_last_while() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::take_last_while(is_big);
}

void
list_drop_last_while() {
   cat::list<int4> local;
   auto view = cat::ref(local) | cat::drop_last_while(is_big);
}
