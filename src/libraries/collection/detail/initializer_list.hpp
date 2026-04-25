// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Freestanding `std::initializer_list` (language support library contract).

namespace std {

template <typename T>
class initializer_list {
  public:
    using value_type = T;
    using reference = T const&;
    using const_reference = T const&;
    using size_type = __SIZE_TYPE__;
    using iterator = T const*;
    using const_iterator = T const*;

    constexpr initializer_list() noexcept = default;

    constexpr initializer_list(T const* p_data, __SIZE_TYPE__ in_size) noexcept
        : m_p_data(p_data)
        , m_size(in_size) {
    }

    // Clang requires this be `constexpr` rather than `consteval`
    [[nodiscard]] constexpr auto size() const noexcept -> __SIZE_TYPE__ {
        return m_size;
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return m_size == 0u;
    }

    [[nodiscard]] constexpr auto begin() const noexcept -> T const* {
        return m_p_data;
    }

    [[nodiscard]] constexpr auto end() const noexcept -> T const* {
        return m_p_data + m_size;
    }

  private:
    T const* m_p_data = nullptr;
    __SIZE_TYPE__ m_size = 0u;
};

template <class T>
[[nodiscard]] constexpr auto begin(initializer_list<T> inits) noexcept
    -> T const* {
    return inits.begin();
}

template <class T>
[[nodiscard]] constexpr auto end(initializer_list<T> inits) noexcept
    -> T const* {
    return inits.end();
}

template <class T>
[[nodiscard]] constexpr auto
cbegin(initializer_list<T> inits) noexcept -> T const* {
    return inits.begin();
}

template <class T>
[[nodiscard]] constexpr auto cend(initializer_list<T> inits) noexcept
    -> T const* {
    return inits.end();
}

}  // namespace std

namespace cat {
using std::initializer_list;
}
