// Placement `new`.
[[nodiscard]]
auto
operator new(unsigned long, void* p_address) -> void* {
   return p_address;
}

[[nodiscard]]
auto
operator new[](unsigned long, void* p_address) -> void* {
   return p_address;
}

[[nodiscard]]
auto
operator new[](unsigned long) -> void* {
   return __builtin_bit_cast(void*, 1ul);
}

[[nodiscard]]
auto
operator new[](unsigned long, std::align_val_t align) -> void* {
   return __builtin_bit_cast(void*, align);
}

void
operator delete[](void*) {
}

void
operator delete[](void*, unsigned long) {
}

void
operator delete[](void*, unsigned long, std::align_val_t) {
}
