// Placement `new`.

[[nodiscard, gnu::returns_nonnull]]
auto
operator new(unsigned long /*unused*/, void* _Nonnull p_address)
   -> void* _Nonnull {
   return p_address;
}

[[nodiscard, gnu::returns_nonnull]]
auto
operator new[](unsigned long /*unused*/, void* _Nonnull p_address)
   -> void* _Nonnull {
   return p_address;
}

[[nodiscard, gnu::returns_nonnull]]
auto
operator new[](unsigned long /*unused*/) -> void* _Nonnull {
   return __builtin_bit_cast(void*, 1ul);
}

[[nodiscard, gnu::returns_nonnull]]
auto
operator new[](unsigned long /*unused*/, std::align_val_t align)
   -> void* _Nonnull {
   return __builtin_bit_cast(void*, align);
}

void
operator delete[](void* _Nullable /*unused*/) {
}

void
operator delete[](void* _Nullable /*unused*/, unsigned long /*unused*/) {
}

void
operator delete
   [](void* _Nullable /*unused*/, unsigned long /*unused*/,
      std::align_val_t /*unused*/) {
}
