#include <cunistd>
#include <errno.h>

enum ErrorCode
{
    OK = 0,
    FAIL = 1,
};

auto is_ok(ErrorCode error) -> bool {
    return error == ErrorCode::OK;
}

auto result_factory(ErrorCode error) -> Result<void, ErrorCode> {
    return error;
}

auto main() -> int {
    result_factory(ErrorCode::OK).or_panic();
    result_factory(ErrorCode::FAIL).or_panic();
    return EXIT_SUCCESS;
}
