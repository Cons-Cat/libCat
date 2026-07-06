#include <cat/dyn>
#include <cat/linear_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

namespace {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
inline constinit cat::idx destruct_count = 0u;
inline constinit cat::idx copy_count = 0u;
inline constinit cat::idx move_count = 0u;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void
reset_counters() {
   destruct_count = 0u;
   copy_count = 0u;
   move_count = 0u;
}

enum class voice : unsigned char {
   meow,
   woof,
};

// A small, well-behaved pet who fits inside the inline storage buffer.
struct kitty {
   cat::int4 treats;

   constexpr kitty() = default;

   constexpr kitty(cat::int4 v) : treats(v) {
   }

   constexpr kitty(kitty const& other) : treats(other.treats) {
      if !consteval {
         ++copy_count;
      }
   }

   constexpr kitty(kitty&& other) noexcept : treats(other.treats) {
      other.treats = 0;
      if !consteval {
         ++move_count;
      }
   }

   constexpr ~kitty() {
      if !consteval {
         ++destruct_count;
      }
   }

   [[nodiscard]]
   constexpr auto
   say() const -> voice {
      return voice::meow;
   }

   [[nodiscard]]
   constexpr auto
   treat_count() const -> cat::int4 {
      return treats;
   }

   constexpr void
   feed(cat::int4 amount) {
      treats = treats + amount;
   }
};

// A roomy pet who drags around enough toys to overflow the inline buffer.
struct puppy {
   cat::int8 toys[16];
   cat::int4 treats;

   constexpr puppy() : toys{}, treats(0) {
   }

   constexpr puppy(cat::int4 v) : toys{}, treats(v) {
      toys[0] = v;
   }

   constexpr puppy(puppy const& other) : toys{}, treats(other.treats) {
      for (cat::idx i = 0u; i < 16u; ++i) {
         toys[i] = other.toys[i];
      }
      if !consteval {
         ++copy_count;
      }
   }

   constexpr puppy(puppy&& other) noexcept : toys{}, treats(other.treats) {
      for (cat::idx i = 0u; i < 16u; ++i) {
         toys[i] = other.toys[i];
         other.toys[i] = 0;
      }
      other.treats = 0;
      if !consteval {
         ++move_count;
      }
   }

   constexpr ~puppy() {
      if !consteval {
         ++destruct_count;
      }
   }

   [[nodiscard]]
   constexpr auto
   say() const -> voice {
      return voice::woof;
   }

   [[nodiscard]]
   constexpr auto
   treat_count() const -> cat::int4 {
      return treats;
   }

   constexpr void
   feed(cat::int4 amount) {
      treats = treats + amount;
   }
};

// Method: returns the animal's voice (meow or woof).
struct say {
   using signature = voice() const;

   template <typename T>
      requires(requires(T const& t) { t.say(); })
   static constexpr auto
   do_invoke(T const& self) -> voice {
      return self.say();
   }
};

// Method: returns how many treats the animal has been fed. Constrains its
// `do_invoke` so types that lack `.treat_count()` are rejected.
struct treat_count {
   using signature = cat::int4() const;

   template <typename T>
      requires(requires(T const& t) { t.treat_count(); })
   static constexpr auto
   do_invoke(T const& self) -> cat::int4 {
      return self.treat_count();
   }
};

// Method: feeds the animal `amount` treats.
struct feed {
   using signature = void(cat::int4);

   template <typename T>
      requires(requires(T& t, cat::int4 v) { t.feed(v); })
   static constexpr void
   do_invoke(T& self, cat::int4 amount) {
      self.feed(amount);
   }
};

// A Method that only accepts kitties.
struct only_for_kitty {
   using signature = void();

   template <cat::is_same<kitty> T>
   static void
   do_invoke(T& /*self*/) {
   }
};

// A speechless type that does NOT model `say` (no `.say()` method).
struct rock {
   cat::int4 weight;
   ~rock() = default;
};

// A roomy pet with enough toys to overflow `64u` and
// force `basic_dyn` onto the heap path.
struct bear {
   cat::int8 toys[40];
   cat::int4 treats;

   constexpr bear() : toys{}, treats(0) {
   }

   constexpr bear(cat::int4 v) : toys{}, treats(v) {
      toys[0] = v;
   }

   constexpr bear(bear const& other) : toys{}, treats(other.treats) {
      for (cat::idx i = 0u; i < 40u; ++i) {
         toys[i] = other.toys[i];
      }
      ++copy_count;
   }

   constexpr bear(bear&& other) noexcept : toys{}, treats(other.treats) {
      for (cat::idx i = 0u; i < 40u; ++i) {
         toys[i] = other.toys[i];
         other.toys[i] = 0;
      }
      other.treats = 0;
      ++move_count;
   }

   ~bear() {
      ++destruct_count;
   }

   [[nodiscard]]
   constexpr auto
   treat_count() const -> cat::int4 {
      return treats;
   }

   void
   feed(cat::int4 amount) {
      treats = treats + amount;
   }
};

static_assert(sizeof(bear) > 256);

// Method-bundle aliases used across the dyn tests. The naming spells out the
// participating methods after a `dyn_` (in-place) or `dyn_` (heap
// fallback through `linear_allocator`) prefix. `cat::destructor` is implied.
using dyn_treat_count = cat::dyn_inplace<cat::destructor, treat_count>;
using dyn_treat_count_feed =
   cat::dyn_inplace<cat::destructor, treat_count, feed>;
using dyn_say = cat::dyn_inplace<cat::destructor, say>;
using dyn_move_constructor_treat_count =
   cat::dyn_inplace<cat::destructor, cat::move_constructor, treat_count>;
using dyn_move_constructor_copy_constructor_treat_count = cat::dyn_inplace<
   cat::destructor, cat::move_constructor, cat::copy_constructor, treat_count>;

using dyn_alloc_treat_count =
   cat::dyn<cat::linear_allocator, cat::destructor, treat_count>;
using dyn_alloc_move_constructor_treat_count = cat::dyn<
   cat::linear_allocator, cat::destructor, cat::move_constructor, treat_count>;

// A `basic_dyn` whose inline buffer is sized big enough to hold a
// `puppy` (which would spill to the heap with the default
// `64u`).
using dyn_alloc_big_treat_count = cat::basic_dyn<
   cat::linear_allocator, sizeof(puppy), cat::destructor, treat_count>;

// A deliberately over-aligned type used to exercise the SBO alignment guard.
struct alignas(256) over_aligned_kitty {
   cat::int4 treats = 0;

   [[nodiscard]]
   constexpr auto
   treat_count() const -> cat::int4 {
      return treats;
   }
};

}  // namespace

// Construction from a value, via deduced-T constructor.
$test(dyn_construct_value) {
   reset_counters();

   {
      dyn_treat_count dyn{kitty{7}};
      cat::verify(dyn.has_value());
      cat::verify(cat::dyn_invoke<treat_count>(dyn) == 7);
      cat::verify(dyn.type_descriptor() == cat::p_dyn_type_id_for<kitty>);
   }

   // Destructor called once for the held value, plus moves into storage.
   cat::verify(destruct_count >= 1u);
}

// In-place construction with `in_place_for<T>`.
$test(dyn_construct_in_place) {
   reset_counters();

   dyn_treat_count dyn{cat::in_place_for<kitty>, 42_i4};
   cat::verify(dyn.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 42);
   // No copy or move needed for in-place construction.
   cat::verify(copy_count == 0u);
   cat::verify(move_count == 0u);
}

// Default-constructed dyn is empty, then can be populated via `emplace`.
$test(dyn_construct_empty) {
   dyn_move_constructor_treat_count dyn;
   cat::verify(!dyn.has_value());
   cat::verify(dyn.type_descriptor() == cat::p_dyn_type_id_for<void>);

   dyn.emplace<kitty>(99_i4);
   cat::verify(dyn.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 99);
}

// `basic_dyn` keeps small values inline and falls back to the heap when
// the value exceeds `64u`.
$test(dyn_inline_vs_heap) {
   cat::page_allocator local_pager;
   cat::span page = local_pager.alloc_multi<cat::byte>(8_uki).or_exit();
   $defer {
      local_pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   reset_counters();

   // Small value is inline (its address moves with the dyn). The byte count
   // tracks `sizeof(T)`.
   {
      dyn_alloc_treat_count dyn{allocator, kitty{1}};
      cat::verify(!dyn.is_stable_pointers());
      cat::verify(dyn.size_bytes() == sizeof(kitty));
      cat::verify(cat::dyn_invoke<treat_count>(dyn) == 1);
   }

   // Oversized value spills to the allocator and the held address is stable.
   {
      dyn_alloc_treat_count dyn{allocator, bear{2}};
      cat::verify(dyn.is_stable_pointers());
      cat::verify(dyn.size_bytes() == sizeof(bear));
      cat::verify(cat::dyn_invoke<treat_count>(dyn) == 2);
   }
}

// A `basic_dyn` with a custom `inline_storage_size` keeps a `puppy`
// inline that the default-sized variant would have spilled to the heap.
$test(dyn_custom_inline_storage_size) {
   static_assert(dyn_alloc_treat_count::inline_size == 64u);
   static_assert(dyn_alloc_big_treat_count::inline_size == sizeof(puppy));
   static_assert(sizeof(puppy) > 64u);

   cat::page_allocator local_pager;
   cat::span page = local_pager.alloc_multi<cat::byte>(8_uki).or_exit();
   $defer {
      local_pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   // The default-sized dyn spills `puppy` to the heap.
   {
      dyn_alloc_treat_count dyn{allocator, puppy{7}};
      cat::verify(dyn.is_stable_pointers());
      cat::verify(cat::dyn_invoke<treat_count>(dyn) == 7);
   }

   // The big-buffer dyn keeps the same `puppy` inline.
   {
      dyn_alloc_big_treat_count dyn{allocator, puppy{9}};
      cat::verify(!dyn.is_stable_pointers());
      cat::verify(dyn.size_bytes() == sizeof(puppy));
      cat::verify(cat::dyn_invoke<treat_count>(dyn) == 9);
   }

   // A `bear` still overflows the larger buffer and goes to the heap.
   {
      dyn_alloc_big_treat_count dyn{allocator, bear{11}};
      cat::verify(dyn.is_stable_pointers());
      cat::verify(cat::dyn_invoke<treat_count>(dyn) == 11);
   }
}

// `fits_inline` rejects values whose alignment exceeds the SBO buffer.
// Properly aligned values are accepted regardless of where they end up
// (inline or heap for `basic_dyn`, always inline for `dyn_inplace`).
$test(dyn_alignment_guard) {
   // Sanity-check the exposed alignment constants.
   static_assert(dyn_treat_count::storage_alignment == 16u);
   static_assert(
      dyn_alloc_treat_count::storage_alignment >= alignof(cat::iword)
   );

   // Standard-aligned types satisfy `fits_inline` on the in-place dyn when
   // they fit the size budget too.
   static_assert(dyn_treat_count::fits_inline<kitty>);
   static_assert(!dyn_treat_count::fits_inline<bear>);

   // A type whose alignment exceeds `16u` is rejected
   // regardless of its size.
   static_assert(alignof(over_aligned_kitty) > 16u);
   static_assert(!dyn_treat_count::fits_inline<over_aligned_kitty>);

   // The same guard applies to `basic_dyn`. Standard-aligned `kitty` fits
   // inline, the over-aligned variant does not.
   static_assert(dyn_alloc_treat_count::fits_inline<kitty>);
   static_assert(!dyn_alloc_treat_count::fits_inline<over_aligned_kitty>);
}

// `cat::movable` enables move construction. The value relocates correctly.
$test(dyn_move_construct) {
   reset_counters();

   dyn_move_constructor_treat_count a{kitty{5}};
   cat::idx moves_before = move_count;

   auto b = cat::move(a);
   cat::verify(b.has_value());
   cat::verify(!a.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(b) == 5);
   cat::verify(move_count > moves_before);
}

// `cat::copyable` enables copy construction.
$test(dyn_copy_construct) {
   reset_counters();

   dyn_move_constructor_copy_constructor_treat_count a{kitty{11}};
   cat::idx copies_before = copy_count;

   auto b = a;
   cat::verify(a.has_value());
   cat::verify(b.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(a) == 11);
   cat::verify(cat::dyn_invoke<treat_count>(b) == 11);
   cat::verify(copy_count > copies_before);
}

// Move-assignment swaps in another value, destroying the previous one.
$test(dyn_move_assign) {
   dyn_move_constructor_treat_count a{kitty{1}};
   dyn_move_constructor_treat_count b{kitty{2}};
   a = cat::move(b);
   cat::verify(a.has_value());
   cat::verify(!b.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(a) == 2);
}

// Copy-assignment requires both `movable` and `copyable`.
$test(dyn_copy_assign) {
   dyn_move_constructor_copy_constructor_treat_count a{kitty{3}};
   dyn_move_constructor_copy_constructor_treat_count b{kitty{4}};
   a = b;
   cat::verify(cat::dyn_invoke<treat_count>(a) == 4);
   cat::verify(cat::dyn_invoke<treat_count>(b) == 4);
}

// `emplace<T>` replaces the held value with a new construction in-place.
$test(dyn_emplace) {
   dyn_treat_count_feed dyn{kitty{0}};

   kitty& ref = dyn.emplace<kitty>(50_i4);
   cat::verify(ref.treats == 50);
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 50);

   cat::dyn_invoke<feed>(dyn, 7_i4);
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 57);

   // The same flow at compile time. This additionally exercises the
   // `if consteval` fallback in `dyn_inplace`'s `emplace`/`destroy_held`:
   // at constant evaluation the held `T` lives on the heap (P2747R2 forbids
   // placement-new into the inline `unsigned char` buffer for non-char `T`)
   // and a typed disposer in the storage union runs `delete` from the
   // destructor.
   static_assert([] consteval {
      dyn_treat_count_feed dyn{kitty{0}};
      dyn.emplace<kitty>(50_i4);
      cat::dyn_invoke<feed>(dyn, 7_i4);
      return cat::dyn_invoke<treat_count>(dyn);
   }() == 57);
}

// `reset` empties the dyn and runs the held value's destructor.
$test(dyn_reset) {
   reset_counters();

   dyn_treat_count dyn{kitty{17}};
   cat::verify(dyn.has_value());
   cat::idx before = destruct_count;

   dyn.reset();
   cat::verify(!dyn.has_value());
   cat::verify(destruct_count > before);
}

// `dyn_ref` non-owning view, dispatch through it.
$test(dyn_ref_basic) {
   kitty v{55};
   cat::dyn_ref<treat_count, feed> ref{v};
   cat::verify(ref.p_type_id() == cat::p_dyn_type_id_for<kitty>);
   cat::verify(cat::dyn_invoke<treat_count>(ref) == 55);
   cat::dyn_invoke<feed>(ref, 3_i4);
   cat::verify(cat::dyn_invoke<treat_count>(ref) == 58);
   cat::verify(v.treats == 58);

   // The same dispatch path at compile time. If the trampoline through
   // `vtable_for<T, Methods...>` did not constant-fold to a direct call on
   // `Method::do_invoke<T>`, this `static_assert` would not compile.
   //
   // `dyn_ref` subset-narrowing (`dyn_ref<wide...>` to `dyn_ref<narrow...>`)
   // is intentionally not exercised: `detail::narrow_vtable` reads the
   // narrower vtable out of the wider one via `reinterpret_cast`, which is
   // not a constant expression even in C++26.
   static_assert([] consteval {
      kitty v{55};
      cat::dyn_ref<treat_count, feed> ref{v};
      cat::dyn_invoke<feed>(ref, 3_i4);
      return cat::dyn_invoke<treat_count>(ref);
   }() == 58);
}

// `const_dyn_ref` only allows const Methods.
$test(dyn_const_ref_basic) {
   kitty const v{99};
   cat::const_dyn_ref<treat_count> cref{v};
   cat::verify(cref.p_type_id() == cat::p_dyn_type_id_for<kitty>);
   cat::verify(cat::dyn_invoke<treat_count>(cref) == 99);

   // Conversion from non-const dyn_ref to const_dyn_ref.
   kitty mut_v{77};
   cat::dyn_ref<treat_count> ref{mut_v};
   cat::const_dyn_ref<treat_count> from_mut = ref;
   cat::verify(cat::dyn_invoke<treat_count>(from_mut) == 77);
}

// `dyn_ptr` is nullable and convertible from a raw pointer.
$test(dyn_ptr_basic) {
   cat::dyn_ptr<treat_count, feed> empty;
   cat::verify(!empty.has_value());
   cat::verify(empty == nullptr);
   cat::verify(empty.type_id_ptr() == cat::p_dyn_type_id_for<void>);

   kitty v{12};
   cat::dyn_ptr<treat_count, feed> ptr{&v};
   cat::verify(ptr.has_value());
   cat::verify(ptr != nullptr);
   cat::verify(ptr.type_id_ptr() == cat::p_dyn_type_id_for<kitty>);

   auto ref = *ptr;
   cat::verify(cat::dyn_invoke<treat_count>(ref) == 12);
   cat::dyn_invoke<feed>(*ptr, 8_i4);
   cat::verify(v.treats == 20);
}

// `const_dyn_ptr` mirrors `dyn_ptr` but for const access.
$test(dyn_const_ptr_basic) {
   cat::const_dyn_ptr<treat_count> empty;
   cat::verify(!empty.has_value());

   kitty v{33};
   cat::const_dyn_ptr<treat_count> cptr{&v};
   cat::verify(cptr.has_value());
   cat::verify(cptr.type_descriptor() == cat::p_dyn_type_id_for<kitty>);
   cat::verify(cat::dyn_invoke<treat_count>(*cptr) == 33);

   // Convert from a dyn_ptr.
   cat::dyn_ptr<treat_count> p{&v};
   cat::const_dyn_ptr<treat_count> cp = p;
   cat::verify(cp.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(*cp) == 33);
}

// `cat::dyn_cast<T>(holder)` returns a typed pointer or nullptr.
$test(dyn_cast) {
   dyn_treat_count dyn{kitty{121}};

   kitty* p_small = cat::dyn_cast<kitty>(&dyn);
   cat::verify(p_small != nullptr);
   cat::verify(p_small->treats == 121);

   // Type mismatch returns nullptr.
   puppy* p_large = cat::dyn_cast<puppy>(&dyn);
   cat::verify(p_large == nullptr);

   // null pointer in -> null pointer out.
   decltype(&dyn) null_dyn = nullptr;
   cat::verify(cat::dyn_cast<kitty>(null_dyn) == nullptr);

   // Through `dyn_ptr`.
   cat::dyn_ptr<cat::destructor, treat_count> ptr = &dyn;
   kitty* p_small2 = cat::dyn_cast<kitty>(ptr);
   cat::verify(p_small2 != nullptr);
   cat::verify(p_small2 == p_small);
   cat::verify(cat::dyn_cast<puppy>(ptr) == nullptr);

   // Through const_dyn_ref.
   cat::const_dyn_ref<treat_count> cref{*p_small};
   kitty const* p_csmall = cat::dyn_cast<kitty>(cref);
   cat::verify(p_csmall != nullptr);
   cat::verify(p_csmall->treats == 121);
   cat::verify(cat::dyn_cast<puppy>(cref) == nullptr);
}

// `operator&` on a polymorphic holder returns a `dyn_ptr` (or const variant).
$test(dyn_addressof_yields_ptr) {
   dyn_treat_count dyn{kitty{8}};

   cat::dyn_ptr<cat::destructor, treat_count> p = &dyn;
   cat::verify(p.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(*p) == 8);

   auto const& cdyn = dyn;
   cat::const_dyn_ptr<cat::destructor, treat_count> cp = &cdyn;
   cat::verify(cp.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(*cp) == 8);
}

// Multiple Methods on a single dyn all dispatch correctly.
$test(dyn_multiple_methods) {
   cat::dyn_inplace<
      cat::destructor, cat::move_constructor, cat::copy_constructor,
      treat_count, feed>
      dyn{kitty{0}};

   cat::dyn_invoke<feed>(dyn, 10_i4);
   cat::dyn_invoke<feed>(dyn, 5_i4);
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 15);
}

// Type descriptor stability: two objects of the same type compare equal,
// objects of different types compare unequal.
$test(dyn_type_descriptors) {
   kitty a{1};
   kitty b{2};
   puppy c{3};

   cat::dyn_ref<treat_count> ra{a};
   cat::dyn_ref<treat_count> rb{b};
   cat::dyn_ref<treat_count> rc{c};

   cat::verify(ra.p_type_id() == rb.p_type_id());
   cat::verify(ra.p_type_id() != rc.p_type_id());
   cat::verify(ra.p_type_id() == cat::p_dyn_type_id_for<kitty>);
   cat::verify(rc.p_type_id() == cat::p_dyn_type_id_for<puppy>);
}

// `cat::dispatch<Method>` works with const-qualified Methods on const holders.
$test(dyn_const_dispatch) {
   dyn_treat_count dyn{kitty{77}};

   auto const& cdyn = dyn;
   cat::verify(cat::dyn_invoke<treat_count>(cdyn) == 77);
}

// Constraints on Method::do_invoke are honored: rock (no .get()) is
// rejected at compile time. We assert that the concept correctly excludes it.
$test(dyn_method_constraints) {
   static_assert(cat::detail::method_supports_type<treat_count, kitty>);
   static_assert(cat::detail::method_supports_type<treat_count, puppy>);
   static_assert(!cat::detail::method_supports_type<treat_count, rock>);
   static_assert(cat::detail::method_supports_type<only_for_kitty, kitty>);
   static_assert(!cat::detail::method_supports_type<only_for_kitty, puppy>);
}

// The vtable slot for a given Method returns a callable, and it is the
// same address for repeated lookups.
$test(dyn_vtable_slots_stable) {
   cat::vtable<treat_count, feed> const* p_vtable =
      cat::p_vtable_for<kitty, treat_count, feed>;
   cat::verify(p_vtable != nullptr);

   auto p_fn1 = p_vtable->fun<treat_count>();
   auto p_fn2 = p_vtable->fun<treat_count>();
   cat::verify(p_fn1 == p_fn2);

   kitty v{200};
   cat::int4 result = p_fn1(static_cast<void const*>(&v));
   cat::verify(result == 200);
}

// `is_stable_pointers` flips depending on inline vs heap storage.
$test(dyn_pointer_stability) {
   cat::page_allocator local_pager;
   cat::span page = local_pager.alloc_multi<cat::byte>(8_uki).or_exit();
   $defer {
      local_pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   dyn_alloc_treat_count heap_dyn{allocator, bear{1}};
   cat::verify(heap_dyn.is_stable_pointers());

   dyn_alloc_treat_count inline_any{allocator, kitty{1}};
   cat::verify(!inline_any.is_stable_pointers());
}

// `make_basic_dyn<Methods...>` deduces from the value.
$test(dyn_make_basic) {
   cat::page_allocator local_pager;
   cat::span page = local_pager.alloc_multi<cat::byte>(4_uki).or_exit();
   $defer {
      local_pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   auto dyn =
      cat::make_basic_dyn<cat::destructor, treat_count>(allocator, kitty{500});
   cat::verify(dyn.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 500);
}

// Heap storage path destroys the held object and frees the heap allocation.
$test(dyn_heap_lifecycle) {
   cat::page_allocator local_pager;
   cat::span page = local_pager.alloc_multi<cat::byte>(8_uki).or_exit();
   $defer {
      local_pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   reset_counters();
   {
      dyn_alloc_treat_count dyn{allocator, bear{42}};
      cat::verify(dyn.is_stable_pointers());
      cat::verify(cat::dyn_invoke<treat_count>(dyn) == 42);
   }
   cat::verify(destruct_count >= 1u);
}

// Move from heap-stored value preserves the heap pointer (no extra moves).
$test(dyn_heap_move_preserves_pointer) {
   cat::page_allocator local_pager;
   cat::span page = local_pager.alloc_multi<cat::byte>(8_uki).or_exit();
   $defer {
      local_pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   dyn_alloc_move_constructor_treat_count a{allocator, bear{99}};
   bear* p_before = cat::dyn_cast<bear>(&a);

   auto b = cat::move(a);
   bear* p_after = cat::dyn_cast<bear>(&b);
   cat::verify(p_before == p_after);
   cat::verify(cat::dyn_invoke<treat_count>(b) == 99);
}

// `methods_list` exposes the configured Methods.
$test(dyn_methods_list) {
   static_assert(cat::is_same<
                 dyn_treat_count_feed::methods,
                 cat::type_list<cat::destructor, treat_count, feed>>);
   static_assert(
      dyn_treat_count_feed::inline_size
      == sizeof(cat::vtable<cat::destructor, treat_count, feed>)
   );
}

// dyn_ref does not allow rebinding via `=`.
$test(dyn_ref_no_rebind) {
   kitty v{1};
   kitty other{2};
   cat::dyn_ref<treat_count> r{v};
   cat::dyn_ref<treat_count> r2{other};
   r = r2;  // same-type assignment is allowed.
   cat::verify(cat::dyn_invoke<treat_count>(r) == 2);
   // Cross-type rebind via `r = other` is rejected by the deleted operator=.
}

// `dyn_ref::rebind` is the explicit form of the cross-type rebind that the
// deleted `operator=(T&&)` directs users towards. It updates all three view
// pointers in one shot, so the next dispatch dispatches on the new value's
// type.
$test(dyn_ref_rebind) {
   kitty cat{7};
   puppy dog{20};

   cat::dyn_ref<treat_count> ref{cat};
   cat::verify(ref.p_type_id() == cat::p_dyn_type_id_for<kitty>);
   cat::verify(cat::dyn_invoke<treat_count>(ref) == 7);

   // Rebind to a different value of the same type.
   kitty other{11};
   ref.rebind(other);
   cat::verify(ref.p_type_id() == cat::p_dyn_type_id_for<kitty>);
   cat::verify(cat::dyn_invoke<treat_count>(ref) == 11);

   // Rebind to a different concrete type that also satisfies `Methods`.
   ref.rebind(dog);
   cat::verify(ref.p_type_id() == cat::p_dyn_type_id_for<puppy>);
   cat::verify(cat::dyn_invoke<treat_count>(ref) == 20);

   // `const_dyn_ref` rebinds the same way.
   kitty const cv1{3};
   kitty const cv2{4};
   cat::const_dyn_ref<treat_count> cref{cv1};
   cat::verify(cat::dyn_invoke<treat_count>(cref) == 3);
   cref.rebind(cv2);
   cat::verify(cat::dyn_invoke<treat_count>(cref) == 4);
}

// Assigning a value via `operator=` uses emplace under the hood.
$test(dyn_assign_value) {
   dyn_treat_count dyn;
   dyn = kitty{15};
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 15);
   dyn = kitty{100};
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 100);
}

// `&dyn_ref` round-trips through `*dyn_ptr`.
$test(dyn_ref_to_ptr_to_ref) {
   kitty v{42};
   cat::dyn_ref<treat_count> ref{v};
   cat::dyn_ptr<treat_count> ptr = &ref;
   cat::verify(ptr.has_value());
   auto ref2 = *ptr;
   cat::verify(cat::dyn_invoke<treat_count>(ref2) == 42);
}

// `dyn_inplace`'s vtable address comes from the object itself, not from a
// shared static, so two distinct dyns hold two distinct vtable addresses.
$test(dyn_inplace_vtable_is_inline) {
   dyn_say a{kitty{1}};
   dyn_say b{kitty{2}};
   cat::dyn_ptr<cat::destructor, say> pa = &a;
   cat::dyn_ptr<cat::destructor, say> pb = &b;
   cat::verify(pa.has_value());
   cat::verify(pb.has_value());
   cat::verify(pa.get() != pb.get());
   cat::verify(cat::dyn_invoke<say>(*pa) == voice::meow);
   cat::verify(cat::dyn_invoke<say>(*pb) == voice::meow);
}

// `inline_size` is deduced from the methods list and scales with the vtable.
$test(dyn_inplace_inline_size) {
   static_assert(
      dyn_say::inline_size == sizeof(cat::vtable<cat::destructor, say>)
   );
   using wide_t = cat::dyn_inplace<
      cat::destructor, cat::move_constructor, cat::copy_constructor, say,
      treat_count, feed>;
   static_assert(wide_t::inline_size > dyn_say::inline_size);

   // The consteval-only disposer pointer lives in a union with the inline
   // storage, so it costs zero runtime bytes. Total layout is just:
   // `inline_size` (storage) + `sizeof(vtable)` + `dyn_type_id_ptr` +
   // `void*`.
   static_assert(
      sizeof(dyn_say)
      == dyn_say::inline_size + sizeof(cat::vtable<cat::destructor, say>)
            + sizeof(cat::dyn_type_id_ptr) + sizeof(void*)
   );
}

// `dyn_ref<Methods...>` narrows to `dyn_ref<Subset...>` when `Subset` is a
// contiguous subset of `Methods`. The narrowed view dispatches the same
// method through the same underlying value.
$test(dyn_ref_narrowing_suffix) {
   kitty cat{5};
   cat::dyn_ref<say, treat_count, feed> wide{cat};
   cat::dyn_ref<treat_count, feed> mid = wide;
   cat::dyn_ref<feed> narrow = wide;

   cat::verify(cat::dyn_invoke<treat_count>(mid) == 5);
   cat::dyn_invoke<feed>(narrow, 4_i4);
   cat::verify(cat::dyn_invoke<treat_count>(mid) == 9);
   cat::verify(cat::dyn_invoke<say>(wide) == voice::meow);
}

// Narrowing from the middle of the Methods pack works because the layout is
// flat.
$test(dyn_ref_narrowing_middle) {
   kitty cat{12};
   cat::dyn_ref<say, treat_count, feed> wide{cat};
   cat::dyn_ref<treat_count> middle = wide;
   cat::verify(cat::dyn_invoke<treat_count>(middle) == 12);
}

// Narrowing from a prefix of the Methods pack also works.
$test(dyn_ref_narrowing_prefix) {
   kitty cat{3};
   cat::dyn_ref<say, treat_count, feed> wide{cat};
   cat::dyn_ref<say, treat_count> head = wide;
   cat::verify(cat::dyn_invoke<say>(head) == voice::meow);
   cat::verify(cat::dyn_invoke<treat_count>(head) == 3);
}

// `const_dyn_ref` narrows the same way as `dyn_ref`.
$test(const_dyn_ref_narrowing) {
   kitty cat{8};
   cat::const_dyn_ref<say, treat_count> wide{cat};
   cat::const_dyn_ref<treat_count> narrow = wide;
   cat::verify(cat::dyn_invoke<treat_count>(narrow) == 8);
}

// A non-const `dyn_ref<Superset...>` narrows to a `const_dyn_ref<Subset...>`.
$test(dyn_ref_to_const_narrowing) {
   kitty cat{6};
   cat::dyn_ref<say, treat_count, feed> wide{cat};
   cat::const_dyn_ref<treat_count> narrow = wide;
   cat::verify(cat::dyn_invoke<treat_count>(narrow) == 6);
}

// `dyn_ptr` supports narrowing too. The pointed-to value remains addressable
// through the narrower view.
$test(dyn_ptr_narrowing) {
   kitty cat{20};
   cat::dyn_ptr<say, treat_count, feed> wide{&cat};
   cat::dyn_ptr<treat_count, feed> narrow = wide;
   cat::verify(narrow.has_value());
   cat::verify(narrow.get() == wide.get());
   cat::verify(cat::dyn_invoke<treat_count>(*narrow) == 20);
}

// A null `dyn_ptr<Superset...>` narrows to a null `dyn_ptr<Subset...>`.
$test(dyn_ptr_null_narrowing) {
   cat::dyn_ptr<say, treat_count, feed> wide;
   cat::verify(wide == nullptr);
   cat::dyn_ptr<treat_count> narrow = wide;
   cat::verify(narrow == nullptr);
}

// `const_dyn_ptr` supports narrowing the same way as `dyn_ptr`, including
// converting from a non-const `dyn_ptr<Superset...>` to a narrower const view.
$test(const_dyn_ptr_narrowing) {
   kitty cat{14};
   cat::dyn_ptr<say, treat_count> wide{&cat};
   cat::const_dyn_ptr<treat_count> narrow = wide;
   cat::verify(narrow.has_value());
   cat::verify(cat::dyn_invoke<treat_count>(*narrow) == 14);
}

// Narrowing preserves the type descriptor so `dyn_cast` still recovers the
// original concrete type.
$test(dyn_narrowing_preserves_descriptor) {
   kitty cat{77};
   cat::dyn_ref<say, treat_count, feed> wide{cat};
   cat::dyn_ref<treat_count> narrow = wide;
   cat::verify(narrow.p_type_id() == cat::p_dyn_type_id_for<kitty>);

   cat::dyn_ptr<say, treat_count, feed> wide_p{&cat};
   cat::dyn_ptr<treat_count> narrow_p = wide_p;
   cat::verify(cat::dyn_cast<kitty>(narrow_p) == &cat);
}

// Narrowed views forward to the same function pointer as a freshly built
// narrow view of the same value.
$test(dyn_narrowing_dispatches_same_fn) {
   kitty cat{1};
   cat::dyn_ref<say, treat_count, feed> wide{cat};
   cat::dyn_ref<treat_count> narrowed = wide;
   cat::dyn_ref<treat_count> fresh{cat};
   cat::verify(
      cat::dyn_invoke<treat_count>(narrowed)
      == cat::dyn_invoke<treat_count>(fresh)
   );
}

// Narrowing rejects subsets that are NOT contiguous (e.g. skipping the
// middle method).
$test(dyn_narrowing_requires_contiguous) {
   using wide_t = cat::dyn_ref<say, treat_count, feed>;
   using non_contig_t = cat::dyn_ref<say, feed>;
   static_assert(
      !cat::is_constructible<non_contig_t, wide_t>,
      "Non-contiguous subset must not be a valid conversion"
   );
   static_assert(
      cat::is_constructible<cat::dyn_ref<treat_count, feed>, wide_t>,
      "Contiguous suffix must be a valid conversion"
   );
}

// `basic_dyn` and `dyn_inplace` produce wider `dyn_ptr`s via
// `operator&`, which can then be narrowed.
$test(dyn_holder_ptr_narrowing) {
   cat::dyn_inplace<cat::destructor, say, treat_count> dyn{kitty{30}};
   cat::dyn_ptr<say, treat_count> wide = &dyn;
   cat::dyn_ptr<treat_count> narrow = wide;
   cat::verify(cat::dyn_invoke<treat_count>(*narrow) == 30);
}

namespace {

// A trivially destructible animal. The default destructor is implicit so
// `is_trivially_destructible<triv_kitty>` holds, which is what allows it
// into holders that omit `cat::destructor`.
struct triv_kitty {
   cat::int4 treats = 0;

   [[nodiscard]]
   constexpr auto
   treat_count() const -> cat::int4 {
      return treats;
   }
};

static_assert(cat::is_trivially_destructible<triv_kitty>);
static_assert(!cat::is_trivially_destructible<kitty>);

}  // namespace

// Holders without `cat::destructor` accept trivially destructible types and
// reject types that need a runtime destructor call.
$test(dyn_destructor_optional_for_trivial_types) {
   using dyn_no_dtor = cat::dyn_inplace<treat_count>;

   static_assert(cat::is_constructible<dyn_no_dtor, triv_kitty>);
   static_assert(!cat::is_constructible<dyn_no_dtor, kitty>);

   dyn_no_dtor dyn{triv_kitty{17}};
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 17);

   dyn.emplace<triv_kitty>(99_i4);
   cat::verify(cat::dyn_invoke<treat_count>(dyn) == 99);

   cat::page_allocator local_pager;
   cat::span page = local_pager.alloc_multi<cat::byte>(4_uki).or_exit();
   $defer {
      local_pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   using basic_no_dtor = cat::dyn<cat::linear_allocator, treat_count>;
   static_assert(
      cat::is_constructible<basic_no_dtor, cat::linear_allocator&, triv_kitty>
   );
   static_assert(
      !cat::is_constructible<basic_no_dtor, cat::linear_allocator&, kitty>
   );

   basic_no_dtor heap_dyn{allocator, triv_kitty{42}};
   cat::verify(cat::dyn_invoke<treat_count>(heap_dyn) == 42);
}
