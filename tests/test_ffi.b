@extern("c") fn puts(cstr s) -> cint;
@extern("m") fn sqrt(f64 x) -> f64;
@extern("m") fn floor(f64 x) -> f64;

fn test_puts() {
    puts("Hello from C FFI!");
}

fn test_sqrt() {
    result := sqrt(16.0);
    floored := floor(result);
    assert_eq(floored, 4.0);
}

fn test_floor() {
    result := floor(3.7);
    assert_eq(result, 3.0);
}
