/// Comprehensive error handling tests

// ============================================================
// Error Definitions
// ============================================================

// Simple error with no fields
SimpleError :: err;

// Error with single field
ValidationError :: err {
    field str
}

// Error with multiple fields
NetworkError :: err {
    code int,
    host str,
    timeout int
}

// ============================================================
// Helper Functions
// ============================================================

fn always_succeeds() -> int or err {
    return 42;
}

fn always_fails() -> int or err {
    fail "always fails";
}

fn fails_with_simple() -> int or err {
    fail SimpleError { message: "simple error" };
}

fn fails_with_validation() -> int or err {
    fail ValidationError { message: "invalid input", field: "email" };
}

fn fails_with_network() -> int or err {
    fail NetworkError { message: "connection refused", code: 111, host: "localhost", timeout: 30 };
}

fn returns_zero() -> int or err {
    return 0;
}

fn returns_empty_string() -> str or err {
    return "";
}

fn returns_true() -> bool or err {
    return true;
}

fn returns_false() -> bool or err {
    return false;
}

// ============================================================
// Test: or return
// ============================================================

fn test_or_return_success() {
    x := always_succeeds() or return;
    assert_eq(x, 42);
}

fn test_or_return_with_value() -> int {
    x := always_fails() or return 99;
    return x;
}

fn helper_or_return_with_value() -> int {
    return test_or_return_with_value();
}

fn test_or_return_with_value_wrapper() {
    result := helper_or_return_with_value();
    assert_eq(result, 99);
}

// ============================================================
// Test: or fail err
// ============================================================

fn test_or_fail_success() or err {
    x := always_succeeds() or fail err;
    assert_eq(x, 42);
}

fn wrapper_propagate() -> int or err {
    x := fails_with_simple() or fail err;
    return x;
}

fn test_propagation_chain() or err {
    result := wrapper_propagate() or match err {
        SimpleError => 123,
        _ => fail err
    };
    assert_eq(result, 123);
}

// ============================================================
// Test: or block
// ============================================================

fn test_or_block_not_triggered() {
    triggered := false;
    x := always_succeeds() or {
        triggered = true;
        return;
    };
    assert_eq(x, 42);
    assert_eq(triggered, false);
}

// ============================================================
// Test: or match
// ============================================================

fn test_or_match_simple_error() or err {
    x := fails_with_simple() or match err {
        SimpleError => 100,
        _ => fail err
    };
    assert_eq(x, 100);
}

fn test_or_match_validation_error() or err {
    x := fails_with_validation() or match err {
        ValidationError => 200,
        SimpleError => 100,
        _ => fail err
    };
    assert_eq(x, 200);
}

fn test_or_match_network_error() or err {
    x := fails_with_network() or match err {
        NetworkError => 300,
        ValidationError => 200,
        SimpleError => 100,
        _ => fail err
    };
    assert_eq(x, 300);
}

fn test_or_match_wildcard() or err {
    x := always_fails() or match err {
        SimpleError => 100,
        _ => 999
    };
    assert_eq(x, 999);
}

fn test_or_match_success() or err {
    x := always_succeeds() or match err {
        _ => 0
    };
    assert_eq(x, 42);
}

// ============================================================
// Test: default with primitives
// ============================================================

fn test_default_zero() {
    x := 0 default 100;
    assert_eq(x, 100);
}

fn test_default_nonzero() {
    x := 42 default 100;
    assert_eq(x, 42);
}

fn test_default_false() {
    b := false default true;
    assert_eq(b, true);
}

fn test_default_true() {
    b := true default false;
    assert_eq(b, true);
}

// ============================================================
// Test: default with fallible functions
// ============================================================

fn test_default_with_zero_result() or err {
    x := returns_zero() or fail err;
    y := x default 50;
    assert_eq(y, 50);
}

// Note: string default not supported (C++ strings not valid in boolean context)

fn test_default_with_false_result() or err {
    b := returns_false() or fail err;
    result := b default true;
    assert_eq(result, true);
}

fn test_default_with_true_result() or err {
    b := returns_true() or fail err;
    result := b default false;
    assert_eq(result, true);
}

// ============================================================
// Test: Chained operations
// ============================================================

fn test_chained_success() or err {
    a := always_succeeds() or fail err;
    b := always_succeeds() or fail err;
    assert_eq(a + b, 84);
}

fn test_multiple_match_handlers() or err {
    x := fails_with_simple() or match err {
        SimpleError => 1,
        _ => fail err
    };

    y := fails_with_validation() or match err {
        ValidationError => 2,
        _ => fail err
    };

    assert_eq(x + y, 3);
}

fn deeply_nested() -> int or err {
    a := always_succeeds() or fail err;
    b := always_succeeds() or fail err;
    c := always_succeeds() or fail err;
    return a + b + c;
}

fn test_deeply_nested() or err {
    result := deeply_nested() or fail err;
    assert_eq(result, 126);
}

// ============================================================
// Test: Error with custom fields
// ============================================================

fn get_network_error() -> int or err {
    fail NetworkError {
        message: "timeout",
        code: 504,
        host: "api.example.com",
        timeout: 60
    };
}

fn test_error_fields_in_match() or err {
    x := get_network_error() or match err {
        NetworkError => 504,
        _ => 0
    };
    assert_eq(x, 504);
}

// ============================================================
// Test: Multiple error types in one function
// ============================================================

fn might_fail_differently(int which) -> int or err {
    if which == 1 {
        fail SimpleError { message: "simple" };
    }

    if which == 2 {
        fail ValidationError { message: "validation", field: "x" };
    }

    if which == 3 {
        fail NetworkError { message: "network", code: 500, host: "h", timeout: 10 };
    }

    return which * 10;
}

fn test_multiple_error_types() or err {
    // Success case
    a := might_fail_differently(0) or fail err;
    assert_eq(a, 0);

    // Simple error
    b := might_fail_differently(1) or match err {
        SimpleError => 1,
        _ => fail err
    };
    assert_eq(b, 1);

    // Validation error
    c := might_fail_differently(2) or match err {
        ValidationError => 2,
        _ => fail err
    };
    assert_eq(c, 2);

    // Network error
    d := might_fail_differently(3) or match err {
        NetworkError => 3,
        _ => fail err
    };
    assert_eq(d, 3);
}

// ============================================================
// Test: Propagation through multiple layers
// ============================================================

fn layer1() -> int or err {
    return always_fails() or fail err;
}

fn layer2() -> int or err {
    return layer1() or fail err;
}

fn layer3() -> int or err {
    return layer2() or fail err;
}

fn test_deep_propagation() or err {
    result := layer3() or match err {
        _ => 999
    };
    assert_eq(result, 999);
}

// ============================================================
// Test: Mixed success and failure paths
// ============================================================

fn conditional_fail(bool should_fail) -> int or err {
    if should_fail {
        fail "conditional failure";
    }
    return 100;
}

fn test_conditional_success() or err {
    x := conditional_fail(false) or fail err;
    assert_eq(x, 100);
}

fn test_conditional_failure() or err {
    x := conditional_fail(true) or match err {
        _ => 200
    };
    assert_eq(x, 200);
}

// ============================================================
// Test: Error messages
// ============================================================

fn get_error_with_message() -> int or err {
    fail "specific error message";
}

fn test_error_message_handling() or err {
    x := get_error_with_message() or match err {
        _ => 42
    };
    assert_eq(x, 42);
}
