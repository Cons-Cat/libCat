#if __has_feature(cxx_exceptions)
#include <cat/detail/cxa_abi.hpp>

#include <cat/string>

// Even with `-fno-rtti` Clang still uses these descriptors for exception
// handling. For now, this is a minimal implementation that only supports
// exceptions, not `dynamic_cast` or `typeid`.

namespace cat::detail {

extern "C" {

alignas(void*) void* _ZTVN10__cxxabiv117__class_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv120__si_class_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv121__vmi_class_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv123__fundamental_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv119__pointer_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv129__pointer_to_member_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv116__enum_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv120__function_type_infoE[3] = {};
alignas(void*) void* _ZTVN10__cxxabiv117__array_type_infoE[3] = {};

struct type_descriptor {
   void* vtable;
   char const* name;
};

struct fundamental_pointer {
   void* vtable;
   char const* name;
   uint4 flags;
   type_descriptor const* pointee;
};

#define CAT_FUNDAMENTAL(suffix, spelling)                                     \
   type_descriptor _ZTI##suffix{                                              \
      static_cast<void*>(                                                     \
         _ZTVN10__cxxabiv123__fundamental_type_infoE + 2                      \
      ),                                                                      \
      spelling                                                                \
   };                                                                         \
   fundamental_pointer _ZTIP##suffix{                                         \
      static_cast<void*>(_ZTVN10__cxxabiv119__pointer_type_infoE + 2),        \
      "P" spelling, 0, &_ZTI##suffix                                          \
   };                                                                         \
   fundamental_pointer _ZTIPK##suffix {                                       \
      static_cast<void*>(_ZTVN10__cxxabiv119__pointer_type_infoE + 2),        \
         "PK" spelling, 1, &_ZTI##suffix                                      \
   }
CAT_FUNDAMENTAL(v, "v");
CAT_FUNDAMENTAL(b, "b");
CAT_FUNDAMENTAL(c, "c");
CAT_FUNDAMENTAL(a, "a");
CAT_FUNDAMENTAL(h, "h");
CAT_FUNDAMENTAL(s, "s");
CAT_FUNDAMENTAL(t, "t");
CAT_FUNDAMENTAL(i, "i");
CAT_FUNDAMENTAL(j, "j");
CAT_FUNDAMENTAL(l, "l");
CAT_FUNDAMENTAL(m, "m");
CAT_FUNDAMENTAL(x, "x");
CAT_FUNDAMENTAL(y, "y");
CAT_FUNDAMENTAL(n, "n");
CAT_FUNDAMENTAL(o, "o");
CAT_FUNDAMENTAL(f, "f");
CAT_FUNDAMENTAL(d, "d");
CAT_FUNDAMENTAL(e, "e");
CAT_FUNDAMENTAL(g, "g");
CAT_FUNDAMENTAL(w, "w");
CAT_FUNDAMENTAL(Dn, "Dn");
CAT_FUNDAMENTAL(Di, "Di");
CAT_FUNDAMENTAL(Ds, "Ds");
CAT_FUNDAMENTAL(Du, "Du");
CAT_FUNDAMENTAL(Dh, "Dh");
#undef CAT_FUNDAMENTAL
}

namespace {
struct single_type : type_descriptor {
   type_descriptor const* base;
};

struct base_type {
   type_descriptor const* type;
   iword offset_flags;
};

struct multiple_type : type_descriptor {
   uint4 flags;
   uint4 count;
   base_type bases[1];
};

struct pointer_type : type_descriptor {
   uint4 flags;
   type_descriptor const* pointee;
};

constexpr uint4 cv_qualifier_mask = 7u;
constexpr uint4 dropped_qualifier_mask = 0x60u;
constexpr uint4 virtual_base_flag = 1u;
constexpr uint4 public_base_flag = 2u;
constexpr uint4 base_offset_shift = 8u;

auto
abi_vtable(void* (&table)[3]) -> void* {
   return static_cast<void*>(table + 2);
}

auto
equal(type_descriptor const* p_left, type_descriptor const* p_right) -> bool {
   if (p_left == p_right) {
      return true;
   }
   return zstr_view(p_left->name) == zstr_view(p_right->name);
}

auto
is_pointer_descriptor(type_descriptor const* p_type) -> bool {
   return p_type->vtable == abi_vtable(_ZTVN10__cxxabiv119__pointer_type_infoE);
}

// Count distinct matching subobjects, including inaccessible paths, so a
// private duplicate cannot accidentally turn an ambiguous base into a match.
struct base_match {
   void* object = nullptr;
   bool found = false;
   bool public_path = false;
   bool ambiguous = false;
   type_descriptor const* virtual_root = nullptr;
   iword offset = 0;
};

void
// NOLINTNEXTLINE(misc-no-recursion)
find_base(
   type_descriptor const* p_target, type_descriptor const* p_source,
   void* p_object, bool public_path, base_match& match,
   type_descriptor const* p_virtual_root = nullptr, iword path_offset = 0
) {
   if (equal(p_target, p_source)) {
      if (
         match.found
         && (p_object != nullptr ? match.object != p_object : match.virtual_root != p_virtual_root || match.offset != path_offset)
      ) {
         match.ambiguous = true;
      }
      match.found = true;
      match.object = p_object;
      match.virtual_root = p_virtual_root;
      match.offset = path_offset;
      match.public_path |= public_path;
      return;
   }
   if (
      p_source->vtable == abi_vtable(_ZTVN10__cxxabiv120__si_class_type_infoE)
   ) {
      find_base(
         p_target, static_cast<single_type const*>(p_source)->base, p_object,
         public_path, match, p_virtual_root, path_offset
      );
   } else if (
      p_source->vtable == abi_vtable(_ZTVN10__cxxabiv121__vmi_class_type_infoE)
   ) {
      auto const* p_type = static_cast<multiple_type const*>(p_source);
      for (idx i = 0u; i < p_type->count; ++i) {
         auto const& base = p_type->bases[i];
         auto const encoded = static_cast<uword>(base.offset_flags);
         auto offset = base.offset_flags >> base_offset_shift;
         bool const is_virtual = (encoded & virtual_base_flag) != 0u;
         bool const is_public = (encoded & public_base_flag) != 0u;
         if (is_virtual && p_object != nullptr) {
            auto const* p_vtable = *static_cast<char const* const*>(p_object);
            auto const p_slot = intptr<char const>(p_vtable) + offset;
            __builtin_memcpy_inline(&offset, p_slot.get(), sizeof(offset));
         }
         void* p_subobject =
            p_object != nullptr
               ? (intptr<char>(static_cast<char*>(p_object)) + offset).get()
               : nullptr;
         find_base(
            p_target, base.type, p_subobject, public_path && is_public, match,
            is_virtual ? base.type : p_virtual_root,
            is_virtual ? 0 : path_offset + offset
         );
      }
   }
}

auto
// NOLINTNEXTLINE(misc-no-recursion)
match_type(
   type_descriptor const* p_caught, type_descriptor const* p_thrown,
   void*& object, uint4 depth = 0u
) -> bool {
   if (equal(p_caught, p_thrown)) {
      return true;
   }
   if (is_pointer_descriptor(p_caught)) {
      if (equal(p_thrown, &_ZTIDn)) {
         object = nullptr;
         return true;
      }

      if (!is_pointer_descriptor(p_thrown)) {
         return false;
      }

      auto const* p_to = static_cast<pointer_type const*>(p_caught);
      auto const* p_from = static_cast<pointer_type const*>(p_thrown);

      // cv-qualifiers may be added, never removed. `noexcept` may be dropped.
      if (
         (p_from->flags & cv_qualifier_mask & ~p_to->flags) != 0
         || (p_to->flags & dropped_qualifier_mask & ~p_from->flags) != 0
      ) {
         return false;
      }

      if (
         depth == 0 && equal(p_to->pointee, &_ZTIv)
         && p_from->pointee->vtable
               != abi_vtable(_ZTVN10__cxxabiv120__function_type_infoE)
      ) {
         return true;
      }

      if (
         !equal(p_to->pointee, p_from->pointee)
         && is_pointer_descriptor(p_to->pointee) && (p_to->flags & 1u) == 0
      ) {
         return false;
      }

      return match_type(p_to->pointee, p_from->pointee, object, depth + 1);
   }

   if (depth > 1) {
      return false;
   }

   base_match match;
   find_base(p_caught, p_thrown, object, true, match);

   if (!match.found || !match.public_path || match.ambiguous) {
      return false;
   }

   object = match.object;
   return true;
}
}  // namespace

auto
exception_matches(void const* p_caught, void*& p_object, void const* p_thrown)
   -> bool {
   auto const* p_from = static_cast<type_descriptor const*>(p_thrown);
   void* p_value =
      is_pointer_descriptor(p_from) ? *static_cast<void**>(p_object) : p_object;
   bool const matched = match_type(
      static_cast<type_descriptor const*>(p_caught), p_from, p_value
   );

   if (matched) {
      p_object = p_value;
   }

   return matched;
}

}  // namespace cat::detail

#endif
