/// Test basic error definitions and fail statements

// Simple error with no fields
ParseError :: err;

// Error with custom fields
IOError :: err {
    code int,
    path str
}

fn might_fail() or err {
    fail "something went wrong";
}

fn could_fail() -> int or err {
    fail "this also fails";
}

fn returns_value() -> int or err {
    return 42;
}

fn test_error_definitions() {
    // This test just verifies that error definitions compile
    assert_eq(1, 1);
}

fn test_or_return() {
    // Test or return handling
    x := returns_value() or return;
    assert_eq(x, 42);
}

fn test_default() {
    // Test default for falsy values
    x := 0 default 10;
    assert_eq(x, 10);

    y := 5 default 10;
    assert_eq(y, 5);
}

fn test_or_fail() or err {
    // Test or fail - propagate errors
    x := returns_value() or fail err;
    assert_eq(x, 42);
}

fn test_or_match() or err {
    // Test or match with different error types
    x := returns_value() or match err {
        IOError => 0,
        _ => fail err
    };
    assert_eq(x, 42);
}

fn returns_io_error() -> int or err {
    fail IOError { message: "not found", code: 404, path: "/test" };
}

fn test_or_match_typed() or err {
    // Test or match with typed error
    x := returns_io_error() or match err {
        IOError => 999,
        _ => fail err
    };
    assert_eq(x, 999);
}

fn test_or_block() {
    // Test or block handler
    handled := false;
    x := returns_value() or {
        handled = true;
        return;
    };
    assert_eq(x, 42);
    assert_eq(handled, false);
}

fn wrapper() -> int or err {
    // Propagate error with or fail
    x := returns_io_error() or fail err;
    return x;
}

fn test_error_propagation() or err {
    // Test that errors propagate correctly
    result := wrapper() or match err {
        IOError => 123,
        _ => fail err
    };
    assert_eq(result, 123);
}
