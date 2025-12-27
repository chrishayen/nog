// Error test: fail in non-fallible function

fn bad() -> int {
    fail "oops";
}

fn main() {
}
