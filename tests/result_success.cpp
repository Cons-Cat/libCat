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
    return false;
}

void meow() {
    // Do not panic when taking in a non-Failure.
    result_factory(MyErrors::OK).or_panic();

    // Construct on a `bool1` r-value.
    Result(true).assert();
    // TODO: This line does not compile:
    // Result(true).assert("Text");
    Result(true).or_panic();
    // TODO: This line does not compile:
    // Result(true).or_panic("Foo");

    // Do not panic when false comes from a different constructor.
    make_false().or_panic();
    cat::exit();
}
