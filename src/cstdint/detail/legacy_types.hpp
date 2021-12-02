namespace std {

/* This library follows the same assumptions as the GNU LibC. These type
 * definitions are accurate for all implementations of GCC, currently, across
 * all environments. */
namespace detail::ints {
    /* The std::detail::ints namespace exists to make using these in both the
     * global and std:: namespace simple. */
    using int8_t = signed char;
    using uint8_t = unsigned char;
    using int16_t = signed short;
    using uint16_t = unsigned short;
    using int32_t = signed int;
    using uint32_t = unsigned int;
    // The 64-bit types assume that they target x86-64, rather than 32-bit x86:
    using int64_t = signed long;
    using uint64_t = unsigned long;
    using ssize_t = int64_t;
    using size_t = uint64_t;

    /* These 128 bit types are not required by the C standard, and technically a
     * LibC is only allowed to define types that range from 1 to 64 bits.
     * However, clangd gives me an error on unsigned __int128_t, and while it
     * doesn't complain about __uint128_t, that type was deprecated in
     * GCC 4.6. I have moved these aliases into this file simply to keep the
     * error away from any file that I care about. */
    // These are GCC built-in types:
    using int128_t = __int128_t;
    using uint128_t = unsigned __int128_t;

    // These remaining ints are not used anywhere throughout this library.
    using int_least8_t = int8_t;
    using int_least16_t = int16_t;
    using int_least32_t = int32_t;
    using int_least64_t = int64_t;
    using uint_least8_t = uint8_t;
    using uint_least16_t = uint16_t;
    using uint_least32_t = uint32_t;
    using uint_least64_t = uint64_t;
    using int_fast8_t = signed char;
    using int_fast16_t = signed long;
    using int_fast32_t = signed long;
    using int_fast64_t = signed long;
    using uint_fast8_t = unsigned char;
    using uint_fast16_t = unsigned long;
    using uint_fast32_t = unsigned long;
    using uint_fast64_t = unsigned long;
    using intptr_t = signed long;
    using uintptr_t = unsigned long;

}  // namespace detail::ints

// Make LibC ints available in the std:: namespace.
using namespace std::detail::ints;

}  // namespace std

// Make LibC ints available in the global namespace.
using namespace std::detail::ints;
