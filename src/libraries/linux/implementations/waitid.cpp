#include <linux>

auto waitid(WaitIdType type, ProcessId id, int8 options) -> Result<Any> {
    return syscall5(247u, type, id, nullptr, options, nullptr);
}
