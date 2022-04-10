// -*- mode: c++ -*-
// vim: set ft=cpp:

template <typename T>
consteval auto simd::set_zeros() -> T {
    // TODO: Is there a cleverer way to do this? Variadic templates?
    // Probably an integer_sequence.
    using Scalar = typename T::Scalar;
    using Vector = cat::detail::Simd<Scalar, T::size(), alignof(T)>;
    using Intrinsic = decltype(Vector::value);

    if constexpr (T::size() == 2) {
        return Intrinsic{0, 0};
    } else if constexpr (T::size() == 4) {
        return Intrinsic{0, 0, 0, 0};
    } else if constexpr (T::size() == 8) {
        return Intrinsic{0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::size() == 16) {
        return Intrinsic{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    } else if constexpr (T::size() == 32) {
        return Intrinsic{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }
    __builtin_unreachable();
}
