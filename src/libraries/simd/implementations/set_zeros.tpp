// -*- mode: c++ -*-
// vim: set ft=cpp:

template <typename T>
consteval auto simd::set_zeros() -> T {
    // TODO: Is there a cleverer way to do this? Variadic templates?
    // Probably an integer_sequence.
    using Scalar = typename T::Scalar;
    using Vector = cat::detail::Simd<Scalar, T::width>;
    using Intrinsic = decltype(Vector::value);

    if constexpr (T::width == 2) {
        return Intrinsic{0, 0};
    } else if constexpr (T::width == 4) {
        return Intrinsic{0, 0, 0, 0};
    } else if constexpr (T::width == 8) {
        return Intrinsic{0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::width == 16) {
        return Intrinsic{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::width == 32) {
        return Intrinsic{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }
    __builtin_unreachable();
}
