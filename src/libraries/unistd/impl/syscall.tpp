// -*- mode: c++ -*-
// vim: set ft=cpp:

template <typename ReturnType, typename T, typename... Args>
auto syscall(T call, Args... parameters) -> Result<ReturnType>
requires(sizeof...(Args) < 7) {
    static constexpr isize length = sizeof...(Args);
    Any arguments[length] = {parameters...};

    using namespace std::detail;
    if constexpr (length == 0) {
        syscall0(call);
        return okay;
    } else if constexpr (length == 1) {
        syscall1(call, arguments[0]);
        return okay;
    } else if constexpr (length == 2) {
        syscall2(call, arguments[0], arguments[1]);
        return okay;
    } else if constexpr (length == 3) {
        return syscall3(call, arguments[0], arguments[1], arguments[2]);
    } else if constexpr (length == 4) {
        syscall4(call, arguments[0], arguments[1], arguments[2], arguments[3]);
        return okay;
    } else if constexpr (length == 5) {
        syscall5(call, arguments[0], arguments[1], arguments[2], arguments[3],
                 arguments[4]);
        return okay;
    } else if constexpr (length == 6) {
        return syscall6(call, arguments[0], arguments[1], arguments[2],
                        arguments[3], arguments[4], arguments[5]);
    }
    __builtin_unreachable();
}
