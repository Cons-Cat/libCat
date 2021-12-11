#include <result.h>

enum MyErrors
{
    OK = 0,
    FAIL = 1,
};

auto result_factory(MyErrors error) -> Result<> {
    if (error == ::OK) {
        return 'a';
    }
    return Failure(error);
}

void meow() {
    result_factory(::OK).or_panic();
    result_factory(::FAIL).or_panic();
}
