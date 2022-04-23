#include <linux>

auto nix::waitid(WaitId type, ProcessId id, WaitOptionsFlags options)
    -> Result<Any> {
    return nix::syscall5(247, type, id, nullptr, options, nullptr);
}
