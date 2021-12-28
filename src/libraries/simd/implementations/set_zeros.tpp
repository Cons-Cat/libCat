// -*- mode: c++ -*-
// vim: set ft=cpp:

template <typename T>
consteval auto simd::set_zeros() -> T {
    // TODO: Is there a cleverer way to do this? Variadic templates?
    // Probably an integer_sequence.
    using ScalarType = typename T::ScalarType;
    using VectorType = std::detail::SimdVector<ScalarType, T::width>;
    using IntrinsicType = decltype(VectorType::value);

    if constexpr (T::width == 2) {
        return IntrinsicType{0, 0};
    } else if constexpr (T::width == 4) {
        return IntrinsicType{0, 0, 0, 0};
    } else if constexpr (T::width == 8) {
        return IntrinsicType{0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::width == 16) {
        return IntrinsicType{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::width == 32) {
        return IntrinsicType{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }
    __builtin_unreachable();
}
