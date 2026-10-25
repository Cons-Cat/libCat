#include <cat/runtime>
#include <cat/string>

// This function is a stub.
extern "C" void
cat::__cxa_pure_virtual() noexcept(!__has_feature(cxx_exceptions)) {
   verify(false, "A pure virtual function was called with no implementation.");
}
