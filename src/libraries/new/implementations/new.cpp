#include <cat/new>

[[nodiscard, gnu::returns_nonnull]]
auto
operator new(unsigned long /*unused*/) noexcept(false) -> void* _Nonnull {
   return __builtin_bit_cast(void*, 1ul);
}

[[nodiscard, gnu::returns_nonnull]]
auto
operator new(unsigned long /*unused*/, void* _Nonnull p_address) noexcept
   -> void* _Nonnull {
   return p_address;
}

[[nodiscard, gnu::returns_nonnull]]
auto
operator new[](unsigned long /*unused*/, void* _Nonnull p_address) noexcept
   -> void* _Nonnull {
   return p_address;
}

[[nodiscard, gnu::returns_nonnull]]
auto
operator new[](unsigned long /*unused*/) noexcept(false) -> void* _Nonnull {
   return __builtin_bit_cast(void*, 1ul);
}

[[nodiscard, gnu::returns_nonnull]]
auto
operator new[](unsigned long /*unused*/, std::align_val_t align) noexcept(false)
   -> void* _Nonnull {
   return __builtin_bit_cast(void*, align);
}

void
operator delete(void* _Nullable /*unused*/) noexcept {
}

void
operator delete(void* _Nullable /*unused*/, unsigned long /*unused*/) noexcept {
}

void
operator delete[](void* _Nullable /*unused*/) noexcept {
}

void
operator delete
   [](void* _Nullable /*unused*/, unsigned long /*unused*/) noexcept {
}

void
operator delete
   [](void* _Nullable /*unused*/, unsigned long /*unused*/,
      std::align_val_t /*unused*/) noexcept {
}
