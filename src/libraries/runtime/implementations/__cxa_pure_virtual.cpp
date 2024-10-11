#include <cat/runtime>
#include <cat/string>

// This function is a stub.
extern "C" void
cat::__cxa_pure_virtual() {
   verify(false, "A pure virtual function was called with no implementation.");
}
