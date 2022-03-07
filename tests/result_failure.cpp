enum MyErrors
{
    OK = 0,
    FAIL = 1,
};

auto result_factory(MyErrors error) -> Result<> {
    if (error == ::OK) {
        return okay;
    }
    return Failure(error);
}

void meow() {
    result_factory(::FAIL).or_panic();
    exit();
}
