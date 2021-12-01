#pragma once

#include <cstdlib>
#include <type_traits>
#include <utility>

template <typename T, typename UError>
struct [[nodiscard]] Result {
    // TODO: Investigate making the data and error a sum type.
  private:
    using DataType = std::conditional_t<std::is_void_v<T>, void*, T>;
    DataType data;

  public:
    UError code;

    Result() = delete;

    // NOLINTNEXTLINE I do not want explicit constructors here.
    constexpr Result(UError&& in_code) {
        this->code = std::forward<UError>(in_code);
    }

    // NOLINTNEXTLINE I do not want explicit constructors here.
    constexpr Result(DataType&& in_data) {
        this->data = std::forward<DataType>(in_data);
    }

    // NOLINTNEXTLINE I do not want explicit constructors here.
    constexpr Result(DataType&& in_data, UError&& in_code) {
        this->data = std::forward<DataType>(in_data);
        this->code = std::forward<UError>(in_code);
    }

    // TODO: Pass in the exit code.
    auto or_panic(char const* p_error_message = "Panic!") -> T {
        if (is_ok(this->code)) {
            // TODO: Print the error message.
            exit(EXIT_FAILURE);
        } else if constexpr (!std::is_void_v<T>) {
            return this->data;
        }
    }

    auto or_return(DataType const& in_data) -> DataType {
        return in_data;
    }

    // TODO: Add std::invokable concept
    auto or_do(auto callback) {
        if (!is_ok(this->code)) {
            return callback();
        } else if constexpr (!std::is_void_v<T>) {
            if constexpr (!std::is_void_v<T>) {
                return this->data;
            }
        }
    }

    // TODO: This should be removed if this becomes a sum type. It is only
    // useful for recursion.
    auto or_propagate() -> Result<T, UError> {
        return *this;
    }
};
