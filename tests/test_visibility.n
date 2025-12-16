@private fn internal_helper() -> int {
    return 42;
}

fn test_private_function_callable_in_same_file() {
    result := internal_helper();
    assert_eq(result, 42);
}

@private PrivateStruct :: struct {
    value int
}

fn test_private_struct_usable_in_same_file() {
    p := PrivateStruct { value: 100 };
    assert_eq(p.value, 100);
}
