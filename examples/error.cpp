enum class MyErrors {
    ok = 0,
    fail = 1,
};

auto result_factory(MyErrors error) -> Result<> {
    if (error == MyErrors::ok) {
        return okay;
    }
    return Failure(error);
}

void meow() {
    // This will never panic:
    result_factory(MyErrors::ok).or_panic();
    // This will always panic:
    result_factory(MyErrors::fail).or_panic();
    cat::exit();
}
