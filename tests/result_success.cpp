enum MyErrors {
    OK = 0,
    FAIL = 1,
};

auto result_factory(MyErrors error) -> Result<> {
    if (error == MyErrors::OK) {
        return okay;
    }
    return Failure(error);
}

auto make_false() -> Result<bool1> {
    // TODO: Find a way to simplify this.
    return bool1(false);
}

void meow() {
    // Do not panic when taking in a non-Failure.
    result_factory(MyErrors::OK).or_panic();
    // Construct on a bool r-value.
    Result(true).or_panic();
    // Do not panic when false comes from a different constructor.
    make_false().or_panic();
    cat::exit();
}
