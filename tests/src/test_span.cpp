#include <cat/array>
#include <cat/meta>
#include <cat/span>
#include <cat/utility>

#include "../unit_tests.hpp"

namespace {

// SFINAE probe for whether `std::tuple_size<T>::value` is well-formed.
template <typename, typename = void>
inline constexpr bool has_tuple_size = false;
template <typename T>
inline constexpr bool has_tuple_size<
   T, cat::detail::void_type<decltype(std::tuple_size<T>::value)>> = true;

// SFINAE probe for whether `cat::as_writable_bytes(span)` is callable.
template <typename, typename = void>
inline constexpr bool can_as_writable_bytes = false;
template <typename T>
inline constexpr bool can_as_writable_bytes<
   T, cat::detail::void_type<decltype(cat::as_writable_bytes(
         cat::declval<T>()))>> = true;

// Heterogeneous comparability concepts modeled after the simplified versions in
// `brevzin/span_ext/test/test.cxx`.
// clang-format off
template <typename T, typename U>
concept is_equality_comparable_with = requires(T const& t, U const& u) {
   { t == u } -> cat::is_convertible<bool>;
   { u == t } -> cat::is_convertible<bool>;
   { t != u } -> cat::is_convertible<bool>;
   { u != t } -> cat::is_convertible<bool>;
};

template <typename T, typename U>
concept is_totally_ordered_with =
   is_equality_comparable_with<T, U>
   && requires(T const& t, U const& u) {
         { t < u } -> cat::is_convertible<bool>;
         { t <= u } -> cat::is_convertible<bool>;
         { t > u } -> cat::is_convertible<bool>;
         { t >= u } -> cat::is_convertible<bool>;
         { u < t } -> cat::is_convertible<bool>;
         { u <= t } -> cat::is_convertible<bool>;
         { u > t } -> cat::is_convertible<bool>;
         { u >= t } -> cat::is_convertible<bool>;
      };
// clang-format on

// Element type with `operator==` only. No < or <=>, so spans of these can be
// equality-compared but not ordered.
struct only_equality {
   int4 i;

   constexpr bool
   operator==(only_equality const& other) const {
      return i == other.i;
   }
};

// Inherits the equality-only element type. Distinct from `only_equality`, so
// `span<derived_only_equality>` and `span<only_equality>` should NOT be
// equality-comparable with each other.
struct derived_only_equality : only_equality {};

// No comparison operators. `span<uncomparable>` should be neither
// equality-comparable nor ordered.
struct uncomparable {
   int4 i;
};

// Run a test body across each cat integer width: int1, uint1, int2, uint2,
// int4, uint4. Each instantiation is exercised once per call.
template <typename Body>
constexpr void
for_each_int_width(Body body) {
   body.template operator()<cat::int1>();
   body.template operator()<cat::uint1>();
   body.template operator()<cat::int2>();
   body.template operator()<cat::uint2>();
   body.template operator()<cat::int4>();
   body.template operator()<cat::uint4>();
}

}  // namespace

// Default and null construction yield an empty span with no backing storage.
test(span_default_construction) {
   cat::span<int4> empty_dynamic;
   cat::verify(empty_dynamic.size() == 0u);
   cat::verify(empty_dynamic.is_empty());

   cat::span<int4 const> empty_const;
   cat::verify(empty_const.size() == 0u);
   cat::verify(empty_const.is_empty());
}

test(span_nullptr_construction) {
   cat::span<int4> from_null{nullptr};
   cat::verify(from_null.size() == 0u);
   cat::verify(from_null.data() == nullptr);
   cat::verify(from_null.is_empty());
}

test(span_pointer_length_construction) {
   int4 values[5] = {10, 20, 30, 40, 50};

   cat::span<int4> dynamic_span{values, 5u};
   cat::verify(dynamic_span.size() == 5u);
   cat::verify(dynamic_span.data() == values);

   cat::span<int4, 5_idx> fixed_span{values, 5u};
   cat::verify(fixed_span.size() == 5u);
   cat::verify(fixed_span.data() == values);
}

// `data()` and `size()` reflect the underlying buffer, and `extent` reports the
// compile-time length for fixed spans or `dynamic_extent` for dynamic ones.
test(span_data_size_extent_observers) {
   int4 values[3] = {1, 2, 3};

   cat::span<int4> dynamic_span{values, 3u};
   static_assert(decltype(dynamic_span)::extent == cat::dynamic_extent);
   cat::verify(dynamic_span.size() == 3u);

   cat::span<int4, 3_idx> fixed_span{values, 3u};
   static_assert(decltype(fixed_span)::extent == 3u);
   cat::verify(fixed_span.size() == 3u);

   cat::span<int4 const> const_span{values, 3u};
   static_assert(cat::is_same<decltype(const_span.data()), int4 const*>);
}

test(span_initializer_list_constraints) {
   bool values[4] = {true, false, true, false};
   bool* p_values = values;
   idx const count = 4u;

   cat::span<bool> span_nonconst{p_values, count};
   cat::verify(span_nonconst.size() == count);

   cat::span<bool const> span_const{p_values, count};
   cat::verify(span_const.size() == count);

   cat::span<bool const> from_list{true, false, true};
   cat::verify(from_list.size() == 3u);
   cat::verify(from_list[0] && !from_list[1] && from_list[2]);

   static_assert(cat::is_constructible<cat::span<bool const>,
                                       std::initializer_list<bool>>);
   static_assert(!cat::is_constructible<cat::span<bool const>,
                                        std::initializer_list<int>>);
}

test(span_type_safety_and_extent) {
   int4 values[4] = {1, 2, 3, 4};

   cat::span from_array = values;
   static_assert(decltype(from_array)::extent == 4u);
   cat::verify(from_array.size() == 4u);

   cat::span<int4, 4_idx> fixed{values, 4u};
   static_assert(decltype(fixed)::extent == 4u);
   cat::verify(fixed.size() == 4u);

   cat::span<int4 const, 4_idx> fixed_const = fixed;
   cat::span<int4 const> dynamic_const = fixed;
   cat::verify(fixed_const.size() == 4u);
   cat::verify(dynamic_const.size() == 4u);

   static_assert(
      !cat::is_constructible<cat::span<int4>, cat::span<int4 const>>);
}

test(span_construction_from_array_container) {
   cat::array<int4, 4u> source{1_i4, 2_i4, 3_i4, 4_i4};

   cat::span<int4> view = source;
   cat::verify(view.size() == 4u);
   cat::verify(view.data() == source.data());

   cat::span<int4 const> const_view = source;
   cat::verify(const_view.size() == 4u);
   cat::verify(const_view.data() == source.data());
}

// CTAD guides should pick `span<T, N>` for raw arrays and propagate `const` for
// `const` containers.
test(span_ctad_from_raw_array) {
   int4 values[3] = {1, 2, 3};
   cat::span deduced = values;
   static_assert(cat::is_same<decltype(deduced), cat::span<int4, 3_idx>>);
   cat::verify(deduced.size() == 3u);
}

test(span_ctad_from_container) {
   cat::array<int4, 5u> source{1_i4, 2_i4, 3_i4, 4_i4, 5_i4};
   cat::span deduced = source;
   cat::verify(deduced.size() == 5u);
   cat::verify(deduced.data() == source.data());

   cat::array<int4, 5u> const csource{1_i4, 2_i4, 3_i4, 4_i4, 5_i4};
   cat::span deduced_const = csource;
   cat::verify(deduced_const.size() == 5u);
   cat::verify(deduced_const.data() == csource.data());
}

test(span_construction_from_const_container) {
   cat::array<int4, 5u> const csource{1_i4, 2_i4, 3_i4, 4_i4, 5_i4};

   cat::span<int4 const> view = csource;
   cat::verify(view.size() == 5u);
   cat::verify(view.data() == csource.data());
}

// `cat::span<T>` implicitly widens to `cat::span<T const>`, but the reverse
// would silently strip `const` and is rejected.
test(span_const_correctness_conversions) {
   cat::array<int4, 3u> source{1_i4, 2_i4, 3_i4};
   cat::span<int4> view = source;
   cat::span<int4 const> const_view = view;
   cat::verify(const_view.size() == 3u);
   cat::verify(const_view.data() == view.data());

   static_assert(
      !cat::is_constructible<cat::span<int4>, cat::span<int4 const>>);
}

test(span_assignment_dynamic) {
   int4 values_a[3] = {1, 2, 3};
   int4 values_b[5] = {10, 20, 30, 40, 50};

   cat::span<int4> view{values_a, 3u};
   cat::span<int4> other{values_b, 5u};
   view = other;
   cat::verify(view.size() == 5u);
   cat::verify(view.data() == values_b);
}

test(span_element_access_subscript) {
   int4 values[4] = {11, 22, 33, 44};
   cat::span<int4> view{values, 4u};

   cat::verify(view[0u] == 11);
   cat::verify(view[3u] == 44);

   view[1u] = 99;
   cat::verify(values[1] == 99);

   cat::span<int4 const> const_view{values, 4u};
   cat::verify(const_view[2u] == 33);
}

// `.at()` returns a `maybe<T&>` rather than asserting on out-of-bounds, so it
// is safe to query indices beyond `size()`.
test(span_at_returns_value_or_nullopt) {
   int4 values[3] = {7, 8, 9};
   cat::span<int4> view{values, 3u};

   cat::verify(view.at(0u).value() == 7);
   cat::verify(view.at(2u).value() == 9);
   cat::verify(!view.at(3u).has_value());
   cat::verify(!view.at(99u).has_value());
}

test(span_front_back) {
   int4 values[5] = {100, 200, 300, 400, 500};
   cat::span<int4> view{values, 5u};

   cat::verify(view.front() == 100);
   cat::verify(view.back() == 500);

   view.front() = -1;
   view.back() = -2;
   cat::verify(values[0] == -1);
   cat::verify(values[4] == -2);
}

test(span_iteration_forward) {
   int4 values[4] = {1, 2, 3, 4};
   cat::span<int4> view{values, 4u};

   idx index = 0u;
   for (int4& element : view) {
      cat::verify(element == values[index]);
      ++index;
   }
   cat::verify(index == 4u);
}

test(span_iteration_reverse) {
   int4 values[4] = {1, 2, 3, 4};
   cat::span<int4> view{values, 4u};

   // Each visited element should equal the value at the descending index.
   iword position = 4u;
   for (int4& element : cat::as_reverse_stepanov(view)) {
      position -= 1;
      cat::verify(element == values[narrow_cast<idx>(position).verify()]);
   }
}

test(span_iteration_const) {
   int4 values[3] = {5, 6, 7};
   cat::span<int4> view{values, 3u};

   idx index = 0u;
   for (int4 const& element : cat::as_const(view)) {
      cat::verify(element == values[index.raw]);
      ++index;
   }
   cat::verify(index == 3u);
}

test(span_subspan) {
   int4 values[6] = {10, 20, 30, 40, 50, 60};
   cat::span<int4> view{values, 6u};

   cat::span<int4> middle = view.subspan(1u, 3u);
   cat::verify(middle.size() == 3u);
   cat::verify(middle[0] == 20 && middle[1] == 30 && middle[2] == 40);

   cat::span<int4 const> const_view{values, 6u};
   cat::span<int4 const> const_middle = const_view.subspan(2u, 3u);
   cat::verify(const_middle.size() == 3u);
   cat::verify(const_middle[0] == 30 && const_middle[2] == 50);

   // Defaulting `count` extends the subspan to the end.
   cat::span<int4> tail = view.subspan(4u);
   cat::verify(tail.size() == 2u);
   cat::verify(tail[0] == 50 && tail[1] == 60);
}

test(span_first_last) {
   int4 values[6] = {1, 2, 3, 4, 5, 6};
   cat::span<int4> view{values, 6u};

   cat::span<int4> head = view.first(3u);
   cat::verify(head.size() == 3u);
   cat::verify(head[0] == 1 && head[2] == 3);

   cat::span<int4> tail = view.last(2u);
   cat::verify(tail.size() == 2u);
   cat::verify(tail[0] == 5 && tail[1] == 6);
}

test(span_compare_equal_sizes_and_content) {
   cat::array<int4, 3u> a{1_i4, 2_i4, 3_i4};
   cat::array<int4, 3u> b{1_i4, 2_i4, 3_i4};
   cat::array<int4, 3u> c{1_i4, 2_i4, 4_i4};

   cat::span<int4 const> view = a;
   cat::verify(view == b);
   cat::verify(b == view);
   cat::verify(!(view == c));
   cat::verify(view != c);
}

// `operator==` short-circuits to `false` when the sizes differ, while
// `operator<=>` falls back to the shorter-side-is-less rule.
test(span_compare_different_sizes) {
   cat::array<int4, 3u> three{1_i4, 2_i4, 3_i4};
   cat::array<int4, 4u> prefix_match{1_i4, 2_i4, 3_i4, 4_i4};

   cat::span<int4 const> view = three;
   cat::verify(!(view == prefix_match));
   cat::verify(view != prefix_match);
   cat::verify((view <=> prefix_match) < 0);
}

test(span_compare_three_way_orderings) {
   cat::array<int4, 3u> base{1_i4, 2_i4, 3_i4};
   cat::array<int4, 3u> equal{1_i4, 2_i4, 3_i4};
   cat::array<int4, 3u> bigger{1_i4, 2_i4, 9_i4};
   cat::array<int4, 3u> smaller{1_i4, 2_i4, 0_i4};

   cat::span<int4 const> view = base;
   cat::verify((view <=> equal) == 0);
   cat::verify((view <=> bigger) < 0);
   cat::verify((view <=> smaller) > 0);
}

test(span_compare_consteval) {
   constexpr auto check = [] consteval -> bool {
      cat::array<int4, 3u> a{1_i4, 2_i4, 3_i4};
      cat::array<int4, 3u> b{1_i4, 2_i4, 3_i4};
      cat::array<int4, 3u> c{1_i4, 2_i4, 4_i4};

      cat::span<int4 const> view_a = a;
      cat::span<int4 const> view_b = b;
      cat::span<int4 const> view_c = c;

      return view_a == view_b && view_a != view_c && (view_a <=> view_c) < 0;
   };
   static_assert(check());
}

test(span_compare_empty_spans) {
   cat::array<int4, 0u> empty_a;
   cat::array<int4, 0u> empty_b;

   cat::span<int4 const> view = empty_a;
   cat::verify(view == empty_b);
   cat::verify((view <=> empty_b) == 0);
}

test(span_deep_comparisons_span_ext_style) {
   cat::array<int4, 3u> values{1_i4, 2_i4, 3_i4};
   cat::array<int4, 3u> same{1_i4, 2_i4, 3_i4};
   cat::array<int4, 3u> larger{1_i4, 2_i4, 4_i4};

   cat::span<int4 const> view = values;
   cat::verify(view == same);
   cat::verify(same == view);
   cat::verify((view <=> same) == 0);
   cat::verify((view <=> larger) < 0);
}

test(span_fixed_extent_tuple_protocol) {
   int4 values[3] = {7, 8, 9};
   cat::span<int4, 3_idx> fixed{values, 3u};

   static_assert(std::tuple_size_v<cat::span<int4, 3_idx>> == 3u);
   static_assert(
      cat::is_same<std::tuple_element_t<0, cat::span<int4, 3_idx>>, int4&>);
   static_assert(
      cat::is_same<std::tuple_element_t<0, cat::span<int4, 3_idx> const>,
                   int4&>);

   auto& [x, y, z] = fixed;
   cat::verify(x == 7 && y == 8 && z == 9);

   cat::get<1_idx>(fixed) = 22;
   cat::verify(values[1] == 22);
}

// The tuple protocol is only opted-in for fixed-extent spans.
test(span_tuple_protocol_excludes_dynamic_extent) {
   static_assert(has_tuple_size<cat::span<int4, 3_idx>>);
   static_assert(!has_tuple_size<cat::span<int4>>);
}

test(span_traits) {
   static_assert(cat::is_trivially_copyable<cat::span<int4>>);
   static_assert(cat::is_trivially_copy_constructible<cat::span<int4>>);
   static_assert(cat::is_trivially_destructible<cat::span<int4>>);
   static_assert(cat::is_trivially_copyable<cat::span<int4, 4_idx>>);
}

// `is_memory_overlapping` for spans takes both sides as `span<T const>` so
// non-`const` arguments must be implicitly converted.
test(span_is_memory_overlapping_helper) {
   int4 source[4] = {1, 2, 3, 4};
   int4 dest[4] = {0, 0, 0, 0};

   cat::span<int4 const> source_view{source, 4u};
   cat::span<int4 const> dest_view{dest, 4u};

   cat::verify(!cat::is_memory_overlapping(source_view, dest_view));

   cat::span<int4 const> aliasing_view{source, 4u};
   cat::verify(cat::is_memory_overlapping(source_view, aliasing_view));
}

// Member typedefs follow `std::span`. `value_type` is `remove_cv_t<T>`, not
// `T`, so a `span<int4 const>` reports `value_type == int4`.
test(span_nested_typedefs) {
   using mutable_span = cat::span<int4>;
   using const_span = cat::span<int4 const>;
   using fixed_span = cat::span<int4, 4_idx>;

   static_assert(cat::is_same<mutable_span::element_type, int4>);
   static_assert(cat::is_same<mutable_span::value_type, int4>);
   static_assert(cat::is_same<mutable_span::size_type, cat::idx>);
   static_assert(cat::is_same<mutable_span::difference_type, cat::iword>);
   static_assert(cat::is_same<mutable_span::pointer, int4*>);
   static_assert(cat::is_same<mutable_span::const_pointer, int4 const*>);
   static_assert(cat::is_same<mutable_span::reference, int4&>);
   static_assert(cat::is_same<mutable_span::const_reference, int4 const&>);

   // For a span over `T const`, `element_type` keeps the `const` (mirrors
   // `std::span`). `value_type` strips it (also mirrors `std::span`, since
   // `container_interface::value_type = remove_cv<T>`).
   static_assert(cat::is_same<const_span::element_type, int4 const>);
   static_assert(cat::is_same<const_span::value_type, int4>);
   static_assert(cat::is_same<const_span::pointer, int4 const*>);
   static_assert(cat::is_same<const_span::reference, int4 const&>);

   // Same typedefs are reachable on a fixed-extent span.
   static_assert(cat::is_same<fixed_span::element_type, int4>);
   static_assert(cat::is_same<fixed_span::value_type, int4>);
}

// `size_bytes()` returns `size() * sizeof(T)` so it scales with the element
// width.
test(span_size_bytes) {
   int4 values[4] = {1, 2, 3, 4};
   cat::span<int4> view{values, 4u};
   cat::verify(view.size_bytes() == 4u * sizeof(int4));

   cat::span<int4, 4_idx> fixed{values, 4u};
   cat::verify(fixed.size_bytes() == 4u * sizeof(int4));

   cat::span<int4> empty;
   cat::verify(empty.size_bytes() == 0u);
}

// Template `first<N>()` returns a fixed-extent subview, even when the source
// extent is dynamic.
test(span_first_template) {
   int4 values[6] = {10, 20, 30, 40, 50, 60};
   cat::span<int4> dynamic_view{values, 6u};

   cat::span<int4, 3_idx> head = dynamic_view.first<3_idx>();
   static_assert(decltype(head)::extent == 3_idx);
   cat::verify(head.size() == 3u);
   cat::verify(head[0u] == 10 && head[1u] == 20 && head[2u] == 30);

   cat::span<int4, 6_idx> fixed_view{values, 6u};
   auto fixed_head = fixed_view.first<2_idx>();
   static_assert(decltype(fixed_head)::extent == 2_idx);
   cat::verify(fixed_head.size() == 2u);
   cat::verify(fixed_head[0u] == 10 && fixed_head[1u] == 20);
}

// Template `last<N>()` returns a fixed-extent subview anchored at the end.
test(span_last_template) {
   int4 values[6] = {10, 20, 30, 40, 50, 60};
   cat::span<int4> dynamic_view{values, 6u};

   cat::span<int4, 2_idx> tail = dynamic_view.last<2_idx>();
   static_assert(decltype(tail)::extent == 2_idx);
   cat::verify(tail.size() == 2u);
   cat::verify(tail[0u] == 50 && tail[1u] == 60);

   cat::span<int4, 6_idx> fixed_view{values, 6u};
   auto fixed_tail = fixed_view.last<3_idx>();
   static_assert(decltype(fixed_tail)::extent == 3_idx);
   cat::verify(fixed_tail.size() == 3u);
   cat::verify(fixed_tail[0u] == 40 && fixed_tail[2u] == 60);
}

// Template `subspan<Offset, Count>()` mirrors `std::span`. With a fixed source
// extent and an explicit `Count`, the result extent equals `Count`. With a
// fixed source extent and a defaulted `Count`, the result extent is
// `Extent - Offset`. With a dynamic source extent, the result is dynamic unless
// `Count` is given.
test(span_subspan_template) {
   int4 values[6] = {10, 20, 30, 40, 50, 60};
   cat::span<int4, 6_idx> fixed_view{values, 6u};

   auto explicit_count = fixed_view.subspan<1_idx, 3_idx>();
   static_assert(decltype(explicit_count)::extent == 3_idx);
   cat::verify(explicit_count.size() == 3u);
   cat::verify(explicit_count[0u] == 20 && explicit_count[2u] == 40);

   auto defaulted_count = fixed_view.subspan<2_idx>();
   static_assert(decltype(defaulted_count)::extent == 4_idx);
   cat::verify(defaulted_count.size() == 4u);
   cat::verify(defaulted_count[0u] == 30 && defaulted_count[3u] == 60);

   cat::span<int4> dynamic_view{values, 6u};
   auto dyn_explicit = dynamic_view.subspan<1_idx, 3_idx>();
   static_assert(decltype(dyn_explicit)::extent == 3_idx);
   cat::verify(dyn_explicit.size() == 3u);

   auto dyn_defaulted = dynamic_view.subspan<2_idx>();
   static_assert(decltype(dyn_defaulted)::extent == cat::dynamic_extent);
   cat::verify(dyn_defaulted.size() == 4u);
   cat::verify(dyn_defaulted[0u] == 30 && dyn_defaulted[3u] == 60);
}

// The runtime `subspan(offset, count)` overload coexists with the template
// `subspan<Offset, Count>()` form without ambiguity.
test(span_subspan_runtime_and_template_coexist) {
   int4 values[6] = {10, 20, 30, 40, 50, 60};
   cat::span<int4> view{values, 6u};

   cat::span<int4> runtime_middle = view.subspan(1u, 3u);
   cat::verify(runtime_middle.size() == 3u);
   cat::verify(runtime_middle[0u] == 20 && runtime_middle[2u] == 40);

   auto template_middle = view.subspan<1_idx, 3_idx>();
   static_assert(decltype(template_middle)::extent == 3_idx);
   cat::verify(template_middle.size() == 3u);
   cat::verify(template_middle[0u] == 20 && template_middle[2u] == 40);
}

// `as_bytes` produces a `span<byte const>` whose extent scales by `sizeof(T)`
// when the source extent is fixed.
test(span_as_bytes) {
   int4 values[2] = {0, 0};
   cat::span<int4, 2_idx> fixed{values, 2u};

   auto bytes = cat::as_bytes(fixed);
   static_assert(cat::is_same<decltype(bytes)::element_type, cat::byte const>);
   static_assert(decltype(bytes)::extent == idx(2u * sizeof(int4)));
   cat::verify(bytes.size() == 2u * sizeof(int4));
   cat::verify(bytes.size_bytes() == 2u * sizeof(int4));

   cat::span<int4> dynamic{values, 2u};
   auto dyn_bytes = cat::as_bytes(dynamic);
   static_assert(decltype(dyn_bytes)::extent == cat::dynamic_extent);
   cat::verify(dyn_bytes.size() == 2u * sizeof(int4));
}

// `as_writable_bytes` only accepts spans whose element type is non-`const`.
// Writes through the byte view are observable through the source span.
test(span_as_writable_bytes) {
   int4 values[1] = {0};
   cat::span<int4, 1_idx> fixed{values, 1u};

   auto bytes = cat::as_writable_bytes(fixed);
   static_assert(cat::is_same<decltype(bytes)::element_type, cat::byte>);
   static_assert(decltype(bytes)::extent == idx(sizeof(int4)));
   cat::verify(bytes.size() == sizeof(int4));

   // `const` spans are rejected at compile time.
   static_assert(can_as_writable_bytes<cat::span<int4>>);
   static_assert(!can_as_writable_bytes<cat::span<int4 const>>);
}

// Mirrors the static `equality_comparable_with`/`totally_ordered_with` asserts
// in `span_ext`'s `test.cxx`. A `cat::span` is equality-comparable and
// orderable with any random-access range whose element type matches.
test(span_compare_concepts_random_access_match) {
   static_assert(
      is_equality_comparable_with<cat::span<int4>, cat::array<int4, 3u>>);
   static_assert(
      is_totally_ordered_with<cat::span<int4>, cat::array<int4, 3u>>);
}

// Mirrors `span_ext`'s mixed-`const`-ness checks: comparing a span and a
// container should work in every `const` combination of the two operands.
test(span_compare_concepts_mixed_constness) {
   static_assert(
      is_totally_ordered_with<cat::span<int4>, cat::array<int4, 3u> const>);
   static_assert(
      is_totally_ordered_with<cat::span<int4 const>, cat::array<int4, 3u>>);
   static_assert(is_totally_ordered_with<cat::span<int4 const>,
                                         cat::array<int4, 3u> const>);
}

// Mirrors `not equality_comparable_with<std::span<int>, std::vector<long>>`
// from `span_ext`: the element types must match (modulo cv) for either form of
// comparison to compile.
test(span_compare_concepts_element_type_mismatch) {
   static_assert(
      !is_equality_comparable_with<cat::span<int4>, cat::array<int8, 3u>>);
   static_assert(
      !is_totally_ordered_with<cat::span<int4>, cat::array<int8, 3u>>);
}

// Mirrors `span_ext`'s `B`/`D` cases. A type with only `operator==` makes the
// span equality-comparable but not ordered. Inheritance carries the equality
// through, but pairing differently-typed spans is rejected even when the
// elements share a base.
test(span_compare_concepts_only_equality_element) {
   static_assert(is_equality_comparable_with<cat::span<only_equality>,
                                             cat::span<only_equality>>);
   static_assert(!is_totally_ordered_with<cat::span<only_equality>,
                                          cat::span<only_equality>>);

   static_assert(is_equality_comparable_with<cat::span<derived_only_equality>,
                                             cat::span<derived_only_equality>>);
   static_assert(!is_totally_ordered_with<cat::span<derived_only_equality>,
                                          cat::span<derived_only_equality>>);

   static_assert(
      !is_equality_comparable_with<cat::span<only_equality>,
                                   cat::span<derived_only_equality>>);
}

// Mirrors `not equality_comparable_with<std::span<NonComparable>, ...>` from
// `span_ext`: an element type with no comparison operators disables both forms.
test(span_compare_concepts_uncomparable_element) {
   static_assert(!is_equality_comparable_with<cat::span<uncomparable>,
                                              cat::span<uncomparable>>);
   static_assert(!is_totally_ordered_with<cat::span<uncomparable>,
                                          cat::span<uncomparable>>);
}

// Mirrors `span_ext`'s `compare_self` template test: every relational operator
// returns the expected boolean when both sides hold identical contents,
// regardless of which operand is on the left.
test(span_compare_self_all_int_widths) {
   for_each_int_width([]<typename T>() {
      cat::array<T, 3u> x{T(1), T(2), T(3)};
      cat::span<T> s = x;

      cat::verify(s == x);
      cat::verify(!(s != x));
      cat::verify(!(s < x));
      cat::verify(s <= x);
      cat::verify(!(s > x));
      cat::verify(s >= x);

      cat::verify(x == s);
      cat::verify(!(x != s));
      cat::verify(!(x < s));
      cat::verify(x <= s);
      cat::verify(!(x > s));
      cat::verify(x >= s);
   });
}

// Mirrors `span_ext`'s `compare_same_length_diff` template test: when the
// length matches but the last element differs, ordering follows the differing
// element and equality is `false`.
test(span_compare_same_length_diff_all_int_widths) {
   for_each_int_width([]<typename T>() {
      cat::array<T, 3u> x{T(1), T(2), T(3)};
      cat::array<T, 3u> y{T(1), T(2), T(4)};
      cat::span<T const> sx = x;
      cat::span<T const> sy = y;

      cat::verify(!(sx == sy));
      cat::verify(sx != sy);
      cat::verify(sx < sy);
      cat::verify(sx <= sy);
      cat::verify(!(sx > sy));
      cat::verify(!(sx >= sy));

      cat::verify(!(sy == sx));
      cat::verify(sy != sx);
      cat::verify(!(sy < sx));
      cat::verify(!(sy <= sx));
      cat::verify(sy > sx);
      cat::verify(sy >= sx);
   });
}

// Mirrors `span_ext`'s `compare_prefix` template test: a span that is a strict
// prefix of another orders below the longer one and is never equal.
test(span_compare_prefix_all_int_widths) {
   for_each_int_width([]<typename T>() {
      cat::array<T, 3u> shorter{T(1), T(2), T(3)};
      cat::array<T, 4u> longer{T(1), T(2), T(3), T(4)};
      cat::span<T const> ss = shorter;
      cat::span<T const> sl = longer;

      cat::verify(!(ss == sl));
      cat::verify(ss != sl);
      cat::verify(ss < sl);
      cat::verify(ss <= sl);
      cat::verify(!(ss > sl));
      cat::verify(!(ss >= sl));

      cat::verify(!(sl == ss));
      cat::verify(sl != ss);
      cat::verify(!(sl < ss));
      cat::verify(!(sl <= ss));
      cat::verify(sl > ss);
      cat::verify(sl >= ss);
   });
}

// Two spans over the same underlying buffer must compare equal regardless of
// how each side was constructed (raw pointer + size vs. CTAD from container).
test(span_compare_aliasing_buffers) {
   int4 values[3] = {1, 2, 3};
   cat::span<int4> direct{values, 3u};
   cat::span<int4 const> from_pointer{values, 3u};

   cat::verify(direct == from_pointer);
   cat::verify(from_pointer == direct);
   cat::verify((direct <=> from_pointer) == 0);
}

// A fixed-extent span and a dynamic-extent span over equal contents compare
// equal: the constraint only requires `value_type` parity, not extent parity.
test(span_compare_fixed_vs_dynamic_extent) {
   int4 values[3] = {1, 2, 3};
   cat::span<int4 const, 3_idx> fixed{values, 3u};
   cat::span<int4 const> dynamic{values, 3u};

   cat::verify(fixed == dynamic);
   cat::verify(dynamic == fixed);
   cat::verify((fixed <=> dynamic) == 0);
}

// `operator==` is short-circuit: when sizes differ it returns `false` without
// inspecting any element, even if the prefix would otherwise match.
// `operator<=>` instead falls back to the shorter-is-less rule.
test(span_compare_size_mismatch_short_circuits) {
   int4 short_values[2] = {1, 2};
   int4 long_values[3] = {1, 2, 3};

   cat::span<int4 const> shorter{short_values, 2u};
   cat::span<int4 const> longer{long_values, 3u};

   cat::verify(!(shorter == longer));
   cat::verify(shorter != longer);
   cat::verify((shorter <=> longer) < 0);
   cat::verify((longer <=> shorter) > 0);
}

// Cppreference parity: explicit iterator method names must work directly
// (range-for already exercises `begin`/`end` indirectly).
test(span_iterator_methods_explicit) {
   int4 values[4] = {10, 20, 30, 40};
   cat::span<int4> view{values, 4u};

   auto first = view.begin();
   auto last = view.end();
   cat::verify(*first == 10);
   cat::verify(*(last - 1) == 40);
   cat::verify((last - first) == 4);
}

// `cbegin`/`cend` always yield iterators to `T const`, even on a non-`const`
// span.
test(span_iterator_methods_const_cbegin_cend) {
   int4 values[3] = {1, 2, 3};
   cat::span<int4> view{values, 3u};

   auto first = view.cbegin();
   auto last = view.cend();
   cat::verify(*first == 1);
   cat::verify(*(last - 1) == 3);
}

// Reverse iterator methods produce the elements in descending order.
test(span_iterator_methods_reverse) {
   int4 values[4] = {1, 2, 3, 4};
   cat::span<int4> view{values, 4u};

   auto rfirst = view.rbegin();
   cat::verify(*rfirst == 4);

   auto crfirst = view.crbegin();
   cat::verify(*crfirst == 4);
}

// `iterator` is a contiguous (random-access) iterator type. Verify the libCat
// concepts hold so generic algorithms can pick the contiguous specialization.
test(span_iterator_concepts) {
   using iterator = cat::span<int4>::iterator;
   static_assert(cat::is_random_access_stepanov_iterator<iterator>);
}

// A zero-extent span is a well-formed empty view: no allocation, no data,
// equal-by-content to any other empty span.
test(span_empty_fixed_extent) {
   cat::span<int4, 0_idx> empty;
   static_assert(decltype(empty)::extent == 0_idx);
   cat::verify(empty.size() == 0u);
   cat::verify(empty.is_empty());
   cat::verify(empty.size_bytes() == 0u);

   cat::span<int4> dyn_empty;
   cat::verify(empty == dyn_empty);
}

// Calling subview helpers with a count of zero on a non-empty span yields an
// empty span, leaving the source unchanged.
test(span_subview_zero_count) {
   int4 values[4] = {1, 2, 3, 4};
   cat::span<int4> view{values, 4u};

   auto head = view.first<0_idx>();
   static_assert(decltype(head)::extent == 0_idx);
   cat::verify(head.size() == 0u);

   auto tail = view.last<0_idx>();
   static_assert(decltype(tail)::extent == 0_idx);
   cat::verify(tail.size() == 0u);

   auto mid = view.subspan<2_idx, 0_idx>();
   static_assert(decltype(mid)::extent == 0_idx);
   cat::verify(mid.size() == 0u);
}

// Asking for the full extent via the template subview overloads should yield a
// span of the original size, with the same data pointer.
test(span_subview_full_extent) {
   int4 values[4] = {1, 2, 3, 4};
   cat::span<int4, 4_idx> view{values, 4u};

   auto all_first = view.first<4_idx>();
   static_assert(decltype(all_first)::extent == 4_idx);
   cat::verify(all_first.data() == view.data());
   cat::verify(all_first.size() == 4u);

   auto all_last = view.last<4_idx>();
   static_assert(decltype(all_last)::extent == 4_idx);
   cat::verify(all_last.data() == view.data());
   cat::verify(all_last.size() == 4u);

   auto all_subspan = view.subspan<0_idx, 4_idx>();
   static_assert(decltype(all_subspan)::extent == 4_idx);
   cat::verify(all_subspan.data() == view.data());
   cat::verify(all_subspan.size() == 4u);
}

// Subviews must compose: chaining `first` and `last` and `subspan` preserves
// both data positions and extents.
test(span_subview_chain) {
   int4 values[8] = {1, 2, 3, 4, 5, 6, 7, 8};
   cat::span<int4, 8_idx> view{values, 8u};

   auto middle_six = view.subspan<1_idx, 6_idx>();
   auto middle_first_three = middle_six.first<3_idx>();
   auto middle_first_three_last_two = middle_first_three.last<2_idx>();

   static_assert(decltype(middle_first_three_last_two)::extent == 2_idx);
   cat::verify(middle_first_three_last_two[0u] == 3);
   cat::verify(middle_first_three_last_two[1u] == 4);
}

// Writes through one span are observable through any aliasing span over the
// same buffer (spans are non-owning views).
test(span_write_through_view) {
   int4 values[3] = {0, 0, 0};
   cat::span<int4> alpha{values, 3u};
   cat::span<int4> beta{values, 3u};

   alpha[0u] = 100;
   alpha[2u] = 300;
   cat::verify(beta[0u] == 100);
   cat::verify(beta[2u] == 300);
   cat::verify(values[1] == 0);
}

// On a fixed-extent span the tuple `get` overloads are reachable on a
// `const`-qualified span, but the returned reference is to the same element
// regardless of `const` (matches `std::span` since `T const&` would require `T`
// to itself be `const`).
test(span_tuple_get_on_const_span) {
   int4 values[2] = {7, 8};
   cat::span<int4, 2_idx> const fixed{values, 2u};

   static_assert(cat::is_same<decltype(cat::get<0_idx>(fixed)), int4&>);
   cat::verify(cat::get<0_idx>(fixed) == 7);
   cat::verify(cat::get<1_idx>(fixed) == 8);
}

// `dynamic_extent` is exposed as a `cat::idx`. Spot-check it equals the
// sentinel used everywhere internally and selects the dynamic specialization of
// the primary template.
test(span_dynamic_extent_constant) {
   static_assert(cat::is_same<decltype(cat::dynamic_extent), cat::idx const>);
   static_assert(cat::span<int4>::extent == cat::dynamic_extent);
   static_assert(cat::span<int4, 4_idx>::extent != cat::dynamic_extent);
}
