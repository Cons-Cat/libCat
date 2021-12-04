#include <cunistd>
#include <errno.h>

#include "result.hpp"

enum class MyErrors
{
    OK = 0,
    FAIL = 1,
};

auto is_ok(Error error) -> bool {
    return error.code == 0;
}

auto result_factory(MyErrors error) -> Result<void> {
    return Error(error);
}

void meow() {
    result_factory(MyErrors::OK).or_panic();
    result_factory(MyErrors::FAIL).or_panic();
}
