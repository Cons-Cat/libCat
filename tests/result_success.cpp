enum MyErrors
{
    OK = 0,
    FAIL = 1,
};

auto result_factory(MyErrors error) -> Result<> {
    if (error == MyErrors::OK) {
        return ok;
    }
    return Failure(error);
}

void meow() {
    result_factory(MyErrors::OK).or_panic();
}
