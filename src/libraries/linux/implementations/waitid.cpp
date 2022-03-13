#include <linux>

auto nix::waitid(WaitIdType type, ProcessId id, int8 options) -> Result<Any> {
    return nix::syscall5(247u, type, id, nullptr, options, nullptr);
}
