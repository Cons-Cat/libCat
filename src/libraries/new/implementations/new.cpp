// Placement `new`.
[[nodiscard]]
auto
operator new(unsigned long /*unused*/, void* p_address) -> void* {
   return p_address;
}

[[nodiscard]]
auto
operator new[](unsigned long /*unused*/, void* p_address) -> void* {
   return p_address;
}

[[nodiscard]]
auto
operator new[](unsigned long /*unused*/) -> void* {
   return __builtin_bit_cast(void*, 1ul);
}

[[nodiscard]]
auto
operator new[](unsigned long /*unused*/, std::align_val_t align) -> void* {
   return __builtin_bit_cast(void*, align);
}

void
operator delete[](void* /*unused*/) {
}

void
operator delete[](void* /*unused*/, unsigned long /*unused*/) {
}

void
operator delete[](void* /*unused*/, unsigned long /*unused*/,
                  std::align_val_t /*unused*/) {
}
