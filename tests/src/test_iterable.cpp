#include <cat/debug>
#include <cat/iterable>
#include <cat/utility>

#include "../unit_tests.hpp"

// Two fixtures exercise the two halves of Flux v2's split protocol.
//
//    `tiny_array<T, count>`: models `cat::is_random_access_collection`
//    plus contiguous storage. Exposes the full position protocol -
//    `begin_pos`/`end_pos`/`inc_pos`/`dec_pos`/`inc_pos(p, n)`/
//    `distance`/`read_at_unchecked` - over a `cat::idx` position, plus
//    `data()` and `size()` for `cat::as_span`. `cat::iterate` and
//    `cat::reverse_iterate` discover it through the position protocol
//    and wrap it in the matching iteration context.
//
//    `tiny_list<T, count>`: models `cat::is_iterable` only. Storage is
//    a fixed-size pool of nodes linked by *indices* (so a copy of the
//    list is still a valid list). It exposes a member `iterate()` and a
//    member `reverse_iterate()` to demonstrate the opt-in path for
//    sources that are not collections but can still be walked
//    backwards.
//
// `noisy_box` is a third fixture used only for the ownership-policy tests: it
// is intentionally non-trivially-copyable so an adaptor will reject it as an
// l-value base, forcing the caller through `cat::ref`/`cat::cref` to opt in.
template <typename T, cat::idx extent>
struct tiny_array
    : cat::contiguous_collection_interface<tiny_array<T, extent>> {
   T m_data[extent]{};

   [[nodiscard]]
   constexpr auto
   data() -> T* {
      return m_data;
   }

   [[nodiscard]]
   constexpr auto
   data() const -> T const* {
      return m_data;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> cat::idx {
      return extent;
   }
};

// `capacity` (rather than `count`) names the node-pool size so it doesn't
// collide with the inherited `count()` member.
template <typename T, cat::idx capacity>
class tiny_list : public cat::iterable_interface {
   struct node {
      T value{};
      // Index of the next node, or `capacity` for end-of-list. Index-based so a
      // by-value copy of the list stays internally consistent.
      cat::idx next = capacity;
   };

   node m_nodes[capacity]{};
   cat::idx m_size = 0u;
   cat::idx m_head = capacity;

 public:
   constexpr auto
   push_back(T value) -> void {
      cat::verify(m_size < capacity);
      cat::idx const slot = m_size;
      m_nodes[slot] = node{value, capacity};
      ++m_size;
      if (m_head == capacity) {
         m_head = slot;
         return;
      }
      cat::idx tail = m_head;
      while (m_nodes[tail].next != capacity) {
         tail = m_nodes[tail].next;
      }
      m_nodes[tail].next = slot;
   }

   class iteration_context {
      tiny_list const* m_p_list;
      cat::idx m_current;

    public:
      using element_type = T const&;

      constexpr iteration_context(tiny_list const* p_list, cat::idx start)
          : m_p_list(p_list), m_current(start) {
      }

      iteration_context(iteration_context const&) = delete;
      iteration_context(iteration_context&&) = delete;
      auto
      operator=(iteration_context const&) -> iteration_context& = delete;
      auto
      operator=(iteration_context&&) -> iteration_context& = delete;

      template <typename Pred>
      constexpr auto
      run_while(Pred&& pred) -> cat::iteration_result {
         while (m_current != capacity) {
            cat::idx const here = m_current;
            m_current = m_p_list->m_nodes[here].next;
            if (!pred(m_p_list->m_nodes[here].value)) {
               return cat::iteration_result::incomplete;
            }
         }
         return cat::iteration_result::complete;
      }
   };

   constexpr auto
   iterate() const -> iteration_context {
      return iteration_context{this, m_head};
   }

   class reverse_iteration_context {
      tiny_list const* m_p_list;
      // List of indices, reversed, kept by value. We resolve the chain up front
      // because singly-linked lists cannot walk backwards in place. This is the
      // canonical pattern when a source can model `reverse_iterable` but not
      // `bidirectional_collection`.
      cat::idx m_indices[capacity]{};
      cat::iword m_remaining;

    public:
      using element_type = T const&;

      constexpr reverse_iteration_context(tiny_list const* p_list)
          : m_p_list(p_list), m_remaining(p_list->m_size) {
         cat::idx node_index = p_list->m_head;
         cat::idx slot;
         for (slot = 0u; slot < p_list->m_size; ++slot) {
            m_indices[slot] = node_index;
            node_index = p_list->m_nodes[node_index].next;
         }
      }

      reverse_iteration_context(reverse_iteration_context const&) = delete;
      reverse_iteration_context(reverse_iteration_context&&) = delete;
      auto
      operator=(reverse_iteration_context const&)
         -> reverse_iteration_context& = delete;
      auto
      operator=(reverse_iteration_context&&)
         -> reverse_iteration_context& = delete;

      template <typename Pred>
      constexpr auto
      run_while(Pred&& pred) -> cat::iteration_result {
         while (m_remaining.raw > 0u) {
            m_remaining -= 1;
            cat::idx const here = m_indices[m_remaining.to_idx().verify()];
            if (!pred(m_p_list->m_nodes[here].value)) {
               return cat::iteration_result::incomplete;
            }
         }
         return cat::iteration_result::complete;
      }
   };

   constexpr auto
   reverse_iterate() const -> reverse_iteration_context {
      return reverse_iteration_context{this};
   }
};

// `noisy_box` is non-trivially-copyable on purpose - it has a user-provided
// move constructor. Adaptors must reject it as an l-value base and the caller
// has to opt in through `cat::ref` / `cat::cref`. This is the structural fix
// Flux v2 makes against accidental dangling references to lvalues.
//
// It also inherits `iterable_interface`, to demonstrate that opting into the
// member-call surface is independent of triviality - the ownership policy still
// applies, the member calls just lower to the same pipe expression.
struct noisy_box : cat::iterable_interface {
   tiny_array<int, 4u> data;

   constexpr noisy_box() = default;

   constexpr explicit noisy_box(tiny_array<int, 4u> v) : data(v) {
   }

   noisy_box(noisy_box const&) = delete;
   auto
   operator=(noisy_box const&) -> noisy_box& = delete;

   // User-provided (not defaulted) so the type is not trivially copyable, which
   // is what the ownership policy keys on.
   constexpr noisy_box(noisy_box&& other) noexcept : data(other.data) {
   }

   constexpr auto
   operator=(noisy_box&& other) noexcept -> noisy_box& {
      data = other.data;
      return *this;
   }

   constexpr auto
   begin_pos() const -> cat::idx {
      return data.begin_pos();
   }

   constexpr auto
   end_pos() const -> cat::idx {
      return data.end_pos();
   }

   constexpr auto
   inc_pos(cat::idx& p) const -> void {
      data.inc_pos(p);
   }

   constexpr auto
   read_at_unchecked(cat::idx p) const -> int const& {
      return data.read_at_unchecked(p);
   }
};

// `tiny_string` is a heap-free, fixed-buffer string just rich enough to stand
// in for `std::string` in the P3725 demo. It is non-trivially copyable
// (user-provided copy/move) and tracks whether it has been moved-from, which is
// what lets the demo assert that `as_rvalue` did in fact move - and where in
// the source.
struct tiny_string {
   static constexpr cat::idx capacity = 16u;
   char m_chars[capacity.raw]{};
   cat::idx m_len = 0u;
   bool m_moved_from = false;

   constexpr tiny_string() = default;

   // Initialise from a NUL-terminated literal. We avoid `cat::strlen` here so
   // the test stays self-contained.
   constexpr tiny_string(char const* literal) {
      while (literal[m_len.raw] != '\0') {
         m_chars[m_len.raw] = literal[m_len.raw];
         m_len.raw = m_len.raw + 1u;
      }
   }

   constexpr tiny_string(tiny_string const& other)
       : m_len(other.m_len), m_moved_from(other.m_moved_from) {
      for (cat::idx i = 0u; i < m_len; ++i) {
         m_chars[i] = other.m_chars[i];
      }
   }

   constexpr tiny_string(tiny_string&& other) noexcept
       : m_len(other.m_len), m_moved_from(other.m_moved_from) {
      for (cat::idx i = 0u; i < m_len; ++i) {
         m_chars[i] = other.m_chars[i];
      }
      other.m_len = 0u;
      other.m_moved_from = true;
   }

   constexpr auto
   operator=(tiny_string const& other) -> tiny_string& {
      m_len = other.m_len;
      m_moved_from = other.m_moved_from;
      for (cat::idx i = 0u; i < m_len; ++i) {
         m_chars[i] = other.m_chars[i];
      }
      return *this;
   }

   constexpr auto
   operator=(tiny_string&& other) noexcept -> tiny_string& {
      m_len = other.m_len;
      m_moved_from = other.m_moved_from;
      for (cat::idx i = 0u; i < m_len; ++i) {
         m_chars[i] = other.m_chars[i];
      }
      other.m_len = 0u;
      other.m_moved_from = true;
      return *this;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> cat::idx {
      return m_len;
   }

   [[nodiscard]]
   constexpr auto
   operator==(tiny_string const& other) const -> bool {
      if (m_len.raw != other.m_len.raw) {
         return false;
      }
      for (cat::idx i = 0u; i < m_len; ++i) {
         if (m_chars[i] != other.m_chars[i]) {
            return false;
         }
      }
      return true;
   }
};

// `tiny_vector<T, capacity>` is a `push_back`-able, fixed-storage container
// that satisfies `is_random_access_collection`. It is the destination for
// `cat::to<...>()` in the P3725 demo. Position protocol comes from
// `contiguous_collection_interface<tiny_vector<...>>` (see
// `<cat/detail/iterable_interface.hpp>`).
template <typename T, cat::idx capacity>
struct tiny_vector
    : cat::contiguous_collection_interface<tiny_vector<T, capacity>> {
   T m_data[capacity]{};
   cat::idx m_size = 0u;

   constexpr void
   push_back(T value) {
      cat::verify(m_size < capacity);
      m_data[m_size] = cat::move(value);
      m_size.raw = m_size.raw + 1u;
   }

   [[nodiscard]]
   constexpr auto
   data() -> T* {
      return m_data;
   }

   [[nodiscard]]
   constexpr auto
   data() const -> T const* {
      return m_data;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> cat::idx {
      return m_size;
   }
};

// === Concept sanity ==========================================================

static_assert(cat::is_position<cat::idx>);
static_assert(cat::is_collection<tiny_array<int, 4u>>);
static_assert(cat::is_iterable<tiny_array<int, 4u>>);
static_assert(cat::is_iterable<tiny_list<int, 8u>>);
// `tiny_list` is intentionally `iterable`-only - position protocol must not be
// inferred.
static_assert(!cat::is_collection<tiny_list<int, 8u>>);

// Refinement hierarchy: `tiny_array` walks all the way up to random access.
// `tiny_list` stops at iterable.
static_assert(cat::is_bidirectional_collection<tiny_array<int, 4u>>);
static_assert(cat::is_random_access_collection<tiny_array<int, 4u>>);
static_assert(!cat::is_bidirectional_collection<tiny_list<int, 8u>>);

// Reverse iteration: collections climb on bidirectional, iterables opt in by
// writing a member `reverse_iterate()`.
static_assert(cat::is_reverse_iterable<tiny_array<int, 4u>>);
static_assert(cat::is_reverse_iterable<tiny_list<int, 8u>>);

// Ownership policy: trivially-copyable lvalues are accepted, but a
// non-trivially-copyable l-value (`noisy_box&`) is not. Wrapping it in
// `cat::reference_wrapper` opts in to non-owning aliasing, which is always
// trivially copyable.
static_assert(cat::is_borrow_acceptable<tiny_array<int, 4u>&>);
static_assert(!cat::is_borrow_acceptable<noisy_box&>);
static_assert(cat::is_borrow_acceptable<cat::reference_wrapper<noisy_box>>);
static_assert(cat::is_borrow_acceptable<noisy_box>);  // r-value is fine

// === Tests ===================================================================

test(flux_collection_basics) {
   tiny_array<int, 4u> arr = {
      {},
      {1, 2, 3, 4}
   };

   // Direct iterate + run_while.
   {
      int total = 0;
      auto ctx = cat::iterate(arr);
      ctx.run_while([&total](int const& x) -> bool {
         total += x;
         return true;
      });
      cat::verify(total == 10);
   }

   // `cat::for_each` terminal.
   {
      int total = 0;
      cat::for_each(arr, [&total](int const& x) {
         total += x;
      });
      cat::verify(total == 10);
   }

   // `cat::sum` terminal.
   cat::verify(cat::sum(arr) == 10);

   // `cat::count` terminal.
   cat::verify(cat::count(arr) == 4u);
}

test(flux_iterable_basics) {
   tiny_list<int, 8u> list;
   list.push_back(10);
   list.push_back(20);
   list.push_back(30);

   // `cat::sum` terminal over a member-`iterate`-only iterable.
   cat::verify(cat::sum(list) == 60);

   // `cat::count` terminal.
   cat::verify(cat::count(list) == 3u);

   // `cat::for_each` terminal.
   int total = 0;
   cat::for_each(list, [&total](int const& x) {
      total += x;
   });
   cat::verify(total == 60);
}

test(flux_pipe_collection) {
   tiny_array<int, 6u> arr = {
      {},
      {1, 2, 3, 4, 5, 6}
   };

   // sum of squares of even values: 4 + 16 + 36 = 56.
   int result = arr
                | cat::filter([](int x) -> bool {
                     return (x % 2) == 0;
                  })
                | cat::transform([](int x) -> int {
                     return x * x;
                  })
                | cat::sum();
   cat::verify(result == 56);
}

test(flux_pipe_iterable) {
   tiny_list<int, 8u> list;
   list.push_back(1);
   list.push_back(2);
   list.push_back(3);
   list.push_back(4);
   list.push_back(5);

   // sum the odd entries: 1 + 3 + 5 = 9.
   int odd_sum = list
                 | cat::filter([](int x) -> bool {
                      return (x % 2) != 0;
                   })
                 | cat::sum();
   cat::verify(odd_sum == 9);

   // multiply each by ten then sum: 10 + 20 + 30 + 40 + 50 = 150.
   int scaled = list
                | cat::transform([](int x) -> int {
                     return x * 10;
                  })
                | cat::sum();
   cat::verify(scaled == 150);
}

test(flux_take_truncates) {
   tiny_array<int, 6u> arr = {
      {},
      {10, 20, 30, 40, 50, 60}
   };

   // Take first three: 10 + 20 + 30 = 60.
   int first_three = arr | cat::take(3u) | cat::sum();
   cat::verify(first_three == 60);

   // Taking more than the source has just exhausts the source.
   int all = arr | cat::take(100u) | cat::sum();
   cat::verify(all == 210);

   // Taking zero produces zero.
   int none = arr | cat::take(0u) | cat::sum();
   cat::verify(none == 0);
}

test(flux_run_while_pause_resume) {
   // `iteration_result::incomplete` lets the caller pause iteration and pick up
   // where they left off. This is the structural feature behind every
   // short-circuiting algorithm.
   tiny_array<int, 5u> arr = {
      {},
      {1, 2, 3, 4, 5}
   };
   auto ctx = cat::iterate(arr);

   int seen = 0;
   auto first = ctx.run_while([&seen](int const& x) -> bool {
      seen += x;
      // Stop once we've seen 1+2+3 = 6.
      return seen < 6;
   });
   cat::verify(first == cat::iteration_result::incomplete);
   cat::verify(seen == 6);

   int rest = 0;
   auto second = ctx.run_while([&rest](int const& x) -> bool {
      rest += x;
      return true;
   });
   cat::verify(second == cat::iteration_result::complete);
   cat::verify(rest == 9);  // 4 + 5
}

test(flux_run_while_immediate_interrupt) {
   // Stop on the first element, then continue with the `run_while(ctx, f)` free
   // shim. Confirms the cursor is advanced for the read that ran the body, so
   // the next pass skips that element.
   tiny_array<int, 4u> arr = {
      {},
      {1, 2, 3, 4}
   };
   auto ctx = cat::iterate(arr);

   int calls = 0;
   auto const first = ctx.run_while([&calls](int) -> bool {
      ++calls;
      return false;
   });
   cat::verify(first == cat::iteration_result::incomplete);
   cat::verify(calls == 1);

   int rest_sum = 0;
   auto const second = cat::run_while(ctx, [&rest_sum](int const& x) -> bool {
      rest_sum += x;
      return true;
   });
   cat::verify(second == cat::iteration_result::complete);
   cat::verify(rest_sum == 9);  // 2 + 3 + 4, first value (1) not seen again
}

test(flux_step_and_next_element) {
   tiny_array<int, 3u> arr = {
      {},
      {7, 8, 9}
   };
   auto ctx = cat::iterate(arr);

   // `next_element` peels one at a time.
   auto first = cat::next_element(ctx);
   cat::verify(first.has_value());
   cat::verify(first.value() == 7);

   // `step` with a non-void function returns a `maybe<R>`.
   auto doubled = cat::step(ctx, [](int x) -> int {
      return x * 2;
   });
   cat::verify(doubled.has_value());
   cat::verify(doubled.value() == 16);

   auto last = cat::next_element(ctx);
   cat::verify(last.has_value());
   cat::verify(last.value() == 9);

   // `step` with a void-returning function returns a `maybe<void>` - engaged
   // when `fn` was called, `nullopt` when the context was exhausted. This
   // collapses the void and non-void paths into one uniformly monadic surface.
   int side_effect = 0;
   auto void_step = cat::step(ctx, [&side_effect](int x) -> void {
      side_effect = x;
   });
   cat::verify(!void_step.has_value());  // ctx already exhausted
   cat::verify(side_effect == 0);

   tiny_array<int, 1u> tiny = {{}, {42}};
   auto tiny_ctx = cat::iterate(tiny);
   auto void_hit = cat::step(tiny_ctx, [&side_effect](int x) -> void {
      side_effect = x;
   });
   cat::verify(void_hit.has_value());
   cat::verify(side_effect == 42);

   auto empty = cat::next_element(ctx);
   cat::verify(!empty.has_value());
}

test(flux_fold_terminal) {
   tiny_array<int, 4u> arr = {
      {},
      {2, 3, 5, 7}
   };

   // Multiplicative fold: 2 * 3 * 5 * 7 = 210.
   int product = cat::fold(
      arr,
      [](int acc, int x) -> int {
         return acc * x;
      },
      1);
   cat::verify(product == 210);

   // Pipeable form.
   int piped = arr
               | cat::fold(
                  [](int acc, int x) -> int {
                     return acc + (x * x);
                  },
                  0);
   cat::verify(piped == 4 + 9 + 25 + 49);
}

test(flux_reverse_collection) {
   tiny_array<int, 5u> arr = {
      {},
      {1, 2, 3, 4, 5}
   };

   // Direct `reverse_iterate` walks last-to-first.
   {
      int seen[5] = {};
      cat::idx i = 0u;
      auto ctx = cat::reverse_iterate(arr);
      ctx.run_while([&seen, &i](int const& x) -> bool {
         seen[i] = x;
         ++i;
         return true;
      });
      cat::verify(seen[0] == 5);
      cat::verify(seen[1] == 4);
      cat::verify(seen[2] == 3);
      cat::verify(seen[3] == 2);
      cat::verify(seen[4] == 1);
   }

   // Pipe form: reverse + transform composes cleanly.
   int doubled_first = arr | cat::reverse() | cat::take(2u) | cat::sum();
   cat::verify(doubled_first == 9);  // 5 + 4
}

test(flux_reverse_iterable_member) {
   // Demonstrates the opt-in path: `tiny_list` is not bidirectional, but it
   // provides a member `reverse_iterate()` so it still works through
   // `cat::reverse()`.
   tiny_list<int, 8u> list;
   list.push_back(10);
   list.push_back(20);
   list.push_back(30);
   list.push_back(40);

   int reversed = list | cat::reverse() | cat::take(2u) | cat::sum();
   cat::verify(reversed == 70);  // 40 + 30
}

test(flux_read_at_and_try_read_at) {
   tiny_array<int, 4u> arr = {
      {},
      {11, 22, 33, 44}
   };

   // `read_at` is checked but not maybe-returning - in-bounds reads succeed.
   cat::verify(cat::read_at(arr, cat::idx{0u}) == 11);
   cat::verify(cat::read_at(arr, cat::idx{3u}) == 44);

   // `try_read_at` returns a `maybe` - in and out of range cases are both
   // observable.
   auto in_range = cat::try_read_at(arr, cat::idx{2u});
   cat::verify(in_range.has_value());
   cat::verify(in_range.value() == 33);

   auto past_end = cat::try_read_at(arr, cat::idx{4u});
   cat::verify(!past_end.has_value());

   auto far_past = cat::try_read_at(arr, cat::idx{1'000u});
   cat::verify(!far_past.has_value());
}

test(flux_slice_basic) {
   tiny_array<int, 6u> arr = {
      {},
      {10, 20, 30, 40, 50, 60}
   };

   // Slice [1, 4) gives {20, 30, 40}, sum = 90.
   auto sub = cat::slice(arr, cat::idx{1u}, cat::idx{4u});
   cat::verify(cat::sum(sub) == 90);

   // Slices preserve refinement: a slice of a random-access collection is
   // itself random access.
   static_assert(cat::is_random_access_collection<decltype(sub)>);

   // Slices compose with adaptors via the pipe.
   int result = sub
                | cat::transform([](int x) -> int {
                     return x + 1;
                  })
                | cat::sum();
   cat::verify(result == 93);  // 21 + 31 + 41

   // Empty slice is valid and yields zero.
   auto empty = cat::slice(arr, cat::idx{2u}, cat::idx{2u});
   cat::verify(cat::sum(empty) == 0);
}

test(flux_as_span) {
   tiny_array<int, 4u> arr = {
      {},
      {7, 14, 21, 28}
   };

   auto span_view = cat::as_span(arr);
   cat::verify(span_view.size() == 4u);
   cat::verify(span_view[0] == 7);
   cat::verify(span_view[3] == 28);

   // The span aliases the original storage - mutating through the span is
   // visible in `arr`.
   span_view[1] = 99;
   cat::verify(arr.m_data[1] == 99);
}

test(flux_ownership_policy) {
   // A `noisy_box` l-value cannot be piped directly - the adaptor would have to
   // copy it, and copying is deleted. `cat::ref` provides the explicit borrow.
   noisy_box box{
      tiny_array<int, 4u>{{}, {2, 4, 6, 8}}
   };

   int total = cat::ref(box) | cat::sum();
   cat::verify(total == 20);

   // Adaptors compose through the borrow normally.
   int doubled = cat::ref(box)
                 | cat::transform([](int x) -> int {
                      return x * 2;
                   })
                 | cat::sum();
   cat::verify(doubled == 40);

   // Static check that the adaptor closure rejects a bare `noisy_box&`
   // outright. We can't trigger SFINAE inside a `test()` body in a way the
   // compile-time framework would catch, but the ownership-policy assertions
   // further up already cover the structural bit.

   // An r-value `noisy_box` is fine: the adaptor moves it.
   int from_rvalue =
      noisy_box{
         tiny_array<int, 4u>{{}, {1, 2, 3, 4}}
   }
      | cat::sum();
   cat::verify(from_rvalue == 10);
}

test(flux_fluent_terminals) {
   // Every terminal is reachable via the member form.
   tiny_array<int, 4u> arr = {
      {},
      {2, 4, 6, 8}
   };

   cat::verify(arr.sum() == 20);
   cat::verify(arr.count() == 4u);

   int total = 0;
   arr.for_each([&total](int const& x) {
      total += x;
   });
   cat::verify(total == 20);

   int product = arr.fold(
      [](int acc, int x) -> int {
         return acc * x;
      },
      1);
   cat::verify(product == 384);  // 2 * 4 * 6 * 8
}

test(flux_fluent_chain) {
   // The fluent surface is the dual of the pipe surface: same adaptors, same
   // closures, same ownership policy. This test mirrors `flux_pipe_collection`
   // but spelled fluently.
   tiny_array<int, 6u> arr = {
      {},
      {1, 2, 3, 4, 5, 6}
   };

   int result = arr.filter([](int x) -> bool {
                      return (x % 2) == 0;
                   })
                   .transform([](int x) -> int {
                      return x * x;
                   })
                   .sum();
   cat::verify(result == 56);  // 4 + 16 + 36

   // `take` is also fluent.
   int first_three = arr.take(3u).sum();
   cat::verify(first_three == 6);  // 1 + 2 + 3
}

test(flux_fluent_reverse) {
   tiny_array<int, 5u> arr = {
      {},
      {1, 2, 3, 4, 5}
   };

   // A bidirectional collection auto-derives reverse, and the fluent
   // `.reverse()` reads it back to back with the rest of the chain.
   int last_two = arr.reverse().take(2u).sum();
   cat::verify(last_two == 9);  // 5 + 4

   // `tiny_list` opts into reverse via a member `reverse_iterate()`, and the
   // fluent form picks that up identically.
   tiny_list<int, 8u> list;
   list.push_back(10);
   list.push_back(20);
   list.push_back(30);
   list.push_back(40);

   int reversed_doubled = list.reverse()
                             .transform([](int x) -> int {
                                return x * 2;
                             })
                             .sum();
   cat::verify(reversed_doubled == 200);  // 80 + 60 + 40 + 20
}

test(flux_fluent_through_ref) {
   // The ownership policy applies to the fluent surface in exactly the same
   // way: a non-trivially-copyable l-value has to come in through
   // `cat::ref(x)`, but once it does, the same chain composes.
   noisy_box box{
      tiny_array<int, 4u>{{}, {3, 6, 9, 12}}
   };

   int doubled_sum = cat::ref(box)
                        .transform([](int x) -> int {
                           return x * 2;
                        })
                        .sum();
   cat::verify(doubled_sum == 60);  // (3 + 6 + 9 + 12) * 2

   // And the r-value case stays working.
   int sum_inline = noisy_box{
      tiny_array<int, 4u>{
                          {},
                          {1, 1, 1, 1}}
   }.sum();
   cat::verify(sum_inline == 4);
}

test(flux_internal_iterate_collection) {
   tiny_array<int, 5u> arr = {
      {},
      {1, 2, 3, 4, 5}
   };
   int total = 0;
   cat::for_each(arr, [&total](int x) {
      total += x;
   });
   cat::verify(total == 15);
}

test(flux_internal_iterate_iterable_only) {
   tiny_list<int, 8u> list;
   list.push_back(2);
   list.push_back(3);
   list.push_back(5);
   list.push_back(7);

   int total = 0;
   cat::for_each(cat::cref(list), [&total](int x) {
      total += x;
   });
   cat::verify(total == 17);
}

test(flux_internal_iterate_moved_collection) {
   tiny_array<int, 3u> arr = {
      {},
      {1, 2, 3}
   };
   int total = 0;
   cat::for_each(cat::move(arr), [&total](int x) {
      total += x;
   });
   cat::verify(total == 6);
}

test(flux_internal_iterate_chain) {
   tiny_array<int, 6u> arr = {
      {},
      {1, 2, 3, 4, 5, 6}
   };
   int total = 0;
   (arr
    | cat::filter([](int x) -> bool {
         return (x % 2) == 0;
      })
    | cat::transform([](int x) -> int {
         return x * 10;
      }))
      .for_each([&total](int x) {
         total += x;
      });
   cat::verify(total == 120);  // 20 + 40 + 60
}

test(flux_internal_iterate_pipeline_transform) {
   tiny_array<int, 3u> arr = {
      {},
      {0, 1, 2}
   };
   int total = 0;
   (cat::move(arr) | cat::transform([](int v) -> int {
       return v * 2;
    }))
      .for_each([&total](int x) {
         total += x;
      });
   cat::verify(total == 6);
}

test(flux_internal_iterate_empty_filter) {
   tiny_array<int, 4u> arr = {
      {},
      {0, 0, 0, 0}
   };
   int iterations = 0;
   (arr | cat::filter([](int) -> bool {
       return false;
    }))
      .for_each([&](int x) {
         static_cast<void>(x);
         ++iterations;
      });
   cat::verify(iterations == 0);
}

test(flux_fluent_on_slice) {
   // A `slice_view` also inherits `iterable_interface`, so a slice can keep
   // using the member-call form without dropping back to the pipe.
   tiny_array<int, 6u> arr = {
      {},
      {10, 20, 30, 40, 50, 60}
   };

   int from_slice = cat::slice(arr, cat::idx{1u}, cat::idx{5u})
                       .filter([](int x) -> bool {
                          return x != 30;
                       })
                       .sum();
   cat::verify(from_slice == 110);  // 20 + 40 + 50
}

// === P3725 demo: `filter | reverse | as_rvalue | to<vector>` ================
//
// This is the example Tristan Brindle showed on slide 22 of "Faster, Safer,
// Better Ranges" (C++ on Sea 2025), via Nico Josuttis's P3725:
//
//    auto sub = cities | std::views::filter(large)
//                      | std::views::reverse
//                      | std::views::as_rvalue
//                      | std::ranges::to<std::vector>();
//
// In `std::views`, this pipeline is silently broken: reverse iteration over a
// filter_view can step `--it` past the underlying range's begin and read
// arbitrary heap data. The expected result is the long strings in reverse
// order. The actual result is undefined behaviour.
//
// The Flux v2 equivalent is structurally safe. `filter_adaptor` propagates
// `reverse_iterable` from its base, but its reverse pass is internal iteration
// over the *base's* reverse context, which owns its own bounds and stops at
// `complete`. There is no `--iter` that can run off the front of the source.

test(flux_p3725_filter_reverse_as_rvalue_to) {
   tiny_array<tiny_string, 4u> cities{
      {},
      {tiny_string{"Amsterdam"}, tiny_string{"Berlin"}, tiny_string{"Cologne"},
       tiny_string{"LA"}}
   };

   auto large = [](tiny_string const& s) -> bool {
      return s.size() > 5u;
   };

   auto sub = cat::ref(cities)
              | cat::filter(large)
              | cat::reverse()
              | cat::as_rvalue()
              | cat::to<tiny_vector<tiny_string, 4u>>();

   // Three long strings, in reverse order, materialised exactly once.
   cat::verify(sub.size() == 3u);
   cat::verify(sub.m_data[0] == tiny_string{"Cologne"});
   cat::verify(sub.m_data[1] == tiny_string{"Berlin"});
   cat::verify(sub.m_data[2] == tiny_string{"Amsterdam"});

   // The matching cities have been moved out of - the destination owns their
   // bytes now. The non-match ("LA") is untouched. This is what the std::views
   // version aspires to and silently fails to do.
   cat::verify(cities.m_data[0].m_moved_from);   // Amsterdam
   cat::verify(cities.m_data[1].m_moved_from);   // Berlin
   cat::verify(cities.m_data[2].m_moved_from);   // Cologne
   cat::verify(!cities.m_data[3].m_moved_from);  // LA

   // The destination strings are not flagged as moved-from: they were
   // move-constructed *from* a not-moved-from source, so the flag at the time
   // of the move was clean.
   cat::verify(!sub.m_data[0].m_moved_from);
   cat::verify(!sub.m_data[1].m_moved_from);
   cat::verify(!sub.m_data[2].m_moved_from);
}

// === TPOIASI: `transform | filter` invokes the transform once per element ====
//
// Jonathan Boccara coined "The Terrible Problem Of Incrementing A Smart
// Iterator" on Fluent C++:
// https://www.fluentcpp.com/2019/02/12/the-terrible-problem-of-incrementing-a-smart-iterator/
//
// In range-v3 / std::views, the chain
//   numbers | transform(times2) | filter(isMultipleOf4)
// invokes `times2` *seven* times for a five-element input. The filter
// iterator's `operator++` has to peek at the next element to decide
// where to stop (one call to `times2` for the peek), and then a
// subsequent `operator*` re-reads the same position (a second call).
// For elements that pass the predicate, `times2` therefore runs twice.
// For those that don't, it runs once.
//
// Flux v2 fuses read and advance into a single `run_while` dispatch: the base
// context yields one element, the transform lambda runs once, the filter lambda
// checks the predicate, and either the terminal callback runs or the element is
// dropped. There is no peek/read split, so each lambda runs exactly once per
// source element no matter how deep the adaptor stack is.
test(flux_tpoiasi_transform_filter_no_double_invocation) {
   tiny_array<int, 5u> numbers = {
      {},
      {1, 2, 3, 4, 5}
   };

   int times2_calls = 0;
   int predicate_calls = 0;

   auto times2 = [&times2_calls](int n) -> int {
      ++times2_calls;
      return n * 2;
   };

   auto is_multiple_of_4 = [&predicate_calls](int n) -> bool {
      ++predicate_calls;
      return (n % 4) == 0;
   };

   auto results = numbers
                  | cat::transform(times2)
                  | cat::filter(is_multiple_of_4)
                  | cat::to<tiny_vector<int, 8u>>();

   // {1,2,3,4,5} -> times2 -> {2,4,6,8,10} -> %4 == 0 -> {4, 8}.
   cat::verify(results.size() == 2u);
   cat::verify(results.m_data[0] == 4);
   cat::verify(results.m_data[1] == 8);

   // Five base elements, five transform invocations - not seven, as range-v3
   // reports.
   cat::verify(times2_calls == 5);
   cat::verify(predicate_calls == 5);
}

// `transform | filter` and `filter | transform` produce the same elements, but
// in the latter ordering range-v3's filter iterator does not have to peek
// through the transform, so TPOIASI does not bite. Pin that the count is
// identical in Flux v2 either way.
test(flux_tpoiasi_filter_transform_invocation_count) {
   tiny_array<int, 5u> numbers = {
      {},
      {1, 2, 3, 4, 5}
   };

   int times2_calls = 0;
   int predicate_calls = 0;

   auto times2 = [&times2_calls](int n) -> int {
      ++times2_calls;
      return n * 2;
   };

   // Pick out the source elements whose doubled value is a multiple of 4 (i.e.
   // the even numbers): {2, 4} -> doubled -> {4, 8}.
   auto is_even = [&predicate_calls](int n) -> bool {
      ++predicate_calls;
      return (n % 2) == 0;
   };

   auto results = numbers
                  | cat::filter(is_even)
                  | cat::transform(times2)
                  | cat::to<tiny_vector<int, 8u>>();

   cat::verify(results.size() == 2u);
   cat::verify(results.m_data[0] == 4);
   cat::verify(results.m_data[1] == 8);

   // Predicate runs once per source element. Transform only runs for the
   // elements the filter let through. Neither lambda is double- invoked.
   cat::verify(predicate_calls == 5);
   cat::verify(times2_calls == 2);
}

// `as_rvalue` rebrands element_type as an xvalue regardless of whether the
// incoming iterable is forward or reverse, and regardless of which adaptor it
// is composed with. Pin the structural pieces.
test(flux_as_rvalue_element_type) {
   tiny_array<tiny_string, 2u> arr;
   using fwd_chain = decltype(cat::ref(arr) | cat::as_rvalue());
   using fwd_ctx = cat::iterable_iteration_context_type<fwd_chain>;
   static_assert(cat::is_same<typename fwd_ctx::element_type, tiny_string&&>);

   using rev_chain =
      decltype(cat::ref(arr) | cat::reverse() | cat::as_rvalue());
   using rev_ctx = cat::iterable_iteration_context_type<rev_chain>;
   static_assert(cat::is_same<typename rev_ctx::element_type, tiny_string&&>);

   // `filter` is now reverse-iterable when its base is, so the P3725-shaped
   // chain has a well-formed reverse context too.
   static_assert(cat::is_reverse_iterable<
                 cat::reference_wrapper<tiny_array<tiny_string, 2u>>>);
}

// Pipe terminal closures mirror the free algorithms (`sum`, `count`, etc.) but
// participate in the same ownership policy as adaptors.
test(flux_pipe_terminal_closures) {
   tiny_array<int, 4u> arr = {
      {},
      {2, 4, 6, 8}
   };

   cat::verify((arr | cat::sum()) == 20);
   cat::verify((arr | cat::count()) == 4u);

   struct tally {
      int n = 0;

      constexpr void
      operator()(int x) {
         n += x;
      }
   };

   tally body{};
   tally returned = arr | cat::for_each(body);
   cat::verify(returned.n == 20);

   int piped_fold = arr
                    | cat::fold(
                       [](int acc, int x) -> int {
                          return acc + (x * x);
                       },
                       0);
   cat::verify(piped_fold == 4 + 16 + 36 + 64);
}

// `to<Container>()` materialises into the destination through its `push_back`
// (here `tiny_vector::push_back`) without needing `as_rvalue` when the source
// elements are cheap to copy.
test(flux_to_without_as_rvalue) {
   tiny_array<int, 4u> arr = {
      {},
      {1, 2, 3, 4}
   };
   auto out = arr | cat::to<tiny_vector<int, 8u>>();
   cat::verify(out.size() == 4u);
   cat::verify(out.m_data[0] == 1);
   cat::verify(out.m_data[3] == 4);
}

// A `slice_view` keeps the parent's random-access protocol, so `distance` and
// `reverse` compose like on the underlying collection.
test(flux_slice_reverse_and_distance) {
   tiny_array<int, 4u> arr = {
      {},
      {10, 100, 1'000, 10'000}
   };
   auto sub = cat::slice(arr, cat::idx{0u}, cat::idx{3u});
   cat::verify(sub.distance(cat::idx{0u}, cat::idx{3u}) == cat::iword{3});

   int rev_two = sub | cat::reverse() | cat::take(2u) | cat::sum();
   cat::verify(rev_two == 1'100);  // 1'000 + 100
}

// `const` collections still model `is_collection` because the Flux position
// hooks are `const`-qualified on the CRTP object handle.
test(flux_iterate_const_collection) {
   tiny_array<int, 3u> const arr = {
      {},
      {3, 4, 5}
   };
   cat::verify(cat::sum(arr) == 12);
   cat::verify((arr
                | cat::filter([](int x) -> bool {
                     return x != 4;
                  })
                | cat::sum())
               == 8);
}

// Structural checks: iteration contexts are non-copyable and expose
// `element_type`. Element/value aliases match the context.
test(flux_iteration_context_and_element_traits) {
   using arr4 = tiny_array<int, 4u>;
   static_assert(
      cat::is_iteration_context<cat::collection_iteration_context<arr4>>);
   static_assert(cat::is_same<cat::iterable_element_type<arr4>, int&>);
   static_assert(cat::is_same<cat::iterable_value_type<arr4>, int>);

   using list8 = tiny_list<int, 8u>;
   static_assert(cat::is_same<cat::iterable_value_type<list8>, int>);
}
